#pragma once

#include <nlohmann/json.hpp>

#include "farsounder/history.hpp"
#include "farsounder/types.hpp"
#include "proto/nav_api.pb.h"

namespace farsounder::requests::detail {

history::GriddedBottomDetection parse_gridded_bottom_detection(
    const nlohmann::json& payload);
history::GriddedInwaterDetection parse_gridded_inwater_detection(
    const nlohmann::json& payload);
history::HistoryData parse_history_data(const nlohmann::json& payload);

}  // namespace farsounder::requests::detail
