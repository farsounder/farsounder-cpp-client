#include "farsounder/subscriber.hpp"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <queue>
#include <stdexcept>
#include <zmq.hpp>

#include "conversions_internal.hpp"
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
