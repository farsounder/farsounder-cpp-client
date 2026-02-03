#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
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

int main() {
    std::cout << "Starting FarSounder example..." << std::endl;
    std::signal(SIGINT, handle_sigint);

    std::cout << "Building config..." << std::endl;
    auto cfg = farsounder::config::build_config(
        "127.0.0.1",
        {
            farsounder::config::PubSubMessage::TargetData,
            farsounder::config::PubSubMessage::ProcessorSettings,
        },
        {}, farsounder::config::CallbackExecutor::ThreadPool, 10.0);

    std::cout << "Subscribing to TargetData..." << std::endl;
    auto sub = farsounder::subscribe(cfg);

    std::cout << "Registering callback for TargetData..." << std::endl;
    sub.on("TargetData", [](const proto::nav_api::TargetData& message) {
        std::cout << "Got a TargetData!\n";
        std::cout << "Targets: " << message.groups_size() << "\n";
    });

    std::cout << "Registering callback for ProcessorSettings..." << std::endl;
    sub.on("ProcessorSettings",
           [](const proto::nav_api::ProcessorSettings& message) {
               std::cout << "Got a ProcessorSettings!\n";
               std::cout << "Min Inwater Squelch: "
                         << message.min_inwater_squelch() << "\n";
               std::cout << "Max Inwater Squelch: "
                         << message.max_inwater_squelch() << "\n";
               std::cout << "Inwater Squelch: " << message.inwater_squelch()
                         << "\n";
               std::cout << "Squelchless Inwater Detector: "
                         << message.squelchless_inwater_detector() << "\n";
               std::cout << "System Type: " << message.system_type() << "\n";
               std::cout << "Field Of View: " << message.fov() << "\n";
               std::cout << "Detect Bottom: " << message.detect_bottom()
                         << "\n";
           });

    std::cout << "Starting subscriber..." << std::endl;
    sub.start();

    try {
        std::cout << "Requesting processor settings..." << std::endl;
        auto response = farsounder::requests::get_processor_settings(cfg);
        std::cout << "Settings result code: " << response.result().code()
                  << "\n";
        std::cout << "Settings: " << response.settings().DebugString() << "\n";

        std::cout << "Requesting history data..." << std::endl;
        auto history = farsounder::requests::get_history_data(
            cfg,
            // if you are playing back the Patience Island Survey dataset
            // these coordinates will have data
            41.6538, -71.3712, 500.0);
        std::cout << "History bottom detections: "
                  << history.gridded_bottom_detections.size() << "\n";
        std::cout << "History inwater detections: "
                  << history.gridded_inwater_detections.size() << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Request failed: " << ex.what() << "\n";
    }

    std::cout << "Running. Press Ctrl+C to exit." << std::endl;
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "Stopping subscriber..." << std::endl;
    sub.stop();

    std::cout << "Done!" << std::endl;
    return 0;
}
