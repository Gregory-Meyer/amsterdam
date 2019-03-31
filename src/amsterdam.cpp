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

#include <amsterdam.h>

namespace amst::detail {

ChannelBase::~ChannelBase() {
    Node *current = first_;

    while (current) {
        Node *const next = current->next;
        delete current;
        current = next;
    }
}

std::unique_ptr<ChannelBase::Node> ChannelBase::try_pop() {
    const std::scoped_lock guard(mtx_);

    return locked_dequeue();
}

std::unique_ptr<ChannelBase::Node> ChannelBase::pop() {
    std::unique_lock guard(mtx_);
    cv_.wait(guard, [this] { return first_ || tx_count_ == 0; });

    std::unique_ptr<Node> node = locked_dequeue();
    assert(node);

    return node;
}

void ChannelBase::push(std::unique_ptr<Node> node) {
    assert(node);

    {
        const std::scoped_lock guard(mtx_);

        if (rx_count_ == 0) {
            throw SendError();
        }

        if (last_) {
            last_->next = node.get();
        } else {
            first_ = node.get();
        }

        last_ = node.release();
    }

    cv_.notify_one();
}

void ChannelBase::connect_tx() noexcept {
    const std::scoped_lock guard(mtx_);

    assert(tx_count_ > 0);
    ++tx_count_;
}

void ChannelBase::connect_rx() noexcept {
    const std::scoped_lock guard(mtx_);

    assert(rx_count_ > 0);
    ++rx_count_;
}

void ChannelBase::disconnect_tx() noexcept {
    const std::scoped_lock guard(mtx_);

    assert(tx_count_ > 0);

    --tx_count_;

    if (tx_count_ == 0) { // now zero
        cv_.notify_all();
    }
}

void ChannelBase::disconnect_rx() noexcept {
    const std::scoped_lock guard(mtx_);

    assert(rx_count_ > 0);

    --rx_count_;
}

std::unique_ptr<ChannelBase::Node> ChannelBase::locked_dequeue() {
    if (!first_) {
        if (tx_count_ == 0) {
            throw RecvError();
        }

        return nullptr;
    }

    std::unique_ptr<Node> node(first_);
    first_ = first_->next;

    if (!first_) {
        last_ = nullptr;
    }

    return node;
}

} // namespace chnl::detail
