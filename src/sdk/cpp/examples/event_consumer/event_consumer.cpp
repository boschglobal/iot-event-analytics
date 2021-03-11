#include <csignal>
#include <initializer_list>
#include <memory>

#include "iotea.hpp"
#include "nlohmann/json.hpp"
#include "logging.hpp"
#include "mqtt_client.hpp"
#include "schema.hpp"

using json = nlohmann::json;

const std::string SERVER_ADDRESS("tcp://localhost:1883");

using namespace iotea::core;

class EventConsumer : public Talent {
   private:
    struct ProviderTalent {
        Callee Multiply;
    } provider_talent;

   public:
    EventConsumer(std::shared_ptr<Publisher> publisher)
        : Talent("event_consumer", publisher) {
        provider_talent.Multiply = CreateCallee("provider_talent", "multiply");
    }

    void OnEvent(const Event& event, EventContext context) override {
        if (event.GetType() == "kuehlschrank") {
            auto args =
                json{event.GetValue().get<int>(), json{{"factor", event.GetValue().get<int>()}, {"unit", "thing"}}};

            provider_talent.Multiply.Call(args, context, [](const json& result, const EventContext& context) {
                log::Info() << "Result: " << result.dump(4);
            });
        } else if (event.GetType() == "blob") {
            log::Info() << "Currently at " << event.GetValue().dump() << " dingdings";
        }
    }

    schema::rules_ptr OnGetRules() const override {
        return OrRules({AndRules({GreaterThan("temp", 3, "kuehlschrank"), LessThan("temp", 10, "kuehlschrank")}),
                        OrRules({IsSet("dingdings", "blob")})});
    }
};

static std::shared_ptr<MqttClient> client = std::make_shared<MqttClient>(SERVER_ADDRESS, "event_consumer");

void signal_handler(int signal) { client->Stop(); }

int main(int argc, char* argv[]) {
    auto talent = std::make_shared<EventConsumer>(client);
    client->RegisterTalent(talent);

    std::signal(SIGINT, signal_handler);

    client->Run();

    return 0;
}