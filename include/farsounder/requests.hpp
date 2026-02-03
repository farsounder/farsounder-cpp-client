#pragma once

#include <future>
#include <optional>

#include "farsounder/config.hpp"
#include "farsounder/history.hpp"
#include "proto/nav_api.pb.h"

namespace farsounder::requests {

proto::nav_api::GetProcessorSettingsResponse get_processor_settings(
    const config::ClientConfig& config);
std::future<proto::nav_api::GetProcessorSettingsResponse>
get_processor_settings_async(const config::ClientConfig& config);

proto::nav_api::SetFieldOfViewResponse set_field_of_view(
    const config::ClientConfig& config, proto::nav_api::FieldOfView fov);
std::future<proto::nav_api::SetFieldOfViewResponse> set_field_of_view_async(
    const config::ClientConfig& config, proto::nav_api::FieldOfView fov);

proto::nav_api::SetBottomDetectionResponse set_bottom_detection(
    const config::ClientConfig& config, bool enable_bottom_detection);
std::future<proto::nav_api::SetBottomDetectionResponse>
set_bottom_detection_async(const config::ClientConfig& config,
                           bool enable_bottom_detection);

proto::nav_api::SetInWaterSquelchResponse set_inwater_squelch(
    const config::ClientConfig& config, float new_squelch_val);
std::future<proto::nav_api::SetInWaterSquelchResponse>
set_inwater_squelch_async(const config::ClientConfig& config,
                          float new_squelch_val);

proto::nav_api::SetSquelchlessInWaterDetectorResponse
set_squelchless_inwater_detector(const config::ClientConfig& config,
                                 bool enable_squelchless_detection);
std::future<proto::nav_api::SetSquelchlessInWaterDetectorResponse>
set_squelchless_inwater_detector_async(const config::ClientConfig& config,
                                       bool enable_squelchless_detection);

proto::nav_api::GetVesselInfoResponse get_vessel_info(
    const config::ClientConfig& config);
std::future<proto::nav_api::GetVesselInfoResponse> get_vessel_info_async(
    const config::ClientConfig& config);

history::HistoryData get_history_data(
    const config::ClientConfig& config, double latitude, double longitude,
    double radius_meters = 500.0,
    std::optional<double> since_timestamp_utc = std::nullopt,
    bool tide_corrected_only = false, int skip = 0, int limit = 50000,
    bool include_count = true);

}  // namespace farsounder::requests
