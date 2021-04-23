/********************************************************************
 * Copyright (c) Robert Bosch GmbH
 * All Rights Reserved.
 *
 * This file may not be distributed without the file ’license.txt’.
 * This file is subject to the terms and conditions defined in file
 * ’license.txt’, which is part of this source code package.
 *********************************************************************/

#include <csignal>
#include <iostream>
#include <memory>

#include "nlohmann/json.hpp"
#include "iotea.hpp"
#include "logging.hpp"
#include "mqtt_client.hpp"

using json = nlohmann::json;
using namespace iotea::core;

static constexpr char SERVER_ADDRESS[] = "tcp://mosquitto:1883";
static constexpr char TALENT_ID[] = "functionProvider-cpp";
static constexpr char FUNC_ECHO[] = "echo";

class FunctionProvider : public FunctionTalent {
   public:
    explicit FunctionProvider()
        : FunctionTalent(TALENT_ID) {
        RegisterFunction(FUNC_ECHO, [](const json& args, const CallContext& context) {
            context.Reply(args[0]);
        });
    }
};

static Client client(SERVER_ADDRESS);

void signal_handler(int signal) { client.Stop(); }

int main(int argc, char* argv[]) {
    auto talent = std::make_shared<FunctionProvider>();
    client.RegisterFunctionTalent(talent);

    std::signal(SIGINT, signal_handler);
    client.Start();

    return 0;
}
