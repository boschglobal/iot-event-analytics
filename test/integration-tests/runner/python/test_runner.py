##############################################################################
# Copyright (c) 2021 Bosch.IO GmbH
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
#
# SPDX-License-Identifier: MPL-2.0
##############################################################################

import asyncio
import logging
import os

from iotea.core.talent_test import TestRunnerTalent
from iotea.core.util.logger import Logger

logging.setLoggerClass(Logger)
logging.getLogger().setLevel(logging.INFO)

os.environ['MQTT_TOPIC_NS'] = 'iotea/'

class TestRunner(TestRunnerTalent):
    def __init__(self, connection_string):
        super(TestRunner, self).__init__('testRunner-py', ['testSet-sdk-py'], connection_string)
        
async def main():
    testrunner = TestRunner('mqtt://mosquitto-local:1883') #TODO This is a workaround as not using a config.json or new Protocol-adapter
    await testrunner.start()

if __name__ == '__main__':
    LOOP = asyncio.get_event_loop()
    LOOP.run_until_complete(main())
    LOOP.close()
