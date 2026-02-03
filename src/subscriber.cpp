#include "farsounder/subscriber.hpp"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <queue>
#include <stdexcept>
#include <zmq.hpp>

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

Subscriber::Subscriber(config::ClientConfig config)
    : config_(std::move(config)), running_(false) {}

Subscriber::~Subscriber() {
    stop();
}

void Subscriber::on(config::PubSubMessage message,
                    HydrophoneCallback callback) {
    if (message != config::PubSubMessage::HydrophoneData) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(callback_mutex_);
    hydrophone_callbacks_.push_back(std::move(callback));
}

void Subscriber::on(config::PubSubMessage message, TargetCallback callback) {
    if (message != config::PubSubMessage::TargetData) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(callback_mutex_);
    target_callbacks_.push_back(std::move(callback));
}

void Subscriber::on(config::PubSubMessage message,
                    ProcessorSettingsCallback callback) {
    if (message != config::PubSubMessage::ProcessorSettings) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(callback_mutex_);
    processor_settings_callbacks_.push_back(std::move(callback));
}

void Subscriber::on(config::PubSubMessage message,
                    VesselInfoCallback callback) {
    if (message != config::PubSubMessage::VesselInfo) {
        throw std::invalid_argument(
            "Callback type does not match pub-sub message");
    }
    std::lock_guard<std::mutex> lock(callback_mutex_);
    vessel_info_callbacks_.push_back(std::move(callback));
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
    if (running_) {
        throw std::runtime_error("Subscriber already running");
    }
    running_ = true;
    worker_ = std::thread([this]() { run_loop(); });
}

void Subscriber::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void Subscriber::run_loop() {
    zmq::context_t context(1);
    std::vector<SocketEntry> sockets;

    for (auto message : config_.subscribe) {
        zmq::socket_t socket(context, zmq::socket_type::sub);
        socket.set(zmq::sockopt::subscribe, "");
        socket.connect(pubsub_address(config_, message));
        sockets.push_back(SocketEntry{message, std::move(socket)});
    }

    std::unique_ptr<ThreadPool> pool;
    if (config_.callback_executor == config::CallbackExecutor::ThreadPool) {
        pool = std::make_unique<ThreadPool>(2);
    }

    while (running_) {
        std::vector<zmq::pollitem_t> items;
        items.reserve(sockets.size());
        for (auto& entry : sockets) {
            items.push_back(zmq::pollitem_t{entry.socket, 0, ZMQ_POLLIN, 0});
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
                auto data = std::make_shared<proto::nav_api::HydrophoneData>();
                if (!data->ParseFromArray(msg.data(),
                                          static_cast<int>(msg.size()))) {
                    break;
                }
                if (pool) {
                    pool->enqueue([this, data]() { dispatch_message(*data); });
                } else {
                    dispatch_message(*data);
                }
                break;
            }
            case config::PubSubMessage::TargetData: {
                auto data = std::make_shared<proto::nav_api::TargetData>();
                if (!data->ParseFromArray(msg.data(),
                                          static_cast<int>(msg.size()))) {
                    break;
                }
                if (pool) {
                    pool->enqueue([this, data]() { dispatch_message(*data); });
                } else {
                    dispatch_message(*data);
                }
                break;
            }
            case config::PubSubMessage::ProcessorSettings: {
                auto data =
                    std::make_shared<proto::nav_api::ProcessorSettings>();
                if (!data->ParseFromArray(msg.data(),
                                          static_cast<int>(msg.size()))) {
                    break;
                }
                if (pool) {
                    pool->enqueue([this, data]() { dispatch_message(*data); });
                } else {
                    dispatch_message(*data);
                }
                break;
            }
            case config::PubSubMessage::VesselInfo: {
                auto data = std::make_shared<proto::nav_api::VesselInfo>();
                if (!data->ParseFromArray(msg.data(),
                                          static_cast<int>(msg.size()))) {
                    break;
                }
                if (pool) {
                    pool->enqueue([this, data]() { dispatch_message(*data); });
                } else {
                    dispatch_message(*data);
                }
                break;
            }
            }
        }
    }
}

void Subscriber::dispatch_message(const proto::nav_api::HydrophoneData& data) {
    std::vector<HydrophoneCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = hydrophone_callbacks_;
    }
    for (const auto& callback : callbacks) {
        callback(data);
    }
}

void Subscriber::dispatch_message(const proto::nav_api::TargetData& data) {
    std::vector<TargetCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = target_callbacks_;
    }
    for (const auto& callback : callbacks) {
        callback(data);
    }
}

void Subscriber::dispatch_message(
    const proto::nav_api::ProcessorSettings& data) {
    std::vector<ProcessorSettingsCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = processor_settings_callbacks_;
    }
    for (const auto& callback : callbacks) {
        callback(data);
    }
}

void Subscriber::dispatch_message(const proto::nav_api::VesselInfo& data) {
    std::vector<VesselInfoCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = vessel_info_callbacks_;
    }
    for (const auto& callback : callbacks) {
        callback(data);
    }
}

Subscriber subscribe(const config::ClientConfig& config) {
    return Subscriber(config);
}

}  // namespace farsounder
