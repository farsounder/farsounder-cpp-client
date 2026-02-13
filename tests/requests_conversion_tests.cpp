#include <gtest/gtest.h>

#include "conversions_internal.hpp"

namespace {

using farsounder::FieldOfView;
using farsounder::ResultCode;
using farsounder::SystemType;
using farsounder::detail::convert_fov;
using farsounder::detail::convert_fov_to_proto;
using farsounder::detail::convert_processor_settings;
using farsounder::detail::convert_result;
using farsounder::detail::convert_result_code;
using farsounder::detail::convert_system_type;
using farsounder::detail::convert_timestamp;
using farsounder::detail::convert_vessel_info;

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

TEST(ResultCodeMapping, UnknownDefaultsToUnknownError) {
    const auto unknown_code =
        static_cast<proto::nav_api::RequestResult::ResultCode>(99);
    EXPECT_EQ(convert_result_code(unknown_code), ResultCode::UnknownError);
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

TEST(FieldOfViewMapping, UnknownDefaultsToNinetyDegreeFiveHundredMeters) {
    const auto unknown_fov = static_cast<proto::nav_api::FieldOfView>(999);
    EXPECT_EQ(convert_fov(unknown_fov), FieldOfView::k90d500m);

    const auto unknown_wrapper = static_cast<FieldOfView>(999);
    EXPECT_EQ(convert_fov_to_proto(unknown_wrapper), proto::nav_api::k90d500m);
}

TEST(SystemTypeMapping, MapsAllValues) {
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS500),
              SystemType::kFS500);
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS1000),
              SystemType::kFS1000);
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS350),
              SystemType::kFS350);
}

TEST(SystemTypeMapping, UnknownDefaultsToFs500) {
    const auto unknown_system =
        static_cast<proto::nav_api::ProcessorSettings::SystemType>(999);
    EXPECT_EQ(convert_system_type(unknown_system), SystemType::kFS500);
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

TEST(TimestampConversion, ConvertsDateTimeToEpoch) {
    proto::time::Time time;
    time.set_year(2026);
    time.set_month(2);
    time.set_day(9);
    time.set_hour(15);
    time.set_minute(42);
    time.set_second(38);
    time.set_millisecond(188);

    const auto converted = convert_timestamp(time);
    EXPECT_NEAR(converted.seconds_since_epoch, 1770651758.188, 1e-6);
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

TEST(RequestResultConversion, MissingTimeKeepsDefaultTimestamp) {
    proto::nav_api::RequestResult result;
    result.set_code(proto::nav_api::RequestResult::kSuccess);
    result.set_result_detail("ok");

    const auto converted = convert_result(result);
    EXPECT_DOUBLE_EQ(converted.time.seconds_since_epoch, 0.0);
    EXPECT_EQ(converted.code, ResultCode::Success);
    EXPECT_EQ(converted.detail, "ok");
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

TEST(ProcessorSettingsConversion, MissingTimeKeepsDefaultTimestamp) {
    proto::nav_api::ProcessorSettings settings;
    settings.set_min_inwater_squelch(1.0f);
    settings.set_max_inwater_squelch(2.0f);
    settings.set_inwater_squelch(1.5f);
    settings.set_squelchless_inwater_detector(false);
    settings.set_detect_bottom(true);
    settings.set_system_type(proto::nav_api::ProcessorSettings::kFS500);
    settings.set_fov(proto::nav_api::k90d500m);

    const auto converted = convert_processor_settings(settings);
    EXPECT_DOUBLE_EQ(converted.time.seconds_since_epoch, 0.0);
}

TEST(VesselInfoConversion, CopiesFields) {
    proto::nav_api::VesselInfo vessel_info;
    vessel_info.set_draft(2.5f);
    vessel_info.set_keel_offset(-0.5f);

    const auto converted = convert_vessel_info(vessel_info);
    EXPECT_FLOAT_EQ(converted.draft, 2.5f);
    EXPECT_FLOAT_EQ(converted.keel_offset, -0.5f);
}

}  // namespace
