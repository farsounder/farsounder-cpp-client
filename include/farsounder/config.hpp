#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace farsounder::config {

enum class PubSubMessage {
    HydrophoneData,
    TargetData,
    RawTargetData,
    ProcessorSettings,
    VesselInfo,
};

enum class ReqRepEndpoint {
    GetProcessorSettings,
    SetFieldOfView,
    SetBottomDetection,
    SetInWaterSquelch,
    SetSquelchlessInWaterDetector,
    GetVesselInfo,
};

enum class RestEndpoint {
    GetHistoryData,
};

enum class CallbackExecutor {
    ThreadPool,
    Inline,
};

struct EnumHash {
    template <typename T>
    std::size_t operator()(T value) const noexcept {
        return static_cast<std::size_t>(value);
    }
};

struct ClientConfig {
    std::string host;
    std::vector<PubSubMessage> subscribe;
    std::unordered_map<PubSubMessage, std::int32_t, EnumHash> pubsub_ports;
    std::unordered_map<ReqRepEndpoint, std::int32_t, EnumHash> reqrep_ports;
    std::unordered_map<RestEndpoint, std::int32_t, EnumHash> rest_ports;
    CallbackExecutor callback_executor;
    double timeout_seconds;
};

std::string_view to_string(PubSubMessage message);
std::string_view to_string(ReqRepEndpoint endpoint);
std::string_view to_string(RestEndpoint endpoint);

ClientConfig build_config(
    std::string host = "127.0.0.1", std::vector<PubSubMessage> subscribe = {},
    std::unordered_map<std::string, std::int32_t> port_overrides = {},
    CallbackExecutor callback_executor = CallbackExecutor::ThreadPool,
    double timeout_seconds = 2.0);

std::int32_t resolve_pubsub_port(const ClientConfig& config,
                                 PubSubMessage message);
std::int32_t resolve_reqrep_port(const ClientConfig& config,
                                 ReqRepEndpoint endpoint);
std::int32_t resolve_rest_port(const ClientConfig& config,
                               RestEndpoint endpoint);

PubSubMessage pubsub_from_name(std::string_view name);
ReqRepEndpoint reqrep_from_name(std::string_view name);
RestEndpoint rest_from_name(std::string_view name);

}  // namespace farsounder::config
