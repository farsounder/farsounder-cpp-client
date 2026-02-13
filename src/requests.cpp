#include "farsounder/requests.hpp"

#include <cpr/cpr.h>

#include <future>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <zmq.hpp>

#include "conversions_internal.hpp"
#include "proto/nav_api.pb.h"
#include "requests_internal.hpp"

namespace farsounder::requests {
namespace {

std::string endpoint_address(const config::ClientConfig& cfg,
                             config::ReqRepEndpoint endpoint) {
    auto port = config::resolve_reqrep_port(cfg, endpoint);
    return "tcp://" + cfg.host + ":" + std::to_string(port);
}

int to_timeout_ms(double timeout_seconds) {
    auto ms = static_cast<int>(timeout_seconds * 1000.0);
    return ms > 0 ? ms : 1;
}

template <typename Request, typename Response>
Response send_request(const config::ClientConfig& cfg,
                      config::ReqRepEndpoint endpoint, const Request& request) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);

    const int timeout_ms = to_timeout_ms(cfg.timeout_seconds);
    socket.set(zmq::sockopt::rcvtimeo, timeout_ms);
    socket.set(zmq::sockopt::sndtimeo, timeout_ms);
    socket.set(zmq::sockopt::linger, 0);

    socket.connect(endpoint_address(cfg, endpoint));

    std::string payload;
    if (!request.SerializeToString(&payload)) {
        throw std::runtime_error("Failed to serialize request");
    }

    zmq::message_t request_msg(payload.begin(), payload.end());
    auto send_result = socket.send(request_msg, zmq::send_flags::none);
    if (!send_result) {
        throw std::runtime_error("Failed to send request");
    }

    zmq::message_t reply;
    auto recv_result = socket.recv(reply, zmq::recv_flags::none);
    if (!recv_result) {
        throw std::runtime_error("Timed out waiting for response");
    }

    Response response;
    if (!response.ParseFromArray(reply.data(),
                                 static_cast<int>(reply.size()))) {
        throw std::runtime_error("Failed to parse response");
    }

    return response;
}

std::string rest_base_url(const config::ClientConfig& cfg) {
    auto port =
        config::resolve_rest_port(cfg, config::RestEndpoint::GetHistoryData);
    return "http://" + cfg.host + ":" + std::to_string(port);
}

}  // namespace

namespace detail {

// =============================================================================
// History data parsing (REST/JSON)
// =============================================================================

history::GriddedBottomDetection parse_gridded_bottom_detection(
    const nlohmann::json& payload) {
    history::GriddedBottomDetection item;
    item.timestamp_utc = payload.at("timestamp_utc").get<double>();
    item.latitude_degrees = payload.at("latitude_degrees").get<double>();
    item.longitude_degrees = payload.at("longitude_degrees").get<double>();
    item.grid_interval_meters =
        payload.at("grid_interval_meters").get<double>();
    item.target_strength_db = payload.at("target_strength_db").get<double>();
    item.is_tide_corrected = payload.at("is_tide_corrected").get<bool>();
    item.uploaded_to_cloud = payload.at("uploaded_to_cloud").get<bool>();
    item.number_of_points = payload.at("number_of_points").get<std::int32_t>();
    item.depth_meters = payload.at("depth_meters").get<double>();
    return item;
}

history::GriddedInwaterDetection parse_gridded_inwater_detection(
    const nlohmann::json& payload) {
    history::GriddedInwaterDetection item;
    item.timestamp_utc = payload.at("timestamp_utc").get<double>();
    item.latitude_degrees = payload.at("latitude_degrees").get<double>();
    item.longitude_degrees = payload.at("longitude_degrees").get<double>();
    item.grid_interval_meters =
        payload.at("grid_interval_meters").get<double>();
    item.target_strength_db = payload.at("target_strength_db").get<double>();
    item.is_tide_corrected = payload.at("is_tide_corrected").get<bool>();
    item.uploaded_to_cloud = payload.at("uploaded_to_cloud").get<bool>();
    item.number_of_points = payload.at("number_of_points").get<std::int32_t>();
    if (payload.contains("deepest_depth_meters")) {
        item.deepest_depth_meters =
            payload.at("deepest_depth_meters").get<double>();
    } else {
        item.deepest_depth_meters = std::nullopt;
    }
    if (payload.contains("shallowest_depth_meters")) {
        item.shallowest_depth_meters =
            payload.at("shallowest_depth_meters").get<double>();
    } else {
        item.shallowest_depth_meters = std::nullopt;
    }
    return item;
}

history::HistoryData parse_history_data(const nlohmann::json& payload) {
    history::HistoryData data;
    if (payload.contains("gridded_bottom_detections")) {
        for (const auto& item : payload.at("gridded_bottom_detections")) {
            data.gridded_bottom_detections.push_back(
                parse_gridded_bottom_detection(item));
        }
    }
    if (payload.contains("gridded_inwater_detections")) {
        for (const auto& item : payload.at("gridded_inwater_detections")) {
            data.gridded_inwater_detections.push_back(
                parse_gridded_inwater_detection(item));
        }
    }
    return data;
}

}  // namespace detail

