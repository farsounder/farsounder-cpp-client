#pragma once

#include "farsounder/types.hpp"
#include "proto/array.pb.h"
#include "proto/grid_description.pb.h"
#include "proto/nav_api.pb.h"

namespace farsounder::detail {

Timestamp convert_timestamp(const proto::time::Time& t);

ResultCode convert_result_code(proto::nav_api::RequestResult::ResultCode code);
RequestResult convert_result(const proto::nav_api::RequestResult& r);

SystemType convert_system_type(
    proto::nav_api::ProcessorSettings::SystemType t);
FieldOfView convert_fov(proto::nav_api::FieldOfView fov);
proto::nav_api::FieldOfView convert_fov_to_proto(FieldOfView fov);

ArrayDataType convert_array_type(proto::array::ArrayData::Type type);
ArrayDataOrder convert_array_order(proto::array::ArrayData::Order order);

Bin convert_bin(const proto::nav_api::Bin& b);
GridDescription convert_grid_description(
    const proto::grid_description::GridDescription& g);
HydrophoneData convert_hydrophone_data(const proto::nav_api::HydrophoneData& h);
TargetData convert_target_data(const proto::nav_api::TargetData& t);
RawTargetData convert_raw_target_data(const proto::nav_api::RawTargetData& t);

ProcessorSettings convert_processor_settings(
    const proto::nav_api::ProcessorSettings& s);
VesselInfo convert_vessel_info(const proto::nav_api::VesselInfo& v);

}  // namespace farsounder::detail
