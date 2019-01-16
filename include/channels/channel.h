#ifndef CHANNELS_CHANNEL_H
#define CHANNELS_CHANNEL_H

#include <condition_variable>
#include <initializer_list>
#include <queue>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace chnl {

class ChannelError : public std::exception {
public:
    virtual ~ChannelError() = default;
};

class SendError : public ChannelError {
public:
    virtual ~SendError() = default;

    const char* what() const noexcept override {
        return "chnl::SendError";
    }
};

class RecvError : public ChannelError {
public:
    virtual ~RecvError() = default;

    const char* what() const noexcept override {
        return "chnl::RecvError";
    }
};

template <typename T>
class Sender;

template <typename T>
class Receiver;

template <typename T>
std::pair<Sender<T>, Receiver<T>> make_channel();

namespace detail {

template <typename T>
class Queue {
public:
    void push(const T &value) {
        emplace(value);
    }

    void push(T &&value) {
        emplace(std::move(value));
    }

    template <typename ...Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, int> = 0>
    void emplace(Ts &&...ts) {
        {
            std::unique_lock lck{ mtx_ };
            queue_.emplace(std::forward<Ts>(ts)...);
        }

        cv_.notify_one();
    }

    template <typename U, typename ...Ts, std::enable_if_t<
        std::is_constructible_v<T, std::initializer_list<U>&, Ts...>,
        int
    > = 0>
    void emplace(std::initializer_list<U> list, Ts &&...ts) {
        emplace(list, std::forward<Ts>(ts)...);
    }

    T pop() {
        std::unique_lock lck{ mtx_ };
        cv_.wait(lck, [this]() { return !queue_.empty(); });

        const T value{ std::move(queue_.back()) };
        queue_.pop();

        return value;
    }

    typename std::queue<T>::size_type size() const {
        std::shared_lock lck{ mtx_ };

        return queue_.size();
    }

    bool empty() const {
        std::shared_lock lck{ mtx_ };

        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    std::condition_variable_any cv_;
    std::shared_mutex mtx_;
};

template <typename T>
class SenderBase;

template <typename T>
class ReceiverBase;

template <typename T>
class SenderBase {
public:
    friend std::pair<Sender<T>, Receiver<T>> make_channel();

    void send(const T &value) {
        if (!receiver_ptr_->lock()) {
            throw SendError{ };
        }
    }

    void send(T &&value) {

    }

private:
    SenderBase() noexcept = default;

    std::weak_ptr<ReceiverBase<T>> receiver_ptr_;
};

template <typename T>
class ReceiverBase {
public:
    friend std::pair<Sender<T>, Receiver<T>> make_channel();
    friend SenderBase<T>;

    T recv() {
        if (!sender_ptr_->lock()) {
            throw RecvError{ };
        }

        std::unique_lock lck{ mtx_ };
        cv_.wait(lck, [this]() { return !queue_.empty() || !sender_ptr_.lock(); });

        if (!sender_ptr_->lock()) {
            throw RecvError{ };
        }

        const T value{ queue_.back() };
        queue_.pop();

        return value;
    }

private:
    ReceiverBase() noexcept = default;

    void send(const T &value) {
        send_emplace(value);
    }

    void send(T &&value) {
        send_emplace(std::move(value));
    }

    template <typename ...Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, int> = 0>
    void send_emplace(Ts &&...ts) {
        {
            std::unique_lock lck{ mtx_ };
            queue_.emplace(std::forward<Ts>(ts)...);
        }

        cv_.notify_one();
    }

    template <typename U, typename ...Ts, std::enable_if_t<
        std::is_constructible_v<T, std::initializer_list<U>&, Ts...>,
        int
    > = 0>
    void send_emplace(std::initializer_list<U> list, Ts &&...ts) {
        emplace(list, std::forward<Ts>(ts)...);
    }

    std::weak_ptr<SenderBase<T>> sender_ptr_;
    std::queue<T> queue_;
    std::condition_variable_any cv_;
    std::shared_mutex mtx_;
};

} // namespace detail

template <typename T>
std::pair<Sender<T>, Receiver<T>> make_channel() {
    const auto queue_ptr = std::make_shared<detail::Queue<T>>();

    const auto sender_ptr = std::make_shared<detail::SenderBase<T>>(queue_ptr);
    const auto receiver_ptr = std::make_shared<detail::ReceiverBase<T>>(queue_ptr);

    sender_ptr->receiver_ptr_ = receiver_ptr;
    receiver_ptr->sender_ptr_ = sender_ptr;

    return { Sender{ sender_ptr }, Receiver{ receiver_ptr } };
}

template <typename T>
class Channel {
public:
    friend Sender<T>;
    friend Receiver<T>;

    
};

template <typename T>
class Sender {
public:

private:
    Channel<T> &parent_;
};

template <typename T>
class Receiver {
public:

private:
    Channel<T> &parent_;
};

} // namespace chnl

#endif
