#include <atomic>
#include <chrono>
#include <csignal>
#include <print>
#include <thread>

#include "farsounder/config.hpp"
#include "farsounder/requests.hpp"
#include "farsounder/subscriber.hpp"
#include "farsounder/types.hpp"

namespace {
std::atomic<bool> g_running{true};

void handle_sigint(int) {
    g_running = false;
}
}  // namespace

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
    sub.on("TargetData", [](const farsounder::TargetData& message) {
        std::println("*** Got a TargetData ***");
        std::println("Target groups: {}", message.groups.size());
        std::println("Bottom bins: {}", message.bottom.size());
        if (message.heading) {
            std::println("Heading: {:.1f} deg", message.heading->degrees);
        }
        if (message.position) {
            std::println("Position: {:.6f}, {:.6f}",
                         message.position->latitude_degrees,
                         message.position->longitude_degrees);
        }
        std::println("");
    });

    std::println("Registering callback for ProcessorSettings...");
    sub.on("ProcessorSettings",
           [](const farsounder::ProcessorSettings& message) {
               std::println("*** Got a ProcessorSettings ***");
               std::println("Min Inwater Squelch: {:.2f}",
                            message.min_inwater_squelch);
               std::println("Max Inwater Squelch: {:.2f}",
                            message.max_inwater_squelch);
               std::println("Inwater Squelch: {:.2f}", message.inwater_squelch);
               std::println("Squelchless Inwater Detector: {}",
                            message.squelchless_inwater_detector);
               std::println("System Type: {}",
                            static_cast<int>(message.system_type));
               std::println("Field Of View: {}",
                            static_cast<int>(message.fov));
               std::println("Detect Bottom: {}", message.detect_bottom);
               std::println("");
           });

    try {
        std::println("*** Requesting processor settings ***");
        auto response = farsounder::requests::get_processor_settings(cfg);
        std::println("Settings result code: {}",
                     static_cast<int>(response.result.code));
        if (response.result.code == farsounder::ResultCode::Success) {
            std::println("Current FOV: {}",
                         static_cast<int>(response.settings.fov));
        }
        std::println("");
    } catch (const std::exception& ex) {
        std::println("Request failed: {}", ex.what());
    }

    try {
        std::println("*** Requesting vessel info ***");
        auto response = farsounder::requests::get_vessel_info(cfg);
        std::println("Vessel info result code: {}",
                     static_cast<int>(response.result.code));
        if (response.result.code == farsounder::ResultCode::Success) {
            std::println("Draft: {:.2f} m", response.info.draft);
            std::println("Keel offset: {:.2f} m", response.info.keel_offset);
        }
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
