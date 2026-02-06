#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
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
    std::cout << "Starting FarSounder example..." << '\n';
    std::signal(SIGINT, handle_sigint);

    std::cout << "Building config..." << '\n';
    auto cfg = farsounder::config::build_config(
        "127.0.0.1",
        {
            farsounder::config::PubSubMessage::TargetData,
            farsounder::config::PubSubMessage::ProcessorSettings,
        },
        {}, farsounder::config::CallbackExecutor::ThreadPool, 10.0);

    std::cout << "Create subscriber with our config..." << '\n';
    auto sub = farsounder::subscribe(cfg);

    std::cout << "Registering callback for TargetData..." << '\n';
    sub.on("TargetData", [](const farsounder::TargetData& message) {
        std::cout << "*** Got a TargetData ***" << '\n';
        std::cout << "Target groups: " << message.groups.size() << '\n';
        std::cout << "Bottom bins: " << message.bottom.size() << '\n';
        if (message.heading) {
            std::cout << "Heading: " << std::fixed << std::setprecision(1)
                      << message.heading->degrees << " deg" << '\n';
        }
        if (message.position) {
            std::cout << "Position: " << std::fixed << std::setprecision(6)
                      << message.position->latitude_degrees << ", "
                      << message.position->longitude_degrees << '\n';
        }
        std::cout << '\n';
    });

    std::cout << "Registering callback for ProcessorSettings..." << '\n';
    sub.on("ProcessorSettings",
           [](const farsounder::ProcessorSettings& message) {
               std::cout << "*** Got a ProcessorSettings ***" << '\n';
               std::cout << "Min Inwater Squelch: " << std::fixed
                         << std::setprecision(2)
                         << message.min_inwater_squelch << '\n';
               std::cout << "Max Inwater Squelch: " << std::fixed
                         << std::setprecision(2)
                         << message.max_inwater_squelch << '\n';
               std::cout << "Inwater Squelch: " << std::fixed
                         << std::setprecision(2) << message.inwater_squelch
                         << '\n';
               std::cout << "Squelchless Inwater Detector: "
                         << message.squelchless_inwater_detector << '\n';
               std::cout << "System Type: "
                         << static_cast<int>(message.system_type) << '\n';
               std::cout << "Field Of View: " << static_cast<int>(message.fov)
                         << '\n';
               std::cout << "Detect Bottom: " << message.detect_bottom << '\n';
               std::cout << '\n';
           });

    try {
        std::cout << "*** Requesting processor settings ***" << '\n';
        auto response = farsounder::requests::get_processor_settings(cfg);
        std::cout << "Settings result code: "
                  << static_cast<int>(response.result.code) << '\n';
        if (response.result.code == farsounder::ResultCode::Success) {
            std::cout << "Current FOV: " << static_cast<int>(response.settings.fov)
                      << '\n';
        }
        std::cout << '\n';
    } catch (const std::exception& ex) {
        std::cout << "Request failed: " << ex.what() << '\n';
    }

    try {
        std::cout << "*** Requesting vessel info ***" << '\n';
        auto response = farsounder::requests::get_vessel_info(cfg);
        std::cout << "Vessel info result code: "
                  << static_cast<int>(response.result.code) << '\n';
        if (response.result.code == farsounder::ResultCode::Success) {
            std::cout << "Draft: " << std::fixed << std::setprecision(2)
                      << response.info.draft << " m" << '\n';
            std::cout << "Keel offset: " << std::fixed << std::setprecision(2)
                      << response.info.keel_offset << " m" << '\n';
        }
        std::cout << '\n';
    } catch (const std::exception& ex) {
        std::cout << "Request failed: " << ex.what() << '\n';
    }

    try {
        std::cout << "*** Requesting history data ***" << '\n';
        auto history = farsounder::requests::get_history_data(
            cfg,
            // if you are playing back the Patience Island Survey dataset
            // these coordinates will have data
            41.6538, -71.3712, 500.0);
        std::cout << "History bottom detections: "
                  << history.gridded_bottom_detections.size() << '\n';
        std::cout << "History inwater detections: "
                  << history.gridded_inwater_detections.size() << '\n';
        std::cout << '\n';
    } catch (const std::exception& ex) {
        std::cout << "Request failed: " << ex.what() << '\n';
    }

    std::cout << "*** Running receive loop - press Ctrl+C to exit ***" << '\n';
    sub.start();

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "Stopping subscriber..." << '\n';
    sub.stop();

    std::cout << "Done!" << '\n';
    return 0;
}