// =============================================================================
// Public API implementations
// =============================================================================

GetProcessorSettingsResponse get_processor_settings(
    const config::ClientConfig& config) {
    proto::nav_api::GetProcessorSettingsRequest request;
    auto proto_response =
        send_request<proto::nav_api::GetProcessorSettingsRequest,
                     proto::nav_api::GetProcessorSettingsResponse>(
            config, config::ReqRepEndpoint::GetProcessorSettings, request);

    GetProcessorSettingsResponse response;
    response.result = farsounder::detail::convert_result(proto_response.result());
    response.settings =
        farsounder::detail::convert_processor_settings(proto_response.settings());
    return response;
}

std::future<GetProcessorSettingsResponse> get_processor_settings_async(
    const config::ClientConfig& config) {
    return std::async(std::launch::async,
                      [config]() { return get_processor_settings(config); });
}

SetFieldOfViewResponse set_field_of_view(const config::ClientConfig& config,
                                         FieldOfView fov) {
    proto::nav_api::SetFieldOfViewRequest request;
    request.set_fov(farsounder::detail::convert_fov_to_proto(fov));
    auto proto_response = send_request<proto::nav_api::SetFieldOfViewRequest,
                                       proto::nav_api::SetFieldOfViewResponse>(
        config, config::ReqRepEndpoint::SetFieldOfView, request);

    SetFieldOfViewResponse response;
    response.result = farsounder::detail::convert_result(proto_response.result());
    return response;
}

std::future<SetFieldOfViewResponse> set_field_of_view_async(
    const config::ClientConfig& config, FieldOfView fov) {
    return std::async(std::launch::async, [config, fov]() {
        return set_field_of_view(config, fov);
    });
}

SetBottomDetectionResponse set_bottom_detection(
    const config::ClientConfig& config, bool enable_bottom_detection) {
    proto::nav_api::SetBottomDetectionRequest request;
    request.set_enable_bottom_detection(enable_bottom_detection);
    auto proto_response =
        send_request<proto::nav_api::SetBottomDetectionRequest,
                     proto::nav_api::SetBottomDetectionResponse>(
            config, config::ReqRepEndpoint::SetBottomDetection, request);

    SetBottomDetectionResponse response;
    response.result = farsounder::detail::convert_result(proto_response.result());
    return response;
}

std::future<SetBottomDetectionResponse> set_bottom_detection_async(
    const config::ClientConfig& config, bool enable_bottom_detection) {
    return std::async(std::launch::async, [config, enable_bottom_detection]() {
        return set_bottom_detection(config, enable_bottom_detection);
    });
}

