<!---
  Copyright (c) 2021 Bosch.IO GmbH

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at https://mozilla.org/MPL/2.0/.

  SPDX-License-Identifier: MPL-2.0
-->

# IoT Event Analytics - IoTea

Bosch.IO GmbH
under [MPL-2.0 licence](https://choosealicense.com/licenses/mpl-2.0/)

![Image of IoTea](./docs/assets/iotea.jpg)

## Project structure

```code
src
| |- core      The platform itself
| |- sdk       The platform software development kits
| |- adapter   Integrations of other systems
|- resources   JSON schemas for object validations
L- docs        Documentation
```

## Prerequisites

### NodeJS

- Have NodeJS >=12.13.0 installed
- Install all dependencies using yarn package manager
  - If yarn is not available on your system, install it using `npm install -g yarn`
  - Run `yarn` in the project root

### Python

- If using Anaconda to manage your Python environments, create a new one
  - `conda create --name <name-of-choice> python=3.7`
    - Python needs to be at at version >=3.6
    - Pick a name of choice
  - `conda activate <name-of-choice>`
- Install all necessary packages using `pip install -r requirements.dev.txt` in the project root

## Build

The recommended way of quickstarting the platform is to use docker-compose where available. If not installed, all components can be built manually using the given build instructions under _./docker/*_

### >> @Corprate-Proxy only <<

If you are working behind a corporate proxy, make sure you have a local non-auth proxy running on port 3128 to be able to connect to the internet without any further manual authentication

- Go into the _./docker-compose_ subdirectory
- Run `docker-compose -f docker-compose.platform.yml build`

## Configure

The platform will spin up together with Mosquitto MQTT brokers

- Run `docker-compose -f docker-compose/docker-compose.platform.yml up`

- __Important:__ If you want to specify an alternative configuration directory for the IoTea Platform and/or the MQTT Broker, you can create alternative configuration folders in a different directory<br>
Your configuration directories should look like this

  ```code
  .env                           Copy from docker-compose/.env

  <mosquitto configuration dir>  Copy from docker-compose/mosquitto
  |- remote
  |  L- config.json
  L- config.json

  <platform configuration dir>   Copy from docker-compose/platform
  |- channels
  |  |- talent.channel.json
  |  L- talent.schema.json
  |- config.json
  |- types.json
  L- uom.json
  ```

  You can now adapt the _types.json_ document to fit your needs. Do not make any changes but the `loglevel` to the _config.json_ when you are using `docker_compose`

The _.env_ file should contain the following

```code
DOCKER_HTTP_PROXY=http://docker.for.win.localhost:3128   (Proxy configuration)
DOCKER_HTTPS_PROXY=http://docker.for.win.localhost:3128  (Proxy configuration)
MOSQUITTO_CONFIG_DIR=                                    (Path to you mosquitto configuration folder - relative from docker-compose directory or absolute path)
PLATFORM_CONFIG_DIR=                                     (Path to you platform configuration folder - relative from docker-compose directory or absolute path. Not needed if you only want to configure Mosquitto)
```

## Run the configured platform

`docker-compose -f docker-compose.platform.yml --env-file <relative path to your env-file from docker-compose folder or absolute path> up --remove-orphans`

## Run a talent on your machine

- Simply create your first python or NodeJS talent following the examples given in _src/sdk/(javascript|python|cpp)/examples<br>
- Start you talent by connecting it to `mqtt://localhost:1883` or a locally running remote talent by connecting it to `mqtt://localhost:1884`.<br>
- There are examples, which start a platform instance by themselves. They only need an MQTT Broker running. To achieve this, simply run<br>
`docker-compose -f docker-compose.mosquitto.yml up`

## Run a talent as AWS Lambda function

You have to provide a custom MQTT configuration as mentioned above to configure the certificate based authentication and the remote broker. You can see a preconfigured example [here](./src/sdk/javascript/examples/console/cloud/config/mosquitto/config.json)

## Dependencies

- List all NodeJS dependencies on stdout `yarn licenses list`
- List all Python dependencies on stdout `pip-licenses --from=mixed`
  - To have a correct list of packages containing all subpackages needed by this project, make sure that you used a "fresh" environment of conda, where you install the dependencies using `pip install -r requirements.dev.txt`. Otherwise pip will list all packages you have installed in your environment.
