#pragma once

#include <nlohmann/json.hpp>

#include "farsounder/history.hpp"
#include "farsounder/types.hpp"
#include "proto/nav_api.pb.h"

namespace farsounder::requests::detail {

Timestamp convert_timestamp(const proto::time::Time& t);
ResultCode convert_result_code(proto::nav_api::RequestResult::ResultCode code);
RequestResult convert_result(const proto::nav_api::RequestResult& r);
SystemType convert_system_type(proto::nav_api::ProcessorSettings::SystemType t);
FieldOfView convert_fov(proto::nav_api::FieldOfView fov);
proto::nav_api::FieldOfView convert_fov_to_proto(FieldOfView fov);
ProcessorSettings convert_processor_settings(
    const proto::nav_api::ProcessorSettings& s);
VesselInfo convert_vessel_info(const proto::nav_api::VesselInfo& v);

history::GriddedBottomDetection parse_gridded_bottom_detection(
    const nlohmann::json& payload);
history::GriddedInwaterDetection parse_gridded_inwater_detection(
    const nlohmann::json& payload);
history::HistoryData parse_history_data(const nlohmann::json& payload);

}  // namespace farsounder::requests::detail
