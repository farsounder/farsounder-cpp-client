#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "farsounder/config.hpp"
#include "proto/nav_api.pb.h"

namespace farsounder {

class Subscriber {
   public:
    using HydrophoneCallback =
        std::function<void(const proto::nav_api::HydrophoneData&)>;
    using TargetCallback =
        std::function<void(const proto::nav_api::TargetData&)>;
    using ProcessorSettingsCallback =
        std::function<void(const proto::nav_api::ProcessorSettings&)>;
    using VesselInfoCallback =
        std::function<void(const proto::nav_api::VesselInfo&)>;

    explicit Subscriber(config::ClientConfig config);
    ~Subscriber();

    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;
    Subscriber(Subscriber&&) = delete;
    Subscriber& operator=(Subscriber&&) = delete;

    // TODO(Heath): Template these - make type safe
    void on(config::PubSubMessage message, HydrophoneCallback callback);
    void on(config::PubSubMessage message, TargetCallback callback);
    void on(config::PubSubMessage message, ProcessorSettingsCallback callback);
    void on(config::PubSubMessage message, VesselInfoCallback callback);

    void on(const std::string& message_name, HydrophoneCallback callback);
    void on(const std::string& message_name, TargetCallback callback);
    void on(const std::string& message_name,
            ProcessorSettingsCallback callback);
    void on(const std::string& message_name, VesselInfoCallback callback);

    void start();
    void stop();

   private:
    void run_loop();

    void dispatch_message(const proto::nav_api::HydrophoneData& data);
    void dispatch_message(const proto::nav_api::TargetData& data);
    void dispatch_message(const proto::nav_api::ProcessorSettings& data);
    void dispatch_message(const proto::nav_api::VesselInfo& data);

    config::ClientConfig config_;
    std::thread worker_;
    std::atomic<bool> running_;

    std::mutex callback_mutex_;
    std::vector<HydrophoneCallback> hydrophone_callbacks_;
    std::vector<TargetCallback> target_callbacks_;
    std::vector<ProcessorSettingsCallback> processor_settings_callbacks_;
    std::vector<VesselInfoCallback> vessel_info_callbacks_;
};

Subscriber subscribe(const config::ClientConfig& config);

}  // namespace farsounder
