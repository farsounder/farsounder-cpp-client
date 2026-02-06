#include "farsounder/subscriber.hpp"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <memory>
#include <queue>
#include <stdexcept>
#include <zmq.hpp>

#include "proto/array.pb.h"
#include "proto/nav_api.pb.h"
#include "subscriber_internal.hpp"

namespace farsounder {
namespace {

class ThreadPool {
   public:
    explicit ThreadPool(std::size_t threads) : stop_(false) {
        if (threads == 0) {
            threads = 1;
        }
        for (std::size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this]() { worker_loop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename Fn>
    void enqueue(Fn&& fn) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.emplace(std::forward<Fn>(fn));
        }
        cv_.notify_one();
    }

   private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_;
};

struct SocketEntry {
    config::PubSubMessage type;
    zmq::socket_t socket;
};

std::string pubsub_address(const config::ClientConfig& cfg,
                           config::PubSubMessage message) {
    auto port = config::resolve_pubsub_port(cfg, message);
    return "tcp://" + cfg.host + ":" + std::to_string(port);
}

}  // namespace

// =============================================================================
// Proto to wrapper type conversions
// =============================================================================

namespace detail {

Timestamp convert_timestamp(const proto::time::Time& t) {
    // Convert year/month/day/hour/minute/second/millisecond to epoch
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

HydrophoneData convert_hydrophone_data(
    const proto::nav_api::HydrophoneData& h) {
    HydrophoneData data;
    if (h.has_time()) {
        data.time = detail::convert_timestamp(h.time());
    }
    data.serial = h.serial();
    data.transmit_id = h.transmit_id();
    data.num_hor_phones = h.num_hor_phones();
    data.num_ver_phones = h.num_ver_phones();

    // Convert raw timeseries if present
    if (h.has_raw_timeseries()) {
        const auto& ts = h.raw_timeseries();
        data.dims.reserve(static_cast<size_t>(ts.dims_size()));
        for (int i = 0; i < ts.dims_size(); ++i) {
            data.dims.push_back(ts.dims(i));
        }
        if (ts.has_type()) {
            data.type = detail::convert_array_type(ts.type());
        }
        if (ts.has_order()) {
            data.order = detail::convert_array_order(ts.order());
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
        data.time = detail::convert_timestamp(t.time());
    }
    data.serial = t.serial();

    if (t.has_heading()) {
        data.heading = Heading{t.heading().heading()};
    }
    if (t.has_position()) {
        data.position = Position{t.position().lat(), t.position().lon()};
    }

    // Convert bottom bins
    data.bottom.reserve(static_cast<size_t>(t.bottom_size()));
    for (const auto& bin : t.bottom()) {
        data.bottom.push_back(detail::convert_bin(bin));
    }

    // Convert target groups
    data.groups.reserve(static_cast<size_t>(t.groups_size()));
    for (const auto& group : t.groups()) {
        TargetGroup tg;
        tg.bins.reserve(static_cast<size_t>(group.bins_size()));
        for (const auto& bin : group.bins()) {
            tg.bins.push_back(detail::convert_bin(bin));
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
        settings.time = detail::convert_timestamp(s.time());
    }
    settings.min_inwater_squelch = s.min_inwater_squelch();
    settings.max_inwater_squelch = s.max_inwater_squelch();
    settings.inwater_squelch = s.inwater_squelch();
    settings.squelchless_inwater_detector = s.squelchless_inwater_detector();
    settings.detect_bottom = s.detect_bottom();
    settings.system_type = detail::convert_system_type(s.system_type());
    settings.fov = detail::convert_fov(s.fov());
    return settings;
}

VesselInfo convert_vessel_info(const proto::nav_api::VesselInfo& v) {
    VesselInfo info;
    info.draft = v.draft();
    info.keel_offset = v.keel_offset();
    return info;
}

}  // namespace detail

// =============================================================================
// Subscriber implementation (PIMPL)
// =============================================================================

struct Subscriber::Impl {
    config::ClientConfig config;
    std::thread worker;
    std::atomic<bool> running{false};

    std::mutex callback_mutex;
    std::vector<HydrophoneCallback> hydrophone_callbacks;
    std::vector<TargetCallback> target_callbacks;
    std::vector<ProcessorSettingsCallback> processor_settings_callbacks;
    std::vector<VesselInfoCallback> vessel_info_callbacks;

    void dispatch_message(const HydrophoneData& data) {
        std::vector<HydrophoneCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex);
            callbacks = hydrophone_callbacks;
        }
        for (const auto& callback : callbacks) {
            callback(data);
        }
    }

    void dispatch_message(const TargetData& data) {
        std::vector<TargetCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex);
            callbacks = target_callbacks;
        }
        for (const auto& callback : callbacks) {
            callback(data);
        }
    }

