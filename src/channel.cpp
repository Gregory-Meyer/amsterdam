#include <channel.h>

namespace chnl::detail {

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
