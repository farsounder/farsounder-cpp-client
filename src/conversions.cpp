#include "conversions_internal.hpp"

#include <ctime>
#include <utility>

namespace farsounder::detail {

Timestamp convert_timestamp(const proto::time::Time& t) {
    std::tm tm = {};
    tm.tm_year = static_cast<int>(t.year()) - 1900;
    tm.tm_mon = static_cast<int>(t.month()) - 1;
    tm.tm_mday = static_cast<int>(t.day());
    tm.tm_hour = static_cast<int>(t.hour());
    tm.tm_min = static_cast<int>(t.minute());
    tm.tm_sec = static_cast<int>(t.second());
#ifdef _WIN32
    auto epoch = _mkgmtime(&tm);
#else
    auto epoch = timegm(&tm);
#endif
    return Timestamp{static_cast<double>(epoch) + t.millisecond() / 1000.0};
}

ResultCode convert_result_code(proto::nav_api::RequestResult::ResultCode code) {
    switch (code) {
    case proto::nav_api::RequestResult::kSuccess:
        return ResultCode::Success;
    case proto::nav_api::RequestResult::kUnknownError:
        return ResultCode::UnknownError;
    case proto::nav_api::RequestResult::kOperationUnavailable:
        return ResultCode::OperationUnavailable;
    case proto::nav_api::RequestResult::kParameterOutOfRange:
        return ResultCode::ParameterOutOfRange;
    case proto::nav_api::RequestResult::kParameterMissing:
        return ResultCode::ParameterMissing;
    case proto::nav_api::RequestResult::kInvalidRequest:
        return ResultCode::InvalidRequest;
    default:
        return ResultCode::UnknownError;
    }
}

RequestResult convert_result(const proto::nav_api::RequestResult& r) {
    RequestResult result;
    if (r.has_time()) {
        result.time = convert_timestamp(r.time());
    }
    result.code = convert_result_code(r.code());
    result.detail = r.result_detail();
    return result;
}

SystemType convert_system_type(
    proto::nav_api::ProcessorSettings::SystemType t) {
    switch (t) {
    case proto::nav_api::ProcessorSettings::kFS500:
        return SystemType::kFS500;
    case proto::nav_api::ProcessorSettings::kFS1000:
        return SystemType::kFS1000;
    case proto::nav_api::ProcessorSettings::kFS350:
        return SystemType::kFS350;
    default:
        return SystemType::kFS500;
    }
}

FieldOfView convert_fov(proto::nav_api::FieldOfView fov) {
    switch (fov) {
    case proto::nav_api::k120d100m:
        return FieldOfView::k120d100m;
    case proto::nav_api::k120d200m:
        return FieldOfView::k120d200m;
    case proto::nav_api::k90d500m:
        return FieldOfView::k90d500m;
    case proto::nav_api::k60d1000m:
        return FieldOfView::k60d1000m;
    case proto::nav_api::k90d100m:
        return FieldOfView::k90d100m;
    case proto::nav_api::k90d200m:
        return FieldOfView::k90d200m;
    case proto::nav_api::k90d350m:
        return FieldOfView::k90d350m;
    case proto::nav_api::kStandby:
        return FieldOfView::kStandby;
    default:
        return FieldOfView::k90d500m;
    }
}

proto::nav_api::FieldOfView convert_fov_to_proto(FieldOfView fov) {
    switch (fov) {
    case FieldOfView::k120d100m:
        return proto::nav_api::k120d100m;
    case FieldOfView::k120d200m:
        return proto::nav_api::k120d200m;
    case FieldOfView::k90d500m:
        return proto::nav_api::k90d500m;
    case FieldOfView::k60d1000m:
        return proto::nav_api::k60d1000m;
    case FieldOfView::k90d100m:
        return proto::nav_api::k90d100m;
    case FieldOfView::k90d200m:
        return proto::nav_api::k90d200m;
    case FieldOfView::k90d350m:
        return proto::nav_api::k90d350m;
    case FieldOfView::kStandby:
        return proto::nav_api::kStandby;
    default:
        return proto::nav_api::k90d500m;
    }
}

ArrayDataType convert_array_type(proto::array::ArrayData::Type type) {
    switch (type) {
    case proto::array::ArrayData::BYTE:
        return ArrayDataType::kByte;
    case proto::array::ArrayData::INT16:
        return ArrayDataType::kInt16;
    case proto::array::ArrayData::UINT16:
        return ArrayDataType::kUInt16;
    case proto::array::ArrayData::INT32:
        return ArrayDataType::kInt32;
    case proto::array::ArrayData::UINT32:
        return ArrayDataType::kUInt32;
    case proto::array::ArrayData::INT64:
        return ArrayDataType::kInt64;
    case proto::array::ArrayData::UINT64:
        return ArrayDataType::kUInt64;
    case proto::array::ArrayData::FLOAT32:
        return ArrayDataType::kFloat32;
    case proto::array::ArrayData::FLOAT64:
        return ArrayDataType::kFloat64;
    case proto::array::ArrayData::COMPLEX64:
        return ArrayDataType::kComplex64;
    case proto::array::ArrayData::COMPLEX128:
        return ArrayDataType::kComplex128;
    case proto::array::ArrayData::BOOL:
        return ArrayDataType::kBool;
    default:
        return ArrayDataType::kByte;
    }
}

ArrayDataOrder convert_array_order(proto::array::ArrayData::Order order) {
    switch (order) {
    case proto::array::ArrayData::ROW_MAJOR:
        return ArrayDataOrder::kRowMajor;
    case proto::array::ArrayData::COLUMN_MAJOR:
        return ArrayDataOrder::kColumnMajor;
    default:
        return ArrayDataOrder::kRowMajor;
    }
}

Bin convert_bin(const proto::nav_api::Bin& b) {
    Bin bin;
    bin.hor_index = b.hor_index();
    bin.ver_index = b.ver_index();
    bin.range_index = b.range_index();
    bin.cross_range = b.cross_range();
    bin.down_range = b.down_range();
    bin.depth = b.depth();
    bin.strength = b.strength();
    return bin;
}

GridDescription convert_grid_description(
    const proto::grid_description::GridDescription& g) {
    GridDescription grid;
    if (g.has_mode()) {
        grid.mode = static_cast<GridMode>(g.mode());
    }
    grid.hor_angles.reserve(static_cast<size_t>(g.hor_angles_size()));
    for (const auto angle : g.hor_angles()) {
        grid.hor_angles.push_back(angle);
    }
    grid.ver_angles.reserve(static_cast<size_t>(g.ver_angles_size()));
    for (const auto angle : g.ver_angles()) {
        grid.ver_angles.push_back(angle);
    }
    if (g.has_max_range()) {
        grid.max_range = g.max_range();
    }
    return grid;
}

HydrophoneData convert_hydrophone_data(const proto::nav_api::HydrophoneData& h) {
    HydrophoneData data;
    if (h.has_time()) {
        data.time = convert_timestamp(h.time());
    }
    data.serial = h.serial();
    data.transmit_id = h.transmit_id();
    data.num_hor_phones = h.num_hor_phones();
    data.num_ver_phones = h.num_ver_phones();

    if (h.has_raw_timeseries()) {
        const auto& ts = h.raw_timeseries();
        data.dims.reserve(static_cast<size_t>(ts.dims_size()));
        for (int i = 0; i < ts.dims_size(); ++i) {
            data.dims.push_back(ts.dims(i));
        }
        if (ts.has_type()) {
            data.type = convert_array_type(ts.type());
        }
        if (ts.has_order()) {
            data.order = convert_array_order(ts.order());
        }
        if (ts.has_data()) {
            data.raw_timeseries = ts.data();
        }
    }
    return data;
}

TargetData convert_target_data(const proto::nav_api::TargetData& t) {
    TargetData data;
    if (t.has_time()) {
        data.time = convert_timestamp(t.time());
    }
    data.serial = t.serial();

    if (t.has_heading()) {
        data.heading = Heading{t.heading().heading()};
    }
    if (t.has_position()) {
        data.position = Position{t.position().lat(), t.position().lon()};
    }

    data.bottom.reserve(static_cast<size_t>(t.bottom_size()));
    for (const auto& bin : t.bottom()) {
        data.bottom.push_back(convert_bin(bin));
    }

    data.groups.reserve(static_cast<size_t>(t.groups_size()));
    for (const auto& group : t.groups()) {
        TargetGroup tg;
        tg.bins.reserve(static_cast<size_t>(group.bins_size()));
        for (const auto& bin : group.bins()) {
            tg.bins.push_back(convert_bin(bin));
        }
        data.groups.push_back(std::move(tg));
    }

    if (t.has_grid_description()) {
        data.grid_description = convert_grid_description(t.grid_description());
    }
    data.max_depth = t.max_depth();
    data.max_range_index = t.max_range_index();
    return data;
}

ProcessorSettings convert_processor_settings(
    const proto::nav_api::ProcessorSettings& s) {
    ProcessorSettings settings;
    if (s.has_time()) {
        settings.time = convert_timestamp(s.time());
    }
    settings.min_inwater_squelch = s.min_inwater_squelch();
    settings.max_inwater_squelch = s.max_inwater_squelch();
    settings.inwater_squelch = s.inwater_squelch();
    settings.squelchless_inwater_detector = s.squelchless_inwater_detector();
    settings.detect_bottom = s.detect_bottom();
    settings.system_type = convert_system_type(s.system_type());
    settings.fov = convert_fov(s.fov());
    return settings;
}

VesselInfo convert_vessel_info(const proto::nav_api::VesselInfo& v) {
    VesselInfo info;
    info.draft = v.draft();
    info.keel_offset = v.keel_offset();
    return info;
}

}  // namespace farsounder::detail
