#include <gtest/gtest.h>

#include "conversions_internal.hpp"

namespace {

using farsounder::ArrayDataOrder;
using farsounder::ArrayDataType;
using farsounder::FieldOfView;
using farsounder::ResultCode;
using farsounder::SystemType;
using farsounder::detail::convert_array_order;
using farsounder::detail::convert_array_type;
using farsounder::detail::convert_bin;
using farsounder::detail::convert_fov;
using farsounder::detail::convert_fov_to_proto;
using farsounder::detail::convert_hydrophone_data;
using farsounder::detail::convert_processor_settings;
using farsounder::detail::convert_result;
using farsounder::detail::convert_result_code;
using farsounder::detail::convert_system_type;
using farsounder::detail::convert_target_data;
using farsounder::detail::convert_timestamp;
using farsounder::detail::convert_vessel_info;

TEST(SharedConversions, CoversRequestSpecificConverters) {
    EXPECT_EQ(convert_result_code(proto::nav_api::RequestResult::kSuccess),
              ResultCode::Success);
    EXPECT_EQ(convert_result_code(
                  static_cast<proto::nav_api::RequestResult::ResultCode>(999)),
              ResultCode::UnknownError);

    proto::nav_api::RequestResult result;
    result.set_code(proto::nav_api::RequestResult::kInvalidRequest);
    result.set_result_detail("bad request");
    const auto converted_result = convert_result(result);
    EXPECT_DOUBLE_EQ(converted_result.time.seconds_since_epoch, 0.0);
    EXPECT_EQ(converted_result.code, ResultCode::InvalidRequest);
    EXPECT_EQ(converted_result.detail, "bad request");

    EXPECT_EQ(convert_fov_to_proto(FieldOfView::k120d100m),
              proto::nav_api::k120d100m);
}

TEST(SharedConversions, CoversSharedEnumConvertersWithDefaults) {
    EXPECT_EQ(convert_system_type(proto::nav_api::ProcessorSettings::kFS1000),
              SystemType::kFS1000);
    EXPECT_EQ(convert_system_type(
                  static_cast<proto::nav_api::ProcessorSettings::SystemType>(
                      999)),
              SystemType::kFS500);

    EXPECT_EQ(convert_fov(proto::nav_api::k120d200m), FieldOfView::k120d200m);
    EXPECT_EQ(convert_fov(static_cast<proto::nav_api::FieldOfView>(999)),
              FieldOfView::k90d500m);

    EXPECT_EQ(convert_array_type(proto::array::ArrayData::FLOAT64),
              ArrayDataType::kFloat64);
    EXPECT_EQ(convert_array_type(static_cast<proto::array::ArrayData::Type>(999)),
              ArrayDataType::kByte);

    EXPECT_EQ(convert_array_order(proto::array::ArrayData::COLUMN_MAJOR),
              ArrayDataOrder::kColumnMajor);
    EXPECT_EQ(
        convert_array_order(static_cast<proto::array::ArrayData::Order>(999)),
        ArrayDataOrder::kRowMajor);
}

TEST(SharedConversions, CoversTimestampAndStructConverters) {
    proto::time::Time time;
    time.set_year(1970);
    time.set_month(1);
    time.set_day(1);
    time.set_hour(0);
    time.set_minute(0);
    time.set_second(3);
    time.set_millisecond(500);
    EXPECT_DOUBLE_EQ(convert_timestamp(time).seconds_since_epoch, 3.5);

    proto::nav_api::Bin bin;
    bin.set_hor_index(8);
    bin.set_strength(1.25f);
    const auto converted_bin = convert_bin(bin);
    EXPECT_EQ(converted_bin.hor_index, 8);
    EXPECT_FLOAT_EQ(converted_bin.strength, 1.25f);

    proto::nav_api::ProcessorSettings settings;
    settings.set_system_type(proto::nav_api::ProcessorSettings::kFS350);
    settings.set_fov(proto::nav_api::k90d350m);
    const auto converted_settings = convert_processor_settings(settings);
    EXPECT_EQ(converted_settings.system_type, SystemType::kFS350);
    EXPECT_EQ(converted_settings.fov, FieldOfView::k90d350m);
    EXPECT_DOUBLE_EQ(converted_settings.time.seconds_since_epoch, 0.0);

    proto::nav_api::VesselInfo vessel;
    vessel.set_draft(3.5f);
    vessel.set_keel_offset(0.25f);
    const auto converted_vessel = convert_vessel_info(vessel);
    EXPECT_FLOAT_EQ(converted_vessel.draft, 3.5f);
    EXPECT_FLOAT_EQ(converted_vessel.keel_offset, 0.25f);
}

TEST(SharedConversions, CoversSubscriberPayloadConverters) {
    proto::nav_api::HydrophoneData hydrophone;
    hydrophone.set_serial("h");
    hydrophone.set_transmit_id("tx");
    auto hydrophone_converted = convert_hydrophone_data(hydrophone);
    EXPECT_TRUE(hydrophone_converted.dims.empty());
    EXPECT_EQ(hydrophone_converted.type, ArrayDataType::kByte);

    auto* raw_timeseries = hydrophone.mutable_raw_timeseries();
    raw_timeseries->add_dims(2);
    raw_timeseries->set_type(proto::array::ArrayData::INT16);
    raw_timeseries->set_order(proto::array::ArrayData::COLUMN_MAJOR);
    raw_timeseries->set_data(std::string("\x01\x02", 2));
    hydrophone_converted = convert_hydrophone_data(hydrophone);
    ASSERT_EQ(hydrophone_converted.dims.size(), 1u);
    EXPECT_EQ(hydrophone_converted.type, ArrayDataType::kInt16);
    EXPECT_EQ(hydrophone_converted.order, ArrayDataOrder::kColumnMajor);

    proto::nav_api::TargetData target;
    target.set_serial("t");
    target.set_max_depth(4.0);
    target.set_max_range_index(2);
    auto target_converted = convert_target_data(target);
    EXPECT_FALSE(target_converted.heading.has_value());
    EXPECT_TRUE(target_converted.bottom.empty());
    EXPECT_FALSE(target_converted.grid_description.has_value());

    auto* heading = target.mutable_heading();
    heading->set_heading(1.0);
    auto* bottom = target.add_bottom();
    bottom->set_hor_index(10);
    auto* grid = target.mutable_grid_description();
    grid->set_mode(proto::grid_description::GridDescription::kFixed);
    target_converted = convert_target_data(target);
    EXPECT_TRUE(target_converted.heading.has_value());
    ASSERT_EQ(target_converted.bottom.size(), 1u);
    EXPECT_EQ(target_converted.bottom[0].hor_index, 10);
    EXPECT_TRUE(target_converted.grid_description.has_value());
}

}  // namespace
