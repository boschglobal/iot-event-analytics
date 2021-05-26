/*****************************************************************************
 * Copyright (c) 2021 Bosch.IO GmbH
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 ****************************************************************************/

#include <chrono>
#include <csignal>
#include <iostream>
#include "nlohmann/json.hpp"

#include "client.hpp"
#include "mqtt_client.hpp"

using json = nlohmann::json;
using iotea::core::FunctionTalent;
using iotea::core::Talent;
using iotea::core::Client;
using iotea::core::Callee;
using iotea::core::event_ctx_ptr;
using iotea::core::Change;
using iotea::core::IsSet;
using iotea::core::OrRules;
using iotea::core::GreaterThan;
using iotea::core::LessThan;
using iotea::core::ErrorMessage;
using iotea::core::Event;
using iotea::core::schema::rule_ptr;

constexpr char SERVER_ADDRESS[] = "tcp://localhost:1883";

static Client client = Client{SERVER_ADDRESS};

class MyService : public Talent {
   public:
    MyService(int events_to_listen_for) : Talent("perf-test"),
        _counter(0),
        _started(false),
        _events_to_listen_for(events_to_listen_for) {
    }

    rule_ptr OnGetRules() const override {
        auto rules = OrRules(
            IsSet("perf-tester.perf_test_start"),
            IsSet("perf-tester.perf_test_end")
        );
        switch (_events_to_listen_for) {
            case 10: rules->Add(IsSet("perf-tester.perf_test_event9"));
            case 9:  rules->Add(IsSet("perf-tester.perf_test_event8"));
            case 8:  rules->Add(IsSet("perf-tester.perf_test_event7"));
            case 7:  rules->Add(IsSet("perf-tester.perf_test_event6"));
            case 6:  rules->Add(IsSet("perf-tester.perf_test_event5"));
            case 5:  rules->Add(IsSet("perf-tester.perf_test_event4"));
            case 4:  rules->Add(IsSet("perf-tester.perf_test_event3"));
            case 3:  rules->Add(IsSet("perf-tester.perf_test_event2"));
            case 2:  rules->Add(IsSet("perf-tester.perf_test_event1"));
            case 1:  rules->Add(IsSet("perf-tester.perf_test_event0"));
        }
        return rules;
    }

    void OnEvent(const Event& event, event_ctx_ptr) override {
        if (event.GetFeature() == "perf-tester.perf_test_start") {
            std::cerr << "Starting perf test" << std::endl;
            _counter = 0;
            _started = true;
            _start_time = std::chrono::system_clock::now();
        } else if (event.GetFeature().find("perf-tester.perf_test_event") == 0) {
            if (_started) {
                _counter++;
                if ((_counter % 100) == 0) {
                std::cerr << "#";
                }
            } else {
                std::cerr << "!";
            }
        } else if (event.GetFeature() == "perf-tester.perf_test_end") {
            std::cerr << std::endl;

            auto duration = std::chrono::system_clock::now() - _start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            std::cerr << "Finished in " << duration_ms << " milliseconds" << std::endl;
            std::cerr << _counter << " events received";
            
            if (duration_ms != 0) {
                auto events_per_second = _counter / ((double)duration_ms / 1000);
                std::cerr << " (" << events_per_second << " / s)" << std::endl;
            }
            _started = false;
            client.Stop();
        }
        // GetLogger().Info() << "Event: " << event.GetValue().dump(4);
    }

    void OnError(const ErrorMessage& msg) override {
        GetLogger().Error() << "Something went a awry, " << msg.GetMessage(); 
    };
  private:
    int64_t _counter;
    std::chrono::system_clock::time_point _start_time;
    bool _started;
    int _events_to_listen_for;
    // std::chrono::system_clock::time_point _end_time;
};

void signal_handler(int) {
    client.Stop();
}

int main(int argc, char* argv[]) {
    int nbr_of_events_to_listen_for = 10;

    if (argc == 2) {
        nbr_of_events_to_listen_for = std::stoi(argv[1]);
    }

    std::cerr << "INPUT: Listen to " << nbr_of_events_to_listen_for << "/10 type of events" << std::endl;
    client.RegisterTalent(std::make_shared<MyService>(nbr_of_events_to_listen_for));

    std::signal(SIGINT, signal_handler);
    client.Start();

    return 0;
}
