#include <gtest/gtest.h>

#include <string>

#include "conversions_internal.hpp"

namespace {

using farsounder::ArrayDataOrder;
using farsounder::ArrayDataType;
using farsounder::FieldOfView;
using farsounder::GridMode;
using farsounder::ResultCode;
using farsounder::SystemType;
using farsounder::detail::convert_array_order;
using farsounder::detail::convert_array_type;
using farsounder::detail::convert_bin;
using farsounder::detail::convert_fov;
using farsounder::detail::convert_fov_to_proto;
using farsounder::detail::convert_hydrophone_data;
using farsounder::detail::convert_processor_settings;
using farsounder::detail::convert_raw_target_data;
using farsounder::detail::convert_result;
using farsounder::detail::convert_result_code;
using farsounder::detail::convert_system_type;
using farsounder::detail::convert_target_data;
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
    EXPECT_EQ(convert_result_code(
                  static_cast<proto::nav_api::RequestResult::ResultCode>(999)),
              ResultCode::UnknownError);
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
    EXPECT_EQ(convert_fov(static_cast<proto::nav_api::FieldOfView>(999)),
              FieldOfView::k90d500m);
    EXPECT_EQ(convert_fov_to_proto(static_cast<FieldOfView>(999)),
              proto::nav_api::k90d500m);
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
    EXPECT_EQ(
        convert_system_type(
            static_cast<proto::nav_api::ProcessorSettings::SystemType>(999)),
        SystemType::kFS500);
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
    EXPECT_DOUBLE_EQ(convert_timestamp(time).seconds_since_epoch, 0.5);
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
    EXPECT_NEAR(convert_timestamp(time).seconds_since_epoch, 1770651758.188,
                1e-6);
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
    proto::nav_api::VesselInfo vessel;
    vessel.set_draft(3.5f);
    vessel.set_keel_offset(0.25f);
    const auto converted = convert_vessel_info(vessel);
    EXPECT_FLOAT_EQ(converted.draft, 3.5f);
    EXPECT_FLOAT_EQ(converted.keel_offset, 0.25f);
}

TEST(ArrayTypeMapping, MapsAllValues) {
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::BYTE),
              ArrayDataType::kByte);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::INT16),
              ArrayDataType::kInt16);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::UINT16),
              ArrayDataType::kUInt16);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::INT32),
              ArrayDataType::kInt32);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::UINT32),
              ArrayDataType::kUInt32);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::INT64),
              ArrayDataType::kInt64);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::UINT64),
              ArrayDataType::kUInt64);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::FLOAT32),
              ArrayDataType::kFloat32);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::FLOAT64),
              ArrayDataType::kFloat64);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::COMPLEX64),
              ArrayDataType::kComplex64);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::COMPLEX128),
              ArrayDataType::kComplex128);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::BOOL),
              ArrayDataType::kBool);
}

TEST(ArrayTypeMapping, UnknownDefaultsToByte) {
    EXPECT_EQ(
        convert_array_type(static_cast<proto::array::ArrayData::Type>(999)),
        ArrayDataType::kByte);
}

TEST(ArrayOrderMapping, MapsAllValues) {
    EXPECT_EQ(convert_array_order(proto::array::ArrayData::ROW_MAJOR),
              ArrayDataOrder::kRowMajor);
    EXPECT_EQ(convert_array_order(proto::array::ArrayData::COLUMN_MAJOR),
              ArrayDataOrder::kColumnMajor);
}

TEST(ArrayOrderMapping, UnknownDefaultsToRowMajor) {
    EXPECT_EQ(
        convert_array_order(static_cast<proto::array::ArrayData::Order>(999)),
        ArrayDataOrder::kRowMajor);
}

TEST(BinConversion, CopiesFields) {
    proto::nav_api::Bin bin;
    bin.set_hor_index(1);
    bin.set_ver_index(2);
    bin.set_range_index(3);
    bin.set_cross_range(1.5f);
    bin.set_down_range(2.5f);
    bin.set_depth(3.5f);
    bin.set_strength(4.5f);
    const auto converted = convert_bin(bin);
    EXPECT_EQ(converted.hor_index, 1);
    EXPECT_EQ(converted.ver_index, 2);
    EXPECT_EQ(converted.range_index, 3);
    EXPECT_FLOAT_EQ(converted.cross_range, 1.5f);
    EXPECT_FLOAT_EQ(converted.down_range, 2.5f);
    EXPECT_FLOAT_EQ(converted.depth, 3.5f);
    EXPECT_FLOAT_EQ(converted.strength, 4.5f);
}

