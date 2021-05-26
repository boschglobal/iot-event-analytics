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

using iotea::core::Talent;
using iotea::core::PlatformEvent;
using iotea::core::Client;
using iotea::core::Callee;
using iotea::core::event_ctx_ptr;
using iotea::core::Change;
using iotea::core::IsSet;
using iotea::core::GreaterThan;
using iotea::core::LessThan;
using iotea::core::ErrorMessage;
using iotea::core::Event;
using iotea::core::schema::rule_ptr;
using iotea::core::schema::Metadata;
using iotea::core::schema::OutputEncoding;

using json = nlohmann::json;
using iotea::core::MqttClient;

constexpr char SERVER_ADDRESS[] = "tcp://localhost:1883";

void run_perf_test(std::string talent_id);

class PerfTester : public Talent {
   public:
    PerfTester() : Talent("perf-tester")
    {
        AddOutput("perf_test_start", Metadata(
            "Start of a perf test",
            0,     // history
            0,     // ttl
            "ONE", // unit
            OutputEncoding(OutputEncoding::Type::String)
        ));
        AddPerfTestOutputEvent(0);
        AddPerfTestOutputEvent(1);
        AddPerfTestOutputEvent(2);
        AddPerfTestOutputEvent(3);
        AddPerfTestOutputEvent(4);
        AddPerfTestOutputEvent(5);
        AddPerfTestOutputEvent(6);
        AddPerfTestOutputEvent(7);
        AddPerfTestOutputEvent(8);
        AddPerfTestOutputEvent(9);
        AddOutput("perf_test_end", Metadata(
            "End of a perf test",
            0,     // history
            0,     // ttl
            "ONE", // unit
            OutputEncoding(OutputEncoding::Type::String)
        ));

    }

    ~PerfTester()
    {
        if (_perf_test_thread.joinable()) {
            _perf_test_thread.join();
        }
    }

    void AddPerfTestOutputEvent(int index) {
        auto index_str = std::to_string(index);
        AddOutput("perf_test_event" + index_str, Metadata(
            "Perf test event" + index_str,
            0,     // history
            0,     // ttl
            "ONE", // unit
            OutputEncoding(OutputEncoding::Type::Number)
        ));
    }

    rule_ptr OnGetRules() const override {
        return Change("anyfeature", "anytype");
    }

    void SetOnline(const std::string &name, bool online) {
        if (name == GetId()) {
            if (online && !_we_are_online) {
                _we_are_online = true;
                std::cerr << "we are online" << std::endl;
                if (_receiver_is_online) {
                    // Start test
                    _perf_test_thread = std::thread(run_perf_test, GetId());
                }
            }
            if (!online && _we_are_online) {
                std::cerr << "we went offline" << std::endl;
                return;
            }
        }
        else if (name == "perf-test") {
            if (online && !_receiver_is_online) {
                _receiver_is_online = true;
                std::cerr << "receiver came online" << std::endl;
                if (_we_are_online) {
                    // Start test
                    _perf_test_thread = std::thread(run_perf_test, GetId());
                }
            }
            if (!online && _receiver_is_online) {
                _receiver_is_online = false;
                std::cerr << "receiver went offline" << std::endl;
                return;
            }
        }
    }

    void SetOffline(const std::string &name, bool online) {

    }

    void OnPlatformEvent(const PlatformEvent& event) override
    {
        auto type = event.GetType();

        switch (type) {
            default:
            return;
            case PlatformEvent::Type::TALENT_RULES_SET:
            {
                auto talent = event.GetData()["talent"].get<std::string>();
                SetOnline(talent, true);
                break;
            }
            case PlatformEvent::Type::TALENT_RULES_UNSET:
            {
                auto talent = event.GetData()["talent"].get<std::string>();
                SetOnline(talent, false);
                break;
            }
        }


    }

    void OnError(const ErrorMessage& msg) override
    {
        GetLogger().Error() << "Something went a awry, " << msg.GetMessage(); 
    };

  private:
    int _counter;
    std::chrono::system_clock::time_point _start_time;
    std::thread _perf_test_thread;
    bool _we_are_online;
    bool _receiver_is_online;
};

static Client client = Client{SERVER_ADDRESS};

void signal_handler(int) {
    client.Stop();
}

int number_of_hundreds = 100;

void run_perf_test(std::string talent_id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cerr << "Running perf test" << std::endl;
    auto epoch = std::chrono::system_clock::now().time_since_epoch();
    auto start_time = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    int64_t counter = 0;

    json data = {
        {"feature", talent_id + ".perf_test_start"},
        // {"type", "default"},
        {"subject", "perf_test"},
        {"instance", "perf_tester"},
        {"whenMs", start_time},
        {"value", "GO"}
    };

    client.Publish("iotea/ingestion/events", data.dump());

    for (int k=0; k<number_of_hundreds; k++) {
        for (int j=0; j<10; j++) {
            for (int i=0; i<10; i++) {
                auto epoch = std::chrono::system_clock::now().time_since_epoch();
                auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

                json data = {
                    {"feature", talent_id + ".perf_test_event" + std::to_string(i)},
                    // {"type", "default"},
                    {"subject", "perf_test"},
                    {"instance", "perf_tester"},
                    {"whenMs", timestamp},
                    {"value", i}

                };

                int backoff_ms = 1;
                bool success = false;

                while (!success) {
                    try {
                        client.Publish("iotea/ingestion/events", data.dump());
                        success = true;
                    } catch (mqtt::exception e) {
                        if (e.get_return_code() == -12) {
                            std::cerr << "..waiting for internal buffer..";
                        } else {
                            std::cerr << e.what() << std::endl;
                            std::cerr << "reason_code: " << e.get_reason_code() << std::endl;
                            std::cerr << "return_code: " << e.get_return_code() << std::endl;
                            std::cerr << "retrying.." << std::endl;
                        }
    
                        backoff_ms++;                        
                        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                    }
                }

                counter++;
            }
        }

        std::cerr << "#"; // Every one hundred messages
    }

    epoch = std::chrono::system_clock::now().time_since_epoch();
    auto end_time = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    data = {
        {"feature", talent_id + ".perf_test_end"},
        // {"type", "default"},
        {"subject", "perf_test"},
        {"instance", "perf_tester"},
        {"whenMs", end_time},
        {"value", "STOP"}
    };

    client.Publish("iotea/ingestion/events", data.dump());

    std::cerr << std::endl << "Stopping..." << std::endl;

    auto duration_ms = end_time - start_time;
    std::cerr << "Finished in " << duration_ms << " milliseconds" << std::endl;
    std::cerr << counter << " events sent";
    
    if (duration_ms != 0) {
        auto events_per_second = counter / ((double)duration_ms / 1000);
        std::cerr << " (" << events_per_second << " / s)" << std::endl;
    }
    client.Stop();
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        number_of_hundreds = std::stoi(argv[1]) / 100;
    }

    std::cerr << "INPUT: Send " << number_of_hundreds * 100 << " events" << std::endl;

    std::signal(SIGINT, signal_handler);

    auto talent = std::make_shared<PerfTester>();
    client.RegisterTalent(talent);

    client.Start();

    return 0;
}