SetInWaterSquelchResponse set_inwater_squelch(
    const config::ClientConfig& config, float new_squelch_val) {
    proto::nav_api::SetInWaterSquelchRequest request;
    request.set_new_squelch_val(new_squelch_val);
    auto proto_response =
        send_request<proto::nav_api::SetInWaterSquelchRequest,
                     proto::nav_api::SetInWaterSquelchResponse>(
            config, config::ReqRepEndpoint::SetInWaterSquelch, request);

    SetInWaterSquelchResponse response;
    response.result = farsounder::detail::convert_result(proto_response.result());
    return response;
}

std::future<SetInWaterSquelchResponse> set_inwater_squelch_async(
    const config::ClientConfig& config, float new_squelch_val) {
    return std::async(std::launch::async, [config, new_squelch_val]() {
        return set_inwater_squelch(config, new_squelch_val);
    });
}

SetSquelchlessInWaterDetectorResponse set_squelchless_inwater_detector(
    const config::ClientConfig& config, bool enable_squelchless_detection) {
    proto::nav_api::SetSquelchlessInWaterDetectorRequest request;
    request.set_enable_squelchless_detection(enable_squelchless_detection);
    auto proto_response =
        send_request<proto::nav_api::SetSquelchlessInWaterDetectorRequest,
                     proto::nav_api::SetSquelchlessInWaterDetectorResponse>(
            config, config::ReqRepEndpoint::SetSquelchlessInWaterDetector,
            request);

    SetSquelchlessInWaterDetectorResponse response;
    response.result = farsounder::detail::convert_result(proto_response.result());
    return response;
}

std::future<SetSquelchlessInWaterDetectorResponse>
set_squelchless_inwater_detector_async(const config::ClientConfig& config,
                                       bool enable_squelchless_detection) {
    return std::async(std::launch::async,
                      [config, enable_squelchless_detection]() {
                          return set_squelchless_inwater_detector(
                              config, enable_squelchless_detection);
                      });
}

GetVesselInfoResponse get_vessel_info(const config::ClientConfig& config) {
    proto::nav_api::GetVesselInfoRequest request;
    auto proto_response = send_request<proto::nav_api::GetVesselInfoRequest,
                                       proto::nav_api::GetVesselInfoResponse>(
        config, config::ReqRepEndpoint::GetVesselInfo, request);

    GetVesselInfoResponse response;
    response.result = farsounder::detail::convert_result(proto_response.result());
    response.info = farsounder::detail::convert_vessel_info(proto_response.info());
    return response;
}

std::future<GetVesselInfoResponse> get_vessel_info_async(
    const config::ClientConfig& config) {
    return std::async(std::launch::async,
                      [config]() { return get_vessel_info(config); });
}

history::HistoryData get_history_data(const config::ClientConfig& config,
                                      double latitude, double longitude,
                                      double radius_meters,
                                      std::optional<double> since_timestamp_utc,
                                      bool tide_corrected_only, int skip,
                                      int limit, bool include_count) {
    const auto url = rest_base_url(config) + "/api/history_data";

    cpr::Parameters params{
        {"latitude", std::to_string(latitude)},
        {"longitude", std::to_string(longitude)},
        {"radius_meters", std::to_string(radius_meters)},
        {"tide_corrected_only", tide_corrected_only ? "true" : "false"},
        {"skip", std::to_string(skip)},
        {"limit", std::to_string(limit)},
        {"include_count", include_count ? "true" : "false"}};

    if (since_timestamp_utc.has_value()) {
        params.Add(
            {"since_timestamp_utc", std::to_string(*since_timestamp_utc)});
    }

    const auto timeout_ms = to_timeout_ms(config.timeout_seconds);
    auto response = cpr::Get(cpr::Url{url}, params, cpr::Timeout{timeout_ms});

    if (response.error) {
        throw std::runtime_error("REST request failed: " +
                                 response.error.message);
    }
    if (response.status_code != 200) {
        throw std::runtime_error("REST request failed with status " +
                                 std::to_string(response.status_code) + ": " +
                                 response.text);
    }

    auto payload = nlohmann::json::parse(response.text);
    return detail::parse_history_data(payload);
}

}  // namespace farsounder::requests
