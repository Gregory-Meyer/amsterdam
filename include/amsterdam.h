// MIT License
//
// Copyright (c) 2019 Gregory Meyer
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice (including
// the next paragraph) shall be included in all copies or substantial
// portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

/**
 *  @file amsterdam.h
 *
 *  Asynchronous channels for C++17.
 *
 *  Channels are manipulated through their Sender/Receiver components.
 *  Each thread using a Channel should have a Sender and/or Receiver to
 *  synchronize with other threads.
 *
 *  Channels are implemented as singly-linked lists, representing a
 *  FIFO queue. Synchronization is achieved through a shared mutex and
 *  condition variable.
 */

#ifndef AMSTERDAM_H
#define AMSTERDAM_H

#include <cassert>
#include <cstddef>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace amst {
namespace detail {

template <typename T>
class Channel;

} // namespace detail

#ifndef DOXYGEN_SHOULD_SKIP_THIS

template <typename T>
class Sender;

template <typename T>
class Receiver;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/** @returns An asynchronous sender/receiver pair. */
template <typename T>
std::pair<Sender<T>, Receiver<T>> channel();

/**
 *  Sender half of an asynchronous MPMC FIFO queue.
 *
 *  Senders *can* be shared between threads, but each thread *should*
 *  have its own Sender.
 */
template <typename T>
class Sender {
public:
    /**
     *  @returns A Sender that will transmit messages on the same
     *           channel as other.
     */
    Sender(const Sender &other) noexcept;

    ~Sender();

    /**
     *  @returns This Sender, which will now transmit messages on the
     *           same channel as other.
     */
    Sender& operator=(const Sender &other) noexcept;

    /**
     *  Copy constructs a message to send across this Sender's channel.
     *
     *  Messages are received in FIFO order (the order they were sent).
     *
     *  @throws SendError if all Receivers on this channel have been
     *          destroyed.
     *  @throws std::bad_alloc
     */
    void push(const T &elem);

    /**
     *  Move constructs a message to send across this Sender's channel.
     *
     *  Messages are received in FIFO order (the order they were sent).
     *
     *  @throws SendError if all Receivers on this channel have been
     *          destroyed.
     *  @throws std::bad_alloc
     */
    void push(T &&elem);

    /**
     *  Forwards all arguments to T's constructor, then sends that
     *  message across this Sender's channel.
     *
     *  Messages are received in FIFO order (the order they were sent).
     *
     *  @throws SendError if all Receivers on this channel have been
     *          destroyed.
     *  @throws std::bad_alloc
     */
    template <typename ...Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, int> = 0>
    void emplace(Ts &&...ts);

    template <typename U>
    friend std::pair<Sender<U>, Receiver<U>> channel();

private:
    explicit Sender(std::shared_ptr<detail::Channel<T>> channel) noexcept;

    std::shared_ptr<detail::Channel<T>> channel_;
};

/**
 *  Receiver half of an asynchronous MPMC FIFO queue.
 *
 *  Receivers *can* be shared between threads, but each thread *should*
 *  have its own Receivers.
 */
template <typename T>
class Receiver {
public:
    /**
     *  @returns A Receiver that will listen for messages on the same
     *           channel as other.
     */
    Receiver(const Receiver &other) noexcept;

    ~Receiver();

    /**
     *  @returns This Receiver, which will now listen for messages on
     *           the same channel as other.
     */
    Receiver& operator=(const Receiver &other) noexcept;

    /**
     *  Polls this Receiver's channel for available messages, returning
     *  it if one is available.
     *
     *  @returns The least-recently queued message sent on this
     *           Receiver's channel, if there was one.
     *
     *  @throws RecvError if all Senders on this channel have been
     *          destroyed.
     */
    std::optional<T> try_pop();

    /**
     *  Blocks until a new message is available on this Receiver's
     *  channel.
     *
     *  @returns The least-recently queued message sent on this
     *           Receiver's channel.
     *
     *  @throws RecvError if all Senders on this channel have been
     *          destroyed.
     */
    T pop();

    template <typename U>
    friend std::pair<Sender<U>, Receiver<U>> channel();

private:
    explicit Receiver(std::shared_ptr<detail::Channel<T>> channel) noexcept;

    std::shared_ptr<detail::Channel<T>> channel_;
};

/** Thrown by Sender if all Receiver on the channel are destroyed. */
class SendError : public std::exception {
public:
    virtual ~SendError() = default;

    const char* what() const noexcept {
        return "chnl::SendError: all receivers hung up";
    }
};

/** Thrown by Receiver if all Senders on the channel are destroyed. */
class RecvError : public std::exception {
public:
    virtual ~RecvError() = default;

    const char* what() const noexcept {
        return "chnl::RecvError: all senders hung up";
    }
};

namespace detail {

class ChannelBase {
public:
    struct Node {
        virtual ~Node() = default;

        Node *next = nullptr;
    };

    ~ChannelBase();

    std::unique_ptr<Node> try_pop();

    std::unique_ptr<Node> pop();

    void push(std::unique_ptr<Node> node);

    void connect_tx() noexcept;

    void connect_rx() noexcept;

    void disconnect_tx() noexcept;

    void disconnect_rx() noexcept;

private:
    std::unique_ptr<Node> locked_dequeue();

    std::mutex mtx_;
    std::condition_variable cv_;
    Node *first_ = nullptr;
    Node *last_ = nullptr;
    std::size_t tx_count_ = 1;
    std::size_t rx_count_ = 1;
};

template <typename T>
class Channel {
public:
    std::optional<T> try_pop() {
        const std::unique_ptr<Node> node(static_cast<Node*>(base_.try_pop().release()));

        if (!node) {
            return std::nullopt;
        }

        return std::move(node->elem);
    }

    T pop() {
        const std::unique_ptr<Node> node(static_cast<Node*>(base_.pop().release()));
        assert(node);

        return std::move(node->elem);
    }

    void push(const T &elem) {
        emplace(elem);
    }

    void push(T &&elem) {
        emplace(std::move(elem));
    }

    template <typename ...Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, int> = 0>
    void emplace(Ts &&...ts) {
        auto node = std::make_unique<Node>(std::forward<Ts>(ts)...);
        base_.push(std::move(node));
    }

    void connect_tx() noexcept {
        base_.connect_tx();
    }

    void connect_rx() noexcept {
        base_.connect_rx();
    }

    void disconnect_tx() noexcept {
        base_.disconnect_tx();
    }

    void disconnect_rx() noexcept {
        base_.disconnect_rx();
    }

private:
    struct Node : ChannelBase::Node {
        template <typename ...Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, int> = 0>
        explicit Node(Ts &&...ts) noexcept(std::is_nothrow_constructible_v<T, Ts...>)
        : elem(std::forward<Ts>(ts)...) { }

        virtual ~Node() = default;

        T elem;
    };

    ChannelBase base_;
};

} // namespace detail

template <typename T>
std::pair<Sender<T>, Receiver<T>> channel() {
    auto channel = std::make_shared<detail::Channel<T>>();

    Sender<T> tx(channel);
    Receiver<T> rx(std::move(channel));

    return {tx, rx};
}

template <typename T>
Sender<T>::Sender(const Sender &other) noexcept : channel_(other.channel_) {
    channel_->connect_tx();
}

template <typename T>
Sender<T>::~Sender() {
    channel_->disconnect_tx();
}

template <typename T>
Sender<T>& Sender<T>::operator=(const Sender &other) noexcept {
    if (channel_ == other.channel_) {
        return *this;
    }

    channel_->disconnect_tx();
    channel_ = other.channel_;
    channel_->connect_tx();

    return *this;
}

template <typename T>
void Sender<T>::push(const T &elem) {
    channel_->push(elem);
}

template <typename T>
void Sender<T>::push(T &&elem) {
    channel_->push(std::move(elem));
}

template <typename T>
template <typename ...Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, int>>
void Sender<T>::emplace(Ts &&...ts) {
    channel_->emplace(std::forward<Ts>(ts)...);
}

template <typename T>
Sender<T>::Sender(std::shared_ptr<detail::Channel<T>> channel) noexcept
: channel_(std::move(channel)) { }

template <typename T>
Receiver<T>::Receiver(const Receiver &other) noexcept : channel_(other.channel_) {
    channel_->connect_rx();
}

template <typename T>
Receiver<T>::~Receiver() {
    channel_->disconnect_rx();
}

template <typename T>
Receiver<T>& Receiver<T>::operator=(const Receiver &other) noexcept {
    if (channel_ == other.channel_) {
        return *this;
    }

    channel_->disconnect_rx();
    channel_ = other.channel_;
    channel_->connect_rx();

    return *this;
}

template <typename T>
std::optional<T> Receiver<T>::try_pop() {
    return channel_->try_pop();
}

template <typename T>
T Receiver<T>::pop() {
    return channel_->pop();
}

template <typename T>
Receiver<T>::Receiver(std::shared_ptr<detail::Channel<T>> channel) noexcept
: channel_(std::move(channel)) { }

} // namespace chnl

#endif
