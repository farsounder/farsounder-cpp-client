#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "requests_internal.hpp"

namespace {

using farsounder::requests::detail::parse_history_data;

TEST(HistoryParsing, ParsesBottomAndInwaterDetections) {
    const auto payload = nlohmann::json::parse(R"({
        "gridded_bottom_detections": [
            {
                "timestamp_utc": 10.0,
                "latitude_degrees": 40.0,
                "longitude_degrees": -70.0,
                "grid_interval_meters": 2.5,
                "target_strength_db": -10.0,
                "is_tide_corrected": true,
                "uploaded_to_cloud": false,
                "number_of_points": 3,
                "depth_meters": 12.5
            }
        ],
        "gridded_inwater_detections": [
            {
                "timestamp_utc": 20.0,
                "latitude_degrees": 41.0,
                "longitude_degrees": -71.0,
                "grid_interval_meters": 3.5,
                "target_strength_db": -20.0,
                "is_tide_corrected": false,
                "uploaded_to_cloud": true,
                "number_of_points": 5,
                "deepest_depth_meters": 55.0,
                "shallowest_depth_meters": 4.0
            },
            {
                "timestamp_utc": 30.0,
                "latitude_degrees": 42.0,
                "longitude_degrees": -72.0,
                "grid_interval_meters": 4.5,
                "target_strength_db": -30.0,
                "is_tide_corrected": true,
                "uploaded_to_cloud": true,
                "number_of_points": 6
            }
        ]
    })");

    const auto data = parse_history_data(payload);

    ASSERT_EQ(data.gridded_bottom_detections.size(), 1u);
    const auto& bottom = data.gridded_bottom_detections.front();
    EXPECT_DOUBLE_EQ(bottom.timestamp_utc, 10.0);
    EXPECT_DOUBLE_EQ(bottom.depth_meters, 12.5);

    ASSERT_EQ(data.gridded_inwater_detections.size(), 2u);
    const auto& inwater_with_depth = data.gridded_inwater_detections.front();
    ASSERT_TRUE(inwater_with_depth.deepest_depth_meters.has_value());
    ASSERT_TRUE(inwater_with_depth.shallowest_depth_meters.has_value());
    EXPECT_DOUBLE_EQ(*inwater_with_depth.deepest_depth_meters, 55.0);
    EXPECT_DOUBLE_EQ(*inwater_with_depth.shallowest_depth_meters, 4.0);

    const auto& inwater_missing_depth = data.gridded_inwater_detections.back();
    EXPECT_FALSE(inwater_missing_depth.deepest_depth_meters.has_value());
    EXPECT_FALSE(inwater_missing_depth.shallowest_depth_meters.has_value());
}

TEST(HistoryParsing, HandlesMissingArrays) {
    const auto payload = nlohmann::json::parse(R"({})");

    const auto data = parse_history_data(payload);
    EXPECT_TRUE(data.gridded_bottom_detections.empty());
    EXPECT_TRUE(data.gridded_inwater_detections.empty());
}

}  // namespace
