#pragma once

#include <cstdint>
#include <vector>

namespace farsounder::history {

struct GriddedBottomDetection {
    double timestamp_utc{};
    double latitude_degrees{};
    double longitude_degrees{};
    double grid_interval_meters{};
    double target_strength_db{};
    bool is_tide_corrected{};
    bool uploaded_to_cloud{};
    std::int32_t number_of_points{};
    double depth_meters{};
};

struct GriddedInwaterDetection {
    double timestamp_utc{};
    double latitude_degrees{};
    double longitude_degrees{};
    double grid_interval_meters{};
    double target_strength_db{};
    bool is_tide_corrected{};
    bool uploaded_to_cloud{};
    std::int32_t number_of_points{};
    double deepest_depth_meters{};
    double shallowest_depth_meters{};
};

struct HistoryData {
    std::vector<GriddedBottomDetection> gridded_bottom_detections;
    std::vector<GriddedInwaterDetection> gridded_inwater_detections;
};

}  // namespace farsounder::history