TEST(HydrophoneConversion, CopiesArrayDataWhenPresent) {
    proto::nav_api::HydrophoneData proto_data;
    proto_data.set_serial("serial-1");
    proto_data.set_transmit_id("tx-1");
    proto_data.set_num_hor_phones(3);
    proto_data.set_num_ver_phones(4);
    auto* time = proto_data.mutable_time();
    time->set_year(1970);
    time->set_month(1);
    time->set_day(1);
    time->set_hour(0);
    time->set_minute(0);
    time->set_second(0);
    time->set_millisecond(0);
    auto* array = proto_data.mutable_raw_timeseries();
    array->add_dims(12);
    array->add_dims(256);
    array->set_type(proto::array::ArrayData::INT16);
    array->set_order(proto::array::ArrayData::COLUMN_MAJOR);
    array->set_data(std::string("\x01\x02\x03\x04", 4));
    const auto converted = convert_hydrophone_data(proto_data);
    EXPECT_EQ(converted.serial, "serial-1");
    EXPECT_EQ(converted.transmit_id, "tx-1");
    EXPECT_EQ(converted.num_hor_phones, 3);
    EXPECT_EQ(converted.num_ver_phones, 4);
    ASSERT_EQ(converted.dims.size(), 2u);
    EXPECT_EQ(converted.dims[0], 12);
    EXPECT_EQ(converted.dims[1], 256);
    EXPECT_EQ(converted.type, ArrayDataType::kInt16);
    EXPECT_EQ(converted.order, ArrayDataOrder::kColumnMajor);
    EXPECT_EQ(converted.raw_timeseries, std::string("\x01\x02\x03\x04", 4));
}

TEST(HydrophoneConversion, HandlesMissingRawTimeseries) {
    proto::nav_api::HydrophoneData proto_data;
    proto_data.set_serial("serial-missing-array");
    proto_data.set_transmit_id("tx-missing-array");
    proto_data.set_num_hor_phones(1);
    proto_data.set_num_ver_phones(2);
    const auto converted = convert_hydrophone_data(proto_data);
    EXPECT_EQ(converted.serial, "serial-missing-array");
    EXPECT_EQ(converted.transmit_id, "tx-missing-array");
    EXPECT_TRUE(converted.dims.empty());
    EXPECT_EQ(converted.type, ArrayDataType::kByte);
    EXPECT_EQ(converted.order, ArrayDataOrder::kRowMajor);
    EXPECT_TRUE(converted.raw_timeseries.empty());
}

