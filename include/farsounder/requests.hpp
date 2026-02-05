#pragma once

#include <future>
#include <optional>

#include "farsounder/config.hpp"
#include "farsounder/history.hpp"
#include "farsounder/types.hpp"

namespace farsounder::requests {

// Get current processor settings
GetProcessorSettingsResponse get_processor_settings(
    const config::ClientConfig& config);
std::future<GetProcessorSettingsResponse> get_processor_settings_async(
    const config::ClientConfig& config);

// Set the sonar's field of view mode
SetFieldOfViewResponse set_field_of_view(const config::ClientConfig& config,
                                         FieldOfView fov);
std::future<SetFieldOfViewResponse> set_field_of_view_async(
    const config::ClientConfig& config, FieldOfView fov);

// Enable or disable bottom detection
SetBottomDetectionResponse set_bottom_detection(
    const config::ClientConfig& config, bool enable_bottom_detection);
std::future<SetBottomDetectionResponse> set_bottom_detection_async(
    const config::ClientConfig& config, bool enable_bottom_detection);

// Set the in-water squelch level (only in manual squelch mode)
SetInWaterSquelchResponse set_inwater_squelch(
    const config::ClientConfig& config, float new_squelch_val);
std::future<SetInWaterSquelchResponse> set_inwater_squelch_async(
    const config::ClientConfig& config, float new_squelch_val);

// Toggle between manual and auto squelch modes
SetSquelchlessInWaterDetectorResponse set_squelchless_inwater_detector(
    const config::ClientConfig& config, bool enable_squelchless_detection);
std::future<SetSquelchlessInWaterDetectorResponse>
set_squelchless_inwater_detector_async(const config::ClientConfig& config,
                                       bool enable_squelchless_detection);

// Get vessel info (draft, keel offset)
GetVesselInfoResponse get_vessel_info(const config::ClientConfig& config);
std::future<GetVesselInfoResponse> get_vessel_info_async(
    const config::ClientConfig& config);

// Get historical detection data from the server
history::HistoryData get_history_data(
    const config::ClientConfig& config, double latitude, double longitude,
    double radius_meters = 500.0,
    std::optional<double> since_timestamp_utc = std::nullopt,
    bool tide_corrected_only = false, int skip = 0, int limit = 50000,
    bool include_count = true);

}  // namespace farsounder::requests
