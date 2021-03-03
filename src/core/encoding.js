/*****************************************************************************
 * Copyright (c) 2021 Bosch.IO GmbH
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 ****************************************************************************/

const jsonata = require('jsonata');

const Logger = require('./util/logger');
const { NamedMqttBroker } = require('./util/mqttBroker');
const InstanceManager = require('./instanceManager');

const {
    ENCODING_TOPIC,
    ROUTING_TOPIC,
    ENCODING_ENCODER_CATEGORY,
    ENCODING_ENCODER_MINMAX,
    ENCODING_ENCODER_DELTA,
    ENCODING_ENCODER_THROUGH,
    ENCODING_TYPE_NUMBER,
    ENCODING_TYPE_STRING,
    ENCODING_TYPE_OBJECT,
    ENCODING_TYPE_BOOLEAN
} = require('./constants');

const equals = require('./util/equals');
const jsonQuery = require('./util/jsonQuery');
const clone = require('./util/clone');

module.exports = class Encoding {
    constructor(connectionString) {
        this.logger = new Logger('Encoding');
        this.broker = new NamedMqttBroker('Encoding', connectionString);
        this.instanceManager = new InstanceManager(connectionString);
    }

    start() {
        return this.instanceManager.start()
            .then(() => this.broker.subscribeJson(`$share/encoding/${ENCODING_TOPIC}`, this.__onEvent.bind(this)))
            .then(() => {
                this.logger.info('Encoding started successfully');
            });
    }

    async __onEvent(event) {
        await this.__handleEvent(event);
    }

    async __handleEvent(ev) {
        const evtctx = Logger.createEventContext(ev);

        let feature = null;

        try {
            feature = await this.instanceManager.getFeature(ev.subject, ev.instance, ev.feature, ev.type);
        }
        catch(err) {
            // Instance does currently not exist. Will be automatically registered via setFeature if not present
        }

        try {
            const metaFeature = await this.instanceManager.getMetadataManager().resolveMetaFeature(ev.type, ev.feature);

            ev.$feature = await this.instanceManager.setFeature(
                ev.subject,
                ev.instance,
                ev.feature,
                this.__encodeValue(ev.value, metaFeature, feature),
                ev.value,
                ev.whenMs,
                ev.type
            );

            this.logger.verbose(`Forwarding event to routing stage ${JSON.stringify(ev)}`, evtctx);

            await this.broker.publishJson(ROUTING_TOPIC, ev);
        }
        catch(err) {
            this.logger.warn(err.message, evtctx, err);
        }
    }

    __encodeValue(value, metaFeature, feature) {
        if (metaFeature.encoding.encoder === null) {
            // Null-Encoding
            return null;
        }

        let rawValue = value;
        let encodedValue = null;

        const useVPath = typeof rawValue === 'object' && rawValue.$vpath;

        if (useVPath) {
            // This property is value.bound e.g. used to put event values in "carrier" values
            try {
                rawValue = jsonQuery.first(rawValue, rawValue.$vpath).value;
            }
            catch(err) {
                throw new Error(`Invalid $vpath ${rawValue.$vpath}`);
            }
        }

        if (metaFeature.encoding.reduce) {
            // This property is metadata-bound and reduces an object into a single value that can be encoded
            const reducedValue = jsonata(metaFeature.encoding.reduce).evaluate(rawValue);

            if (reducedValue === undefined) {
                throw new Error(`Invalid reducer ${metaFeature.encoding.reduce} given for value ${rawValue}`);
            }

            rawValue = reducedValue;
        }

        switch(metaFeature.encoding.encoder) {
            case ENCODING_ENCODER_THROUGH: {
                // Through is just a shorthand for minmax encoding in the range between 0 and 1
                // The value will be automatically clamped to 0 and 1 if incoming values exceed this range

                Object.assign(metaFeature.encoding, {
                        min: 0,
                        max: 1
                    });

                encodedValue = this.__encode_minmax(rawValue, metaFeature, feature);
                break;
            }
            case ENCODING_ENCODER_MINMAX: {
                encodedValue = this.__encode_minmax(rawValue, metaFeature, feature);
                break;
            }
            case ENCODING_ENCODER_DELTA: {
                encodedValue = this.__encode_delta(rawValue, metaFeature, feature);
                break;
            }
            case ENCODING_ENCODER_CATEGORY: {
                encodedValue = this.__encode_category(rawValue, metaFeature, feature);
                break;
            }
            default: {
                throw new Error(`Encoder ${metaFeature.encoding.encoder} not found`);
            }
        }

        if (!useVPath) {
            return encodedValue;
        }

        // Clone the value because of object referencing
        return jsonQuery.updateFirst(clone(value), value.$vpath, encodedValue);
    }

    __encode_minmax(value, metaFeature) {
        if (!Number.isFinite(value)) {
            throw new Error(`Expected number value, received ${typeof value}`);
        }

        const min = metaFeature.encoding.min;
        const max = metaFeature.encoding.max;
        // Clamp and scale between 0..1
        return Math.max(0, Math.min(1, (value - min) / (max - min)));
    }

    __encode_delta(value, metaFeature, feature) {
        if (!Number.isFinite(value)) {
            throw new Error(`Expected number value, received ${typeof value}`);
        }

        if (feature === null) {
            return 0;
        }

        return value - feature.raw;
    }

    __encode_category(value, metaFeature) {
        switch(metaFeature.encoding.type) {
            case ENCODING_TYPE_BOOLEAN: {
                if (!(typeof value === 'boolean')) {
                    throw new Error(`Expected boolean value, received ${typeof value}`);
                }

                if (metaFeature.encoding.enum.length !== 2) {
                    throw new Error(`Enumerations for booleans only allow true or false`);
                }

                return metaFeature.encoding.enum.indexOf(value);
            }
            case ENCODING_TYPE_NUMBER:
            case ENCODING_TYPE_STRING: {
                if (!Number.isFinite(value) && !(typeof value === 'string')) {
                    throw new Error(`Expected number or string value, received ${typeof value}`);
                }

                const idx = metaFeature.encoding.enum.indexOf(value);

                if (idx === -1) {
                    throw new Error(`Value ${value} was not found in given enumeration ${metaFeature.encoding.enum}`);
                }

                return idx / (metaFeature.encoding.enum.length - 1);
            }
            case ENCODING_TYPE_OBJECT: {
                if (!(typeof value === 'object')) {
                    throw new Error(`Expected object value, received ${typeof value}`);
                }

                for (const [idx, obj] of metaFeature.encoding.enum.entries()) {
                    if (equals(obj, value, { strictArrayOrder: false })) {
                        return idx / (metaFeature.encoding.enum.length - 1);
                    }
                }

                throw new Error(`Value ${value} was not found in given enumeration objects ${JSON.stringify(metaFeature.encoding.enum)}`);
            }
        }

        throw new Error(`No categorical encoder found for given value type ${metaFeature.encoding.type}`);
    }
};