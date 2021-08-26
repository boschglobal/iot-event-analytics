# Integration Tests

Integration tests are testing the interaction between multiple active components by running on a fully or partially functional platform.

# Planned: Using docker-compose to spin up integration test environment

- Run the message broker, the platform and active test components

    `docker-compose -f docker-compose.mosquitto.yml -f docker-compose.platform.yml -f docker-compose.integrationtests.yml up --build`
- ...
- Find test results

# Running integration sets for a specific language

__Note:__ Running these tests in the way described in this document is supposed to be used by contributors only.

## Running integration test suite with JavaScript

- Start the platform in a new terminal, as described in [README.md](../../README.md)
    
    `docker-compose -f ... -f ... up --build`

- Start the Event Tester talent (Utility)

    `node testset_sdk/javascript/eventTester.js`

- Start the Function Provider talent (Utility)

    `node testset_sdk/javascript/functionProvider.js`

- Start the Test Set SDK (Testing features of the platform)

    `node testset_sdk/javascript/testSetSDK.js`

- Run the test runner to execute the actual tests and print the test results

    `node runner/javascript/test_runner.js`
     
- Expected output:

        Talent testRunner-js-4dee4595 started successfully
        Start Integration Tests
        Get Tests for testSet-sdk-js
        Prepare testSet-sdk-js
        Running testSet-sdk-js
        [1/9] Running Test: echoString
         - Result: OK (31ms)
        [2/9] Running Test: echoBoolean
        - Result: OK (30ms)
        ...
        - Result: OK (27ms)
        [9/9] Running Test: receiveEvent1ByMultipleTalents
        - Result: OK (68ms)
        Result of testSet-sdk-js is true
        Overall test result is true


## Running integration test suite with Python

- Start the platform in a new terminal (`docker-compose -f ... -f ... up --build`)
- Install dependencies `pip install -r ..\..\..\..\requirements.dev.txt`
- Build IOTEA module `cd src\sdk\python\src && python3 setup.py egg_info --egg-base ../lib bdist_wheel --dist-dir=../lib clean --all`
- Install IOTEA module `pip3  install  --force-reinstall ../lib/boschio_iotea-0.5.0-py3-none-any.whl`
- Publish event: `node ..\..\..\..\src\tools\mqtt\cli.js pub -c "mqtt://localhost:1883" -t "iotea/ingestion/events" -f "events.txt" --times 1 --delayMs 2000 --transform iotea.ts-now.jna`
- ***TODO*** Start the Event Tester talent - Utilities for test cases
- Start the Function Provider talent (`node testset_sdk/python/function_provider.py`) - Utilities for test cases
- Start the Test Set SDK (`node testset_sdk/python/test_set_sdk.py`) - Testing features of the platform
- Run the test runner (`node runner/python/test_runner.py`) - Will execute the actual tests and print the result
- Test results will be written to `reports/junit-report.xml` (filename may be specified in config.json)

## Running integration test suite with C++

- Start the platform in a new terminal (`docker-compose -f ... -f ... up --build`)
- ***TODO*** Start the Event Tester talent - Utilities for test cases
- Start the Function Provider talent (build and run function_provider.cpp) - Utilities for test cases
- Start the Test Set SDK (build and run testset_sdk.cpp) - Testing features of the platform
- ***TODO*** Run the test runner - Will execute the actual tests and print the result