TEST(TargetDataConversion, CopiesFields) {
    proto::nav_api::TargetData proto_data;
    proto_data.set_serial("serial-2");
    proto_data.set_max_depth(55.0);
    proto_data.set_max_range_index(7);
    auto* time = proto_data.mutable_time();
    time->set_year(1970);
    time->set_month(1);
    time->set_day(1);
    time->set_hour(0);
    time->set_minute(0);
    time->set_second(1);
    time->set_millisecond(0);
    auto* heading = proto_data.mutable_heading();
    heading->set_heading(12.5);
    auto* position = proto_data.mutable_position();
    position->set_lat(42.0);
    position->set_lon(-70.0);
    auto* bottom = proto_data.add_bottom();
    bottom->set_hor_index(1);
    bottom->set_ver_index(2);
    bottom->set_range_index(3);
    bottom->set_cross_range(1.0f);
    bottom->set_down_range(2.0f);
    bottom->set_depth(3.0f);
    bottom->set_strength(4.0f);
    auto* group = proto_data.add_groups();
    auto* group_bin = group->add_bins();
    group_bin->set_hor_index(4);
    group_bin->set_ver_index(5);
    group_bin->set_range_index(6);
    group_bin->set_cross_range(5.0f);
    group_bin->set_down_range(6.0f);
    group_bin->set_depth(7.0f);
    group_bin->set_strength(8.0f);
    auto* grid = proto_data.mutable_grid_description();
    grid->set_mode(proto::grid_description::GridDescription::kFixed);
    grid->add_hor_angles(1.0);
    grid->add_hor_angles(-1.0);
    grid->add_ver_angles(2.0);
    grid->add_ver_angles(-2.0);
    grid->set_max_range(123.0);
    const auto converted = convert_target_data(proto_data);
    EXPECT_EQ(converted.serial, "serial-2");
    ASSERT_TRUE(converted.heading.has_value());
    EXPECT_DOUBLE_EQ(converted.heading->degrees, 12.5);
    ASSERT_TRUE(converted.position.has_value());
    EXPECT_DOUBLE_EQ(converted.position->latitude_degrees, 42.0);
    EXPECT_DOUBLE_EQ(converted.position->longitude_degrees, -70.0);
    ASSERT_EQ(converted.bottom.size(), 1u);
    EXPECT_EQ(converted.bottom[0].hor_index, 1);
    ASSERT_EQ(converted.groups.size(), 1u);
    ASSERT_EQ(converted.groups[0].bins.size(), 1u);
    EXPECT_EQ(converted.groups[0].bins[0].hor_index, 4);
    ASSERT_TRUE(converted.grid_description.has_value());
    EXPECT_EQ(converted.grid_description->mode, GridMode::kFixed);
    ASSERT_EQ(converted.grid_description->hor_angles.size(), 2u);
    EXPECT_DOUBLE_EQ(converted.grid_description->hor_angles[0], 1.0);
    EXPECT_DOUBLE_EQ(converted.grid_description->hor_angles[1], -1.0);
    ASSERT_EQ(converted.grid_description->ver_angles.size(), 2u);
    EXPECT_DOUBLE_EQ(converted.grid_description->ver_angles[0], 2.0);
    EXPECT_DOUBLE_EQ(converted.grid_description->ver_angles[1], -2.0);
    EXPECT_DOUBLE_EQ(converted.grid_description->max_range, 123.0);
    EXPECT_DOUBLE_EQ(converted.max_depth, 55.0);
    EXPECT_EQ(converted.max_range_index, 7);
}

TEST(TargetDataConversion, HandlesMissingOptionalFields) {
    proto::nav_api::TargetData proto_data;
    proto_data.set_serial("serial-minimal");
    proto_data.set_max_depth(10.0);
    proto_data.set_max_range_index(3);
    const auto converted = convert_target_data(proto_data);
    EXPECT_EQ(converted.serial, "serial-minimal");
    EXPECT_FALSE(converted.heading.has_value());
    EXPECT_FALSE(converted.position.has_value());
    EXPECT_TRUE(converted.bottom.empty());
    EXPECT_TRUE(converted.groups.empty());
    EXPECT_FALSE(converted.grid_description.has_value());
    EXPECT_DOUBLE_EQ(converted.max_depth, 10.0);
    EXPECT_EQ(converted.max_range_index, 3);
}

