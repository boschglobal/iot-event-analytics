/*****************************************************************************
 * Copyright (c) 2021 Bosch.IO GmbH
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 ****************************************************************************/

const Talent = require('./talent');

const {
    Rule,
    OrRules,
    SchemaConstraint
} = require('./rules');

const {
    TalentInput,
    TalentOutput
} = require('./util/talentIO');

const {
    DEFAULT_TYPE,
    VALUE_TYPE_RAW,
    ENCODING_TYPE_OBJECT,
    ENCODING_TYPE_ANY,
    MSG_TYPE_ERROR
} = require('./constants');

module.exports = class FunctionTalent extends Talent {
    constructor(id, connectionString, disableMqtt5Support) {
        super(id, connectionString, disableMqtt5Support);
        this.functions = {};
    }

    registerFunction(name, cb) {
        this.functions[name] = cb;

        this.skipCycleCheckFor(`${DEFAULT_TYPE}.${this.id}.${name}-in`);

        // Define input parameters as well, so that they exist
        this.addOutput(`${name}-in`, {
            description: `Argument(s) for function ${name}`,
            encoding: {
                type: ENCODING_TYPE_OBJECT,
                encoder: null
            },
            unit: 'ONE'
        });

        this.addOutput(`${name}-out`, {
            description: `Result of function ${name}`,
            encoding: {
                type: ENCODING_TYPE_ANY,
                encoder: null
            },
            unit: 'ONE'
        });
    }

    async onEvent(ev, evtctx) {
        const rawValue = TalentInput.getRawValue(ev);
        const args = rawValue.args;
        args.push(ev);
        args.push(evtctx);

        let result = null;

        let $tsuffix = `/${rawValue.chnl}/${rawValue.call}`;

        try {
            result = TalentOutput.create(this, ev, `${rawValue.func}-out`, {
                $tsuffix,
                $vpath: 'value',
                value: await this.functions[rawValue.func](...args)
            });
        }
        catch(err) {
            result = TalentOutput.create(this, ev, `${rawValue.func}-out`, {
                $tsuffix,
                $vpath: 'error',
                error: err.message
            });

            result.msgType = MSG_TYPE_ERROR;
        }

        return [ result ];
    }

    getRules() {
        return new OrRules(Object.keys(this.functions).map(func => {
            const eventSchema = {
                type: 'object',
                required: [ 'func', 'args', 'chnl', 'call' ],
                properties: {
                    func: {
                        type: 'string',
                        const: func
                    },
                    args: {
                        type: 'array'
                    },
                    chnl: {
                        type: 'string'
                    },
                    call: {
                        type: 'string'
                    }
                },
                additionalProperties: false
            };

            return new Rule(new SchemaConstraint(`${this.id}.${func}-in`, eventSchema, DEFAULT_TYPE, VALUE_TYPE_RAW));
        }));
    }
};