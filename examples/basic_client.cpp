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
const std::string kDefaultHost = "127.0.0.1";
const std::string kHelpFlag = "--help";
// notes on running the client in WSL:
// https://learn.microsoft.com/en-us/windows/wsl/networking#identify-ip-address (scenario 2) 
std::atomic<bool> g_running{true};

void handle_sigint(int) {
    g_running = false;
}
}  // namespace

int main(
    int argc, char** argv
) {

    if (argc > 2 || (argc > 1 && std::string(argv[1]) == kHelpFlag)) {
        std::cout << "Usage: " << argv[0] << " [host]" << '\n';
        std::cout << "  host: the host to connect to (default: " << kDefaultHost << ")" << std::endl;
        std::cout << "  This should point at the machine running the" << std::endl
                  << "  SonaSoft demo or SonaSoft nav software." << std::endl;
        std::cout << "  On WSL, you can use the ip address of the WSL host." << std::endl;
        std::cout << "  One way to get this ip is to run this inside the WSL terminal:" << std::endl;
        std::cout << "    $ ip route | awk '/default/ {print $3}'" << std::endl;
        std::cout << "  For example, if the ip address is 172.30.64.1, you can use:" << std::endl;
        std::cout << "    " << argv[0] << " 172.30.64.1" << std::endl;
        return 1;
    }

    std::string host = kDefaultHost;
    if (argc > 1) {
        host = argv[1];
    }

    std::cout << "Starting FarSounder example..." << '\n';
    std::cout << "  connecting to host: " << host << '\n';

    std::signal(SIGINT, handle_sigint);

    std::cout << "Building config..." << '\n';
    auto cfg = farsounder::config::build_config(
        host,
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
        if (message.grid_description) {
            std::cout << "Grid shape: "
                      << message.grid_description->hor_angles.size() << " x "
                      << message.grid_description->ver_angles.size() << '\n';
            std::cout << "Grid max range: " << std::fixed
                      << std::setprecision(1)
                      << message.grid_description->max_range << " m" << '\n';
        }
        if (!message.groups.empty() && !message.groups.front().bins.empty()) {
            const auto& bin = message.groups.front().bins.front();
            std::cout << "First target bin: hor=" << bin.hor_index
                      << " ver=" << bin.ver_index
                      << " range=" << bin.range_index << " cross=" << std::fixed
                      << std::setprecision(2) << bin.cross_range
                      << " down=" << bin.down_range << " depth=" << bin.depth
                      << " strength=" << bin.strength << '\n';
        }
        if (!message.bottom.empty()) {
            const auto& bin = message.bottom.front();
            std::cout << "First bottom bin: hor=" << bin.hor_index
                      << " ver=" << bin.ver_index
                      << " range=" << bin.range_index << " cross=" << std::fixed
                      << std::setprecision(2) << bin.cross_range
                      << " down=" << bin.down_range << " depth=" << bin.depth
                      << " strength=" << bin.strength << '\n';
        }
        std::cout << '\n';
    });

    std::cout << "Registering callback for ProcessorSettings..." << '\n';
    sub.on("ProcessorSettings",
           [](const farsounder::ProcessorSettings& message) {
               std::cout << "*** Got a ProcessorSettings ***" << '\n';
               std::cout << "Min Inwater Squelch: " << std::fixed
                         << std::setprecision(2) << message.min_inwater_squelch
                         << '\n';
               std::cout << "Max Inwater Squelch: " << std::fixed
                         << std::setprecision(2) << message.max_inwater_squelch
                         << '\n';
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
            std::cout << "Current FOV: "
                      << static_cast<int>(response.settings.fov) << '\n';
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
