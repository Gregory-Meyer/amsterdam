#include <channels/channel.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

template <typename T>
using ChannelPtr = std::shared_ptr<chnl::Channel<T>>;

std::mutex cout_mtx;

void consume(int tid, ChannelPtr<std::string> ch_ptr) {
    assert(ch_ptr);

    while (true) {
        const auto msg{ ch_ptr->pop() };

        std::scoped_lock lck{ cout_mtx };
        std::cout << "thread " << tid << " consumed message '" << msg << '\'' << std::endl;
    }
}

void produce(int tid, ChannelPtr<std::string> ch_ptr) {
    using namespace std::literals;

    assert(ch_ptr);

    int msg_idx = 0;

    while (true) {
        std::ostringstream oss;
        oss << tid << ": " << msg_idx;
        auto msg{ oss.str() };

        std::this_thread::sleep_for(1s);

        {
            std::scoped_lock lck{ cout_mtx };
            std::cout << "thread " << tid << " produced message '" << msg << '\'' << std::endl;
        }


        ch_ptr->push(std::move(msg));
    }
}

int main() {
    static constexpr std::size_t NUM_PRODUCERS = 4;
    static constexpr std::size_t NUM_CONSUMERS = 16;

    const auto ch_ptr = std::make_shared<chnl::Channel<std::string>>();

    std::vector<std::thread> producers;
    producers.reserve(NUM_PRODUCERS);

    std::vector<std::thread> consumers;
    consumers.reserve(NUM_CONSUMERS);

    int tid = 0;

    for (std::size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(produce, tid++, ch_ptr);
    }

    for (std::size_t i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consume, tid++, ch_ptr);
    }

    for (auto &producer : producers) {
        producer.join();
    }

    for (auto &consumer : consumers) {
        consumer.join();
    }
}
