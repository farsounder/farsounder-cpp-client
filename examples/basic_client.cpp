#include <atomic>
#include <chrono>
#include <csignal>
#include <print>
#include <thread>

#include "farsounder/config.hpp"
#include "farsounder/requests.hpp"
#include "farsounder/subscriber.hpp"

namespace {
std::atomic<bool> g_running{true};

void handle_sigint(int) {
    g_running = false;
}
}  // namespace

template <typename T>
int to_int(const T& value) {
    return static_cast<int>(value);
}

int main() {
    std::println("Starting FarSounder example...");
    std::signal(SIGINT, handle_sigint);

    std::println("Building config...");
    auto cfg = farsounder::config::build_config(
        "127.0.0.1",
        {
            farsounder::config::PubSubMessage::TargetData,
            farsounder::config::PubSubMessage::ProcessorSettings,
        },
        {}, farsounder::config::CallbackExecutor::ThreadPool, 10.0);

    std::println("Create subscriber with our config...");
    auto sub = farsounder::subscribe(cfg);

    std::println("Registering callback for TargetData...");
    sub.on("TargetData", [](const proto::nav_api::TargetData& message) {
        std::println("*** Got a TargetData ***");
        std::println("Targets: {}", message.groups_size());
        std::println("Bins: {}", message.bottom_size());
        std::println("");
    });

    std::println("Registering callback for ProcessorSettings...");
    sub.on("ProcessorSettings",
           [](const proto::nav_api::ProcessorSettings& message) {
               std::println("*** Got a ProcessorSettings ***");
               std::println("Min Inwater Squelch: {}",
                            message.min_inwater_squelch());
               std::println("Max Inwater Squelch: {}",
                            message.max_inwater_squelch());
               std::println("Inwater Squelch: {}", message.inwater_squelch());
               std::println("Squelchless Inwater Detector: {}",
                            message.squelchless_inwater_detector());
               std::println("System Type: {}", to_int(message.system_type()));
               std::println("Field Of View: {}", to_int(message.fov()));
               std::println("Detect Bottom: {}", message.detect_bottom());
               std::println("");
           });

    try {
        std::println("*** Requesting processor settings ***");
        auto response = farsounder::requests::get_processor_settings(cfg);
        std::println("Settings result code: {}",
                     to_int(response.result().code()));
        std::println("");
    } catch (const std::exception& ex) {
        std::println("Request failed: {}", ex.what());
    }

    try {
        std::println("*** Requesting history data ***");
        auto history = farsounder::requests::get_history_data(
            cfg,
            // if you are playing back the Patience Island Survey dataset
            // these coordinates will have data
            41.6538, -71.3712, 500.0);
        std::println("History bottom detections: {}",
                     history.gridded_bottom_detections.size());
        std::println("History inwater detections: {}",
                     history.gridded_inwater_detections.size());
        std::println("");
    } catch (const std::exception& ex) {
        std::println("Request failed: {}", ex.what());
    }

    std::println("*** Running receive loop - press Ctrl+C to exit ***");
    sub.start();

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::println("Stopping subscriber...");
    sub.stop();

    std::println("Done!");
    return 0;
}
