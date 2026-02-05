#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "farsounder/config.hpp"
#include "farsounder/types.hpp"

namespace farsounder {

class Subscriber {
   public:
    using HydrophoneCallback = std::function<void(const HydrophoneData&)>;
    using TargetCallback = std::function<void(const TargetData&)>;
    using ProcessorSettingsCallback =
        std::function<void(const ProcessorSettings&)>;
    using VesselInfoCallback = std::function<void(const VesselInfo&)>;

    explicit Subscriber(config::ClientConfig config);
    ~Subscriber();

    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;
    Subscriber(Subscriber&&) noexcept;
    Subscriber& operator=(Subscriber&&) noexcept;

    // Register callbacks by message type enum
    void on(config::PubSubMessage message, HydrophoneCallback callback);
    void on(config::PubSubMessage message, TargetCallback callback);
    void on(config::PubSubMessage message, ProcessorSettingsCallback callback);
    void on(config::PubSubMessage message, VesselInfoCallback callback);

    // Register callbacks by message name string
    void on(const std::string& message_name, HydrophoneCallback callback);
    void on(const std::string& message_name, TargetCallback callback);
    void on(const std::string& message_name,
            ProcessorSettingsCallback callback);
    void on(const std::string& message_name, VesselInfoCallback callback);

    void start();
    void stop();

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

Subscriber subscribe(const config::ClientConfig& config);

}  // namespace farsounder
