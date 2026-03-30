#include "farsounder/config.hpp"

#include <stdexcept>

namespace farsounder::config {
namespace {

std::unordered_map<PubSubMessage, std::int32_t, EnumHash>
default_pubsub_ports() {
    return {
        {PubSubMessage::HydrophoneData, 61501},
        {PubSubMessage::TargetData, 61502},
        {PubSubMessage::RawTargetData, 61505},
        {PubSubMessage::ProcessorSettings, 61503},
        {PubSubMessage::VesselInfo, 61504},
    };
}

std::unordered_map<ReqRepEndpoint, std::int32_t, EnumHash>
default_reqrep_ports() {
    return {
        {ReqRepEndpoint::GetProcessorSettings, 60501},
        {ReqRepEndpoint::SetFieldOfView, 60502},
        {ReqRepEndpoint::SetBottomDetection, 60503},
        {ReqRepEndpoint::SetInWaterSquelch, 60504},
        {ReqRepEndpoint::SetSquelchlessInWaterDetector, 60505},
        {ReqRepEndpoint::GetVesselInfo, 60506},
    };
}

std::unordered_map<RestEndpoint, std::int32_t, EnumHash> default_rest_ports() {
    return {
        {RestEndpoint::GetHistoryData, 3000},
    };
}

std::unordered_map<std::string, PubSubMessage> pubsub_name_map() {
    return {
        {"HydrophoneData", PubSubMessage::HydrophoneData},
        {"TargetData", PubSubMessage::TargetData},
        {"RawTargetData", PubSubMessage::RawTargetData},
        {"ProcessorSettings", PubSubMessage::ProcessorSettings},
        {"VesselInfo", PubSubMessage::VesselInfo},
    };
}

std::unordered_map<std::string, ReqRepEndpoint> reqrep_name_map() {
    return {
        {"GetProcessorSettings", ReqRepEndpoint::GetProcessorSettings},
        {"SetFieldOfView", ReqRepEndpoint::SetFieldOfView},
        {"SetBottomDetection", ReqRepEndpoint::SetBottomDetection},
        {"SetInWaterSquelch", ReqRepEndpoint::SetInWaterSquelch},
        {"SetSquelchlessInWaterDetector",
         ReqRepEndpoint::SetSquelchlessInWaterDetector},
        {"GetVesselInfo", ReqRepEndpoint::GetVesselInfo},
    };
}

std::unordered_map<std::string, RestEndpoint> rest_name_map() {
    return {
        {"GetHistoryData", RestEndpoint::GetHistoryData},
    };
}

template <typename Map>
bool contains_key(const Map& map, const typename Map::key_type& key) {
    return map.find(key) != map.end();
}

}  // namespace

std::string_view to_string(PubSubMessage message) {
    switch (message) {
    case PubSubMessage::HydrophoneData:
        return "HydrophoneData";
    case PubSubMessage::TargetData:
        return "TargetData";
    case PubSubMessage::RawTargetData:
        return "RawTargetData";
    case PubSubMessage::ProcessorSettings:
        return "ProcessorSettings";
    case PubSubMessage::VesselInfo:
        return "VesselInfo";
    }
    return "Unknown";
}

std::string_view to_string(ReqRepEndpoint endpoint) {
    switch (endpoint) {
    case ReqRepEndpoint::GetProcessorSettings:
        return "GetProcessorSettings";
    case ReqRepEndpoint::SetFieldOfView:
        return "SetFieldOfView";
    case ReqRepEndpoint::SetBottomDetection:
        return "SetBottomDetection";
    case ReqRepEndpoint::SetInWaterSquelch:
        return "SetInWaterSquelch";
    case ReqRepEndpoint::SetSquelchlessInWaterDetector:
        return "SetSquelchlessInWaterDetector";
    case ReqRepEndpoint::GetVesselInfo:
        return "GetVesselInfo";
    }
    return "Unknown";
}

std::string_view to_string(RestEndpoint endpoint) {
    switch (endpoint) {
    case RestEndpoint::GetHistoryData:
        return "GetHistoryData";
    }
    return "Unknown";
}

PubSubMessage pubsub_from_name(std::string_view name) {
    auto map = pubsub_name_map();
    auto it = map.find(std::string{name});
    if (it == map.end()) {
        throw std::invalid_argument("Unknown pub-sub message name: " +
                                    std::string{name});
    }
    return it->second;
}

ReqRepEndpoint reqrep_from_name(std::string_view name) {
    auto map = reqrep_name_map();
    auto it = map.find(std::string{name});
    if (it == map.end()) {
        throw std::invalid_argument("Unknown request-reply endpoint name: " +
                                    std::string{name});
    }
    return it->second;
}

RestEndpoint rest_from_name(std::string_view name) {
    auto map = rest_name_map();
    auto it = map.find(std::string{name});
    if (it == map.end()) {
        throw std::invalid_argument("Unknown REST endpoint name: " +
                                    std::string{name});
    }
    return it->second;
}

ClientConfig build_config(
    std::string host, std::vector<PubSubMessage> subscribe,
    std::unordered_map<std::string, std::int32_t> port_overrides,
    CallbackExecutor callback_executor, double timeout_seconds) {
    if (timeout_seconds <= 0.0) {
        throw std::invalid_argument("timeout_seconds must be positive");
    }

    auto pubsub_ports = default_pubsub_ports();
    auto reqrep_ports = default_reqrep_ports();
    auto rest_ports = default_rest_ports();

    if (!port_overrides.empty()) {
        auto pubsub_names = pubsub_name_map();
        auto reqrep_names = reqrep_name_map();
        auto rest_names = rest_name_map();

        for (const auto& [key, port] : port_overrides) {
            if (port <= 0) {
                throw std::invalid_argument("Port override must be positive: " +
                                            key);
            }

            auto pubsub_it = pubsub_names.find(key);
            if (pubsub_it != pubsub_names.end()) {
                pubsub_ports[pubsub_it->second] = port;
                continue;
            }

            auto reqrep_it = reqrep_names.find(key);
            if (reqrep_it != reqrep_names.end()) {
                reqrep_ports[reqrep_it->second] = port;
                continue;
            }

            auto rest_it = rest_names.find(key);
            if (rest_it != rest_names.end()) {
                rest_ports[rest_it->second] = port;
                continue;
            }

            throw std::invalid_argument("Unknown port override key: " + key);
        }
    }

    if (subscribe.empty()) {
        subscribe = {
            PubSubMessage::HydrophoneData, PubSubMessage::TargetData,
            PubSubMessage::RawTargetData,  PubSubMessage::ProcessorSettings,
            PubSubMessage::VesselInfo,
        };
    }

    for (const auto message : subscribe) {
        if (!contains_key(pubsub_ports, message)) {
            throw std::invalid_argument("Unknown pub-sub message type");
        }
    }

    return ClientConfig{
        std::move(host),         std::move(subscribe),  std::move(pubsub_ports),
        std::move(reqrep_ports), std::move(rest_ports), callback_executor,
        timeout_seconds,
    };
}

std::int32_t resolve_pubsub_port(const ClientConfig& config,
                                 PubSubMessage message) {
    auto it = config.pubsub_ports.find(message);
    if (it == config.pubsub_ports.end()) {
        throw std::invalid_argument("Unknown pub-sub message");
    }
    return it->second;
}

std::int32_t resolve_reqrep_port(const ClientConfig& config,
                                 ReqRepEndpoint endpoint) {
    auto it = config.reqrep_ports.find(endpoint);
    if (it == config.reqrep_ports.end()) {
        throw std::invalid_argument("Unknown request-reply endpoint");
    }
    return it->second;
}

std::int32_t resolve_rest_port(const ClientConfig& config,
                               RestEndpoint endpoint) {
    auto it = config.rest_ports.find(endpoint);
    if (it == config.rest_ports.end()) {
        throw std::invalid_argument("Unknown REST endpoint");
    }
    return it->second;
}

}  // namespace farsounder::config