TEST(RawTargetDataConversion, CopiesFields) {
    proto::nav_api::RawTargetData proto_data;
    proto_data.set_serial("serial-raw");
    proto_data.set_max_depth(75.0);
    proto_data.set_max_range_index(11);
    proto_data.set_kernel_roll(1.25f);
    proto_data.add_rolls(0.1f);
    proto_data.add_rolls(-0.2f);
    proto_data.add_tilts(0.3f);
    proto_data.add_tilts(-0.4f);
    proto_data.set_bin_length(0.75f);
    proto_data.set_range_to_first_bin(8.5f);
    auto* time = proto_data.mutable_time();
    time->set_year(1970);
    time->set_month(1);
    time->set_day(1);
    time->set_hour(0);
    time->set_minute(0);
    time->set_second(2);
    time->set_millisecond(0);
    auto* heading = proto_data.mutable_heading();
    heading->set_heading(87.5);
    auto* position = proto_data.mutable_position();
    position->set_lat(41.0);
    position->set_lon(-71.0);
    auto* bottom = proto_data.add_bottom();
    bottom->set_hor_index(1);
    bottom->set_ver_index(2);
    bottom->set_range_index(3);
    bottom->set_cross_range(4.0f);
    bottom->set_down_range(5.0f);
    bottom->set_depth(6.0f);
    bottom->set_strength(7.0f);
    auto* target = proto_data.add_target();
    target->set_hor_index(8);
    target->set_ver_index(9);
    target->set_range_index(10);
    target->set_cross_range(11.0f);
    target->set_down_range(12.0f);
    target->set_depth(13.0f);
    target->set_strength(14.0f);
    auto* grid = proto_data.mutable_grid_description();
    grid->set_mode(proto::grid_description::GridDescription::kFixed);
    grid->add_hor_angles(3.0);
    grid->add_ver_angles(-3.0);
    grid->set_max_range(250.0);

    const auto converted = convert_raw_target_data(proto_data);
    EXPECT_EQ(converted.serial, "serial-raw");
    ASSERT_TRUE(converted.heading.has_value());
    EXPECT_DOUBLE_EQ(converted.heading->degrees, 87.5);
    ASSERT_TRUE(converted.position.has_value());
    EXPECT_DOUBLE_EQ(converted.position->latitude_degrees, 41.0);
    EXPECT_DOUBLE_EQ(converted.position->longitude_degrees, -71.0);
    ASSERT_EQ(converted.bottom.size(), 1u);
    EXPECT_EQ(converted.bottom[0].hor_index, 1);
    ASSERT_EQ(converted.target.size(), 1u);
    EXPECT_EQ(converted.target[0].hor_index, 8);
    ASSERT_TRUE(converted.grid_description.has_value());
    EXPECT_EQ(converted.grid_description->mode, GridMode::kFixed);
    ASSERT_EQ(converted.grid_description->hor_angles.size(), 1u);
    EXPECT_DOUBLE_EQ(converted.grid_description->hor_angles[0], 3.0);
    ASSERT_EQ(converted.grid_description->ver_angles.size(), 1u);
    EXPECT_DOUBLE_EQ(converted.grid_description->ver_angles[0], -3.0);
    EXPECT_DOUBLE_EQ(converted.grid_description->max_range, 250.0);
    EXPECT_DOUBLE_EQ(converted.max_depth, 75.0);
    EXPECT_EQ(converted.max_range_index, 11);
    EXPECT_FLOAT_EQ(converted.kernel_roll, 1.25f);
    ASSERT_EQ(converted.rolls.size(), 2u);
    EXPECT_FLOAT_EQ(converted.rolls[0], 0.1f);
    EXPECT_FLOAT_EQ(converted.rolls[1], -0.2f);
    ASSERT_EQ(converted.tilts.size(), 2u);
    EXPECT_FLOAT_EQ(converted.tilts[0], 0.3f);
    EXPECT_FLOAT_EQ(converted.tilts[1], -0.4f);
    EXPECT_FLOAT_EQ(converted.bin_length, 0.75f);
    EXPECT_FLOAT_EQ(converted.range_to_first_bin, 8.5f);
}

TEST(RawTargetDataConversion, HandlesMissingOptionalFields) {
    proto::nav_api::RawTargetData proto_data;
    proto_data.set_serial("serial-raw-minimal");

    const auto converted = convert_raw_target_data(proto_data);
    EXPECT_EQ(converted.serial, "serial-raw-minimal");
    EXPECT_FALSE(converted.heading.has_value());
    EXPECT_FALSE(converted.position.has_value());
    EXPECT_TRUE(converted.bottom.empty());
    EXPECT_TRUE(converted.target.empty());
    EXPECT_FALSE(converted.grid_description.has_value());
    EXPECT_DOUBLE_EQ(converted.max_depth, 0.0);
    EXPECT_EQ(converted.max_range_index, 0);
    EXPECT_FLOAT_EQ(converted.kernel_roll, 0.0f);
    EXPECT_TRUE(converted.rolls.empty());
    EXPECT_TRUE(converted.tilts.empty());
    EXPECT_FLOAT_EQ(converted.bin_length, 0.0f);
    EXPECT_FLOAT_EQ(converted.range_to_first_bin, 0.0f);
}

TEST(BasicSanity, ConversionHelpersRemainUsable) {
    // Keep a small broad sanity for grouped helper usage.
    proto::nav_api::Bin bin;
    bin.set_hor_index(8);
    bin.set_strength(1.25f);
    const auto converted_bin = convert_bin(bin);
    EXPECT_EQ(converted_bin.hor_index, 8);
    EXPECT_FLOAT_EQ(converted_bin.strength, 1.25f);

    proto::time::Time time;
    time.set_year(1970);
    time.set_month(1);
    time.set_day(1);
    time.set_hour(0);
    time.set_minute(0);
    time.set_second(3);
    time.set_millisecond(500);
    EXPECT_DOUBLE_EQ(convert_timestamp(time).seconds_since_epoch, 3.5);
}

}  // namespace
