#include <gtest/gtest.h>

#include "requests_internal.hpp"

namespace {

using farsounder::FieldOfView;
using farsounder::ResultCode;
using farsounder::SystemType;
using farsounder::requests::detail::convert_fov;
using farsounder::requests::detail::convert_fov_to_proto;
using farsounder::requests::detail::convert_processor_settings;
using farsounder::requests::detail::convert_result;
using farsounder::requests::detail::convert_result_code;
using farsounder::requests::detail::convert_system_type;
using farsounder::requests::detail::convert_timestamp;

TEST(ResultCodeMapping, MapsAllValues) {
    EXPECT_EQ(convert_result_code(proto::nav_api::RequestResult::kSuccess),
              ResultCode::Success);
    EXPECT_EQ(convert_result_code(proto::nav_api::RequestResult::kUnknownError),
              ResultCode::UnknownError);
    EXPECT_EQ(convert_result_code(
                  proto::nav_api::RequestResult::kOperationUnavailable),
              ResultCode::OperationUnavailable);
    EXPECT_EQ(convert_result_code(
                  proto::nav_api::RequestResult::kParameterOutOfRange),
              ResultCode::ParameterOutOfRange);
    EXPECT_EQ(
        convert_result_code(proto::nav_api::RequestResult::kParameterMissing),
        ResultCode::ParameterMissing);
    EXPECT_EQ(
        convert_result_code(proto::nav_api::RequestResult::kInvalidRequest),
        ResultCode::InvalidRequest);
}

TEST(FieldOfViewMapping, MapsAllValues) {
    EXPECT_EQ(convert_fov(proto::nav_api::k120d100m), FieldOfView::k120d100m);
    EXPECT_EQ(convert_fov(proto::nav_api::k120d200m), FieldOfView::k120d200m);
    EXPECT_EQ(convert_fov(proto::nav_api::k90d500m), FieldOfView::k90d500m);
    EXPECT_EQ(convert_fov(proto::nav_api::k60d1000m), FieldOfView::k60d1000m);
    EXPECT_EQ(convert_fov(proto::nav_api::k90d100m), FieldOfView::k90d100m);
    EXPECT_EQ(convert_fov(proto::nav_api::k90d200m), FieldOfView::k90d200m);
    EXPECT_EQ(convert_fov(proto::nav_api::k90d350m), FieldOfView::k90d350m);
    EXPECT_EQ(convert_fov(proto::nav_api::kStandby), FieldOfView::kStandby);
}

TEST(FieldOfViewMapping, RoundTripsThroughProto) {
    const FieldOfView values[] = {
        FieldOfView::k120d100m, FieldOfView::k120d200m, FieldOfView::k90d500m,
        FieldOfView::k60d1000m, FieldOfView::k90d100m,  FieldOfView::k90d200m,
        FieldOfView::k90d350m,  FieldOfView::kStandby};

    for (auto value : values) {
        EXPECT_EQ(convert_fov(convert_fov_to_proto(value)), value);
    }
}

TEST(SystemTypeMapping, MapsAllValues) {
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS500),
              SystemType::kFS500);
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS1000),
              SystemType::kFS1000);
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS350),
              SystemType::kFS350);
}

TEST(TimestampConversion, ConvertsEpochWithMilliseconds) {
    proto::time::Time time;
    time.set_year(1970);
    time.set_month(1);
    time.set_day(1);
    time.set_hour(0);
    time.set_minute(0);
    time.set_second(0);
    time.set_millisecond(500);

    const auto converted = convert_timestamp(time);
    EXPECT_DOUBLE_EQ(converted.seconds_since_epoch, 0.5);
}

TEST(RequestResultConversion, CopiesFields) {
    proto::nav_api::RequestResult result;
    result.set_code(proto::nav_api::RequestResult::kParameterOutOfRange);
    result.set_result_detail("range error");
    auto* time = result.mutable_time();
    time->set_year(1970);
    time->set_month(1);
    time->set_day(1);
    time->set_hour(0);
    time->set_minute(0);
    time->set_second(1);
    time->set_millisecond(0);

    const auto converted = convert_result(result);
    EXPECT_EQ(converted.code, ResultCode::ParameterOutOfRange);
    EXPECT_EQ(converted.detail, "range error");
    EXPECT_DOUBLE_EQ(converted.time.seconds_since_epoch, 1.0);
}

TEST(ProcessorSettingsConversion, CopiesAllFields) {
    proto::nav_api::ProcessorSettings settings;
    settings.set_min_inwater_squelch(1.0f);
    settings.set_max_inwater_squelch(2.0f);
    settings.set_inwater_squelch(1.5f);
    settings.set_squelchless_inwater_detector(true);
    settings.set_detect_bottom(false);
    settings.set_system_type(proto::nav_api::ProcessorSettings::kFS350);
    settings.set_fov(proto::nav_api::k90d350m);

    auto* time = settings.mutable_time();
    time->set_year(1970);
    time->set_month(1);
    time->set_day(1);
    time->set_hour(0);
    time->set_minute(0);
    time->set_second(0);
    time->set_millisecond(0);

    const auto converted = convert_processor_settings(settings);
    EXPECT_DOUBLE_EQ(converted.time.seconds_since_epoch, 0.0);
    EXPECT_FLOAT_EQ(converted.min_inwater_squelch, 1.0f);
    EXPECT_FLOAT_EQ(converted.max_inwater_squelch, 2.0f);
    EXPECT_FLOAT_EQ(converted.inwater_squelch, 1.5f);
    EXPECT_TRUE(converted.squelchless_inwater_detector);
    EXPECT_FALSE(converted.detect_bottom);
    EXPECT_EQ(converted.system_type, SystemType::kFS350);
    EXPECT_EQ(converted.fov, FieldOfView::k90d350m);
}

}  // namespace
