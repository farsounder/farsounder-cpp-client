#include <gtest/gtest.h>

#include <string>

#include "subscriber_internal.hpp"

namespace {

using farsounder::ArrayDataOrder;
using farsounder::ArrayDataType;
using farsounder::FieldOfView;
using farsounder::GridMode;
using farsounder::SystemType;
using farsounder::detail::convert_array_order;
using farsounder::detail::convert_array_type;
using farsounder::detail::convert_bin;
using farsounder::detail::convert_fov;
using farsounder::detail::convert_hydrophone_data;
using farsounder::detail::convert_processor_settings;
using farsounder::detail::convert_system_type;
using farsounder::detail::convert_target_data;
using farsounder::detail::convert_timestamp;
using farsounder::detail::convert_vessel_info;

TEST(SubscriberConversions, TimestampConversion) {
    proto::time::Time time;
    time.set_year(1970);
    time.set_month(1);
    time.set_day(1);
    time.set_hour(0);
    time.set_minute(0);
    time.set_second(2);
    time.set_millisecond(250);

    const auto converted = convert_timestamp(time);
    EXPECT_DOUBLE_EQ(converted.seconds_since_epoch, 2.25);
}

TEST(SubscriberConversions, EnumMappings) {
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS500),
              SystemType::kFS500);
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS1000),
              SystemType::kFS1000);
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS350),
              SystemType::kFS350);

    EXPECT_EQ(convert_fov(proto::nav_api::k120d100m), FieldOfView::k120d100m);
    EXPECT_EQ(convert_fov(proto::nav_api::kStandby), FieldOfView::kStandby);

    EXPECT_EQ(convert_array_type(proto::array::ArrayData::FLOAT32),
              ArrayDataType::kFloat32);
    EXPECT_EQ(convert_array_type(proto::array::ArrayData::UINT16),
              ArrayDataType::kUInt16);

    EXPECT_EQ(convert_array_order(proto::array::ArrayData::ROW_MAJOR),
              ArrayDataOrder::kRowMajor);
    EXPECT_EQ(convert_array_order(proto::array::ArrayData::COLUMN_MAJOR),
              ArrayDataOrder::kColumnMajor);
}

TEST(SubscriberConversions, BinConversionCopiesFields) {
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

TEST(SubscriberConversions, HydrophoneDataConversionCopiesArrayData) {
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

TEST(SubscriberConversions, TargetDataConversionCopiesFields) {
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

TEST(SubscriberConversions, ProcessorSettingsConversionCopiesFields) {
    proto::nav_api::ProcessorSettings proto_settings;
    proto_settings.set_min_inwater_squelch(1.0f);
    proto_settings.set_max_inwater_squelch(2.0f);
    proto_settings.set_inwater_squelch(1.5f);
    proto_settings.set_squelchless_inwater_detector(true);
    proto_settings.set_detect_bottom(false);
    proto_settings.set_system_type(proto::nav_api::ProcessorSettings::kFS500);
    proto_settings.set_fov(proto::nav_api::k120d200m);

    auto* time = proto_settings.mutable_time();
    time->set_year(1970);
    time->set_month(1);
    time->set_day(1);
    time->set_hour(0);
    time->set_minute(0);
    time->set_second(0);
    time->set_millisecond(0);

    const auto converted = convert_processor_settings(proto_settings);
    EXPECT_FLOAT_EQ(converted.min_inwater_squelch, 1.0f);
    EXPECT_FLOAT_EQ(converted.max_inwater_squelch, 2.0f);
    EXPECT_FLOAT_EQ(converted.inwater_squelch, 1.5f);
    EXPECT_TRUE(converted.squelchless_inwater_detector);
    EXPECT_FALSE(converted.detect_bottom);
    EXPECT_EQ(converted.system_type, SystemType::kFS500);
    EXPECT_EQ(converted.fov, FieldOfView::k120d200m);
}

TEST(SubscriberConversions, VesselInfoConversionCopiesFields) {
    proto::nav_api::VesselInfo proto_info;
    proto_info.set_draft(2.5f);
    proto_info.set_keel_offset(-0.5f);

    const auto converted = convert_vessel_info(proto_info);
    EXPECT_FLOAT_EQ(converted.draft, 2.5f);
    EXPECT_FLOAT_EQ(converted.keel_offset, -0.5f);
}

}  // namespace