    void dispatch_message(const ProcessorSettings& data) {
        std::vector<ProcessorSettingsCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex);
            callbacks = processor_settings_callbacks;
        }
        for (const auto& callback : callbacks) {
            callback(data);
        }
    }

    void dispatch_message(const VesselInfo& data) {
        std::vector<VesselInfoCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(callback_mutex);
            callbacks = vessel_info_callbacks;
        }
        for (const auto& callback : callbacks) {
            callback(data);
        }
    }

    void run_loop() {
        zmq::context_t context(1);
        std::vector<SocketEntry> sockets;

        for (auto message : config.subscribe) {
            zmq::socket_t socket(context, zmq::socket_type::sub);
            socket.set(zmq::sockopt::subscribe, "");
            socket.connect(pubsub_address(config, message));
            sockets.push_back(SocketEntry{message, std::move(socket)});
        }

        std::unique_ptr<ThreadPool> pool;
        if (config.callback_executor == config::CallbackExecutor::ThreadPool) {
            pool = std::make_unique<ThreadPool>(2);
        }

        while (running) {
            std::vector<zmq::pollitem_t> items;
            items.reserve(sockets.size());
            for (auto& entry : sockets) {
                items.push_back(
                    zmq::pollitem_t{entry.socket, 0, ZMQ_POLLIN, 0});
            }

            zmq::poll(items, std::chrono::milliseconds(100));

            for (std::size_t i = 0; i < items.size(); ++i) {
                if (!(items[i].revents & ZMQ_POLLIN)) {
                    continue;
                }

                zmq::message_t msg;
                auto recv_result =
                    sockets[i].socket.recv(msg, zmq::recv_flags::none);
                if (!recv_result) {
                    continue;
                }

                switch (sockets[i].type) {
                case config::PubSubMessage::HydrophoneData: {
                    proto::nav_api::HydrophoneData proto_data;
                    if (!proto_data.ParseFromArray(
                            msg.data(), static_cast<int>(msg.size()))) {
                        break;
                    }
                    auto data = std::make_shared<HydrophoneData>(
                        detail::convert_hydrophone_data(proto_data));
                    if (pool) {
                        pool->enqueue(
                            [this, data]() { dispatch_message(*data); });
                    } else {
                        dispatch_message(*data);
                    }
                    break;
                }
                case config::PubSubMessage::TargetData: {
                    proto::nav_api::TargetData proto_data;
                    if (!proto_data.ParseFromArray(
                            msg.data(), static_cast<int>(msg.size()))) {
                        break;
                    }
                    auto data = std::make_shared<TargetData>(
                        detail::convert_target_data(proto_data));
                    if (pool) {
                        pool->enqueue(
                            [this, data]() { dispatch_message(*data); });
                    } else {
                        dispatch_message(*data);
                    }
                    break;
                }
                case config::PubSubMessage::ProcessorSettings: {
                    proto::nav_api::ProcessorSettings proto_data;
                    if (!proto_data.ParseFromArray(
                            msg.data(), static_cast<int>(msg.size()))) {
                        break;
                    }
                    auto data = std::make_shared<ProcessorSettings>(
                        detail::convert_processor_settings(proto_data));
                    if (pool) {
                        pool->enqueue(
                            [this, data]() { dispatch_message(*data); });
                    } else {
                        dispatch_message(*data);
                    }
                    break;
                }
                case config::PubSubMessage::VesselInfo: {
                    proto::nav_api::VesselInfo proto_data;
                    if (!proto_data.ParseFromArray(
                            msg.data(), static_cast<int>(msg.size()))) {
                        break;
                    }
                    auto data = std::make_shared<VesselInfo>(
                        detail::convert_vessel_info(proto_data));
                    if (pool) {
                        pool->enqueue(
                            [this, data]() { dispatch_message(*data); });
                    } else {
                        dispatch_message(*data);
                    }
                    break;
                }
                }
            }
        }
    }
};

Subscriber::Subscriber(config::ClientConfig config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = std::move(config);
}

Subscriber::~Subscriber() {
    stop();
}

Subscriber::Subscriber(Subscriber&&) noexcept = default;
Subscriber& Subscriber::operator=(Subscriber&&) noexcept = default;

void Subscriber::on(config::PubSubMessage message,
                    HydrophoneCallback callback) {
    if (message != config::PubSubMessage::HydrophoneData) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(impl_->callback_mutex);
    impl_->hydrophone_callbacks.push_back(std::move(callback));
}

void Subscriber::on(config::PubSubMessage message, TargetCallback callback) {
    if (message != config::PubSubMessage::TargetData) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(impl_->callback_mutex);
    impl_->target_callbacks.push_back(std::move(callback));
}

void Subscriber::on(config::PubSubMessage message,
                    ProcessorSettingsCallback callback) {
    if (message != config::PubSubMessage::ProcessorSettings) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(impl_->callback_mutex);
    impl_->processor_settings_callbacks.push_back(std::move(callback));
}

void Subscriber::on(config::PubSubMessage message,
                    VesselInfoCallback callback) {
    if (message != config::PubSubMessage::VesselInfo) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(impl_->callback_mutex);
    impl_->vessel_info_callbacks.push_back(std::move(callback));
}

void Subscriber::on(const std::string& message_name,
                    HydrophoneCallback callback) {
    on(config::pubsub_from_name(message_name), std::move(callback));
}

void Subscriber::on(const std::string& message_name, TargetCallback callback) {
    on(config::pubsub_from_name(message_name), std::move(callback));
}

void Subscriber::on(const std::string& message_name,
                    ProcessorSettingsCallback callback) {
    on(config::pubsub_from_name(message_name), std::move(callback));
}

void Subscriber::on(const std::string& message_name,
                    VesselInfoCallback callback) {
    on(config::pubsub_from_name(message_name), std::move(callback));
}

void Subscriber::start() {
    if (impl_->running) {
        throw std::runtime_error("Subscriber already running");
    }
    impl_->running = true;
    impl_->worker = std::thread([this]() { impl_->run_loop(); });
}

void Subscriber::stop() {
    if (!impl_ || !impl_->running) {
        return;
    }
    impl_->running = false;
    if (impl_->worker.joinable()) {
        impl_->worker.join();
    }
}

Subscriber subscribe(const config::ClientConfig& config) {
    return Subscriber(config);
}

}  // namespace farsounder
