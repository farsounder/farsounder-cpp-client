#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace farsounder {

enum class ResultCode {
    Success = 0,
    UnknownError = 1,
    OperationUnavailable = 2,
    ParameterOutOfRange = 3,
    ParameterMissing = 4,
    InvalidRequest = 5,
};

enum class FieldOfView {
    // Argos-500 & Argos-1000 systems
    k120d100m = 5,  // 120 degree sector out to 100 meters
    k120d200m = 6,  // 120 degree sector out to 200 meters
    k90d500m = 7,   // 90 degree sector out to 500 meters
    // Argos-1000 systems only
    k60d1000m = 8,  // 60 degree sector out to 1000 meters
    // Argos-350 systems
    k90d100m = 9,
    k90d200m = 10,
    k90d350m = 11,
    // All systems: standby mode
    kStandby = 12,
};

enum class SystemType {
    kFS500 = 1,   // Argos-500 sonar
    kFS1000 = 2,  // Argos-1000 sonar
    kFS350 = 3,   // Argos-350 sonar
};

struct Timestamp {
    double seconds_since_epoch{};
};

struct RequestResult {
    Timestamp time{};
    ResultCode code{ResultCode::Success};
    std::string detail;
};

struct Position {
    double latitude_degrees{};
    double longitude_degrees{};
};

struct Heading {
    double degrees{};
};

struct ProcessorSettings {
    Timestamp time{};
    float min_inwater_squelch{};
    float max_inwater_squelch{};
    float inwater_squelch{};
    bool squelchless_inwater_detector{};
    bool detect_bottom{};
    SystemType system_type{SystemType::kFS500};
    FieldOfView fov{FieldOfView::k90d500m};
};

struct GetProcessorSettingsResponse {
    RequestResult result;
    ProcessorSettings settings;
};

struct SetFieldOfViewResponse {
    RequestResult result;
};

struct SetBottomDetectionResponse {
    RequestResult result;
};

struct SetInWaterSquelchResponse {
    RequestResult result;
};

struct SetSquelchlessInWaterDetectorResponse {
    RequestResult result;
};

struct VesselInfo {
    float draft{};        // Draft of the keel in meters
    float keel_offset{};  // Offset from keel to transducer center
};

struct GetVesselInfoResponse {
    RequestResult result;
    VesselInfo info;
};

struct Bin {
    std::int32_t hor_index{};
    std::int32_t ver_index{};
    std::int32_t range_index{};
    float cross_range{};  // meters relative to sonar
    float down_range{};   // meters relative to sonar
    float depth{};        // meters
    float strength{};     // range normalized loudness
};

struct TargetGroup {
    std::vector<Bin> bins;
};

struct TargetData {
    Timestamp time{};
    std::string serial;
    std::optional<Heading> heading;
    std::optional<Position> position;
    std::vector<Bin> bottom;          // Sea floor detections
    std::vector<TargetGroup> groups;  // In-water target groups
    double max_depth{};
    std::int32_t max_range_index{};
};

struct HydrophoneData {
    Timestamp time{};
    std::string serial;
    std::string transmit_id;
    std::int32_t num_hor_phones{};
    std::int32_t num_ver_phones{};
    // Raw timeseries data - flattened 2D array (channels x samples)
    std::vector<float> raw_timeseries;
    std::int32_t num_channels{};
    std::int32_t num_samples{};
};

}  // namespace farsounder
