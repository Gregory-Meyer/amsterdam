e#include <channel.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

#include <catch2/catch.hpp>

TEST_CASE("push/pop") {
    auto [tx, rx] = chnl::channel<int>();

    tx.push(5);
    REQUIRE(rx.pop() == 5);
}

TEST_CASE("push/pop: shared tx") {
    auto [tx, rx] = chnl::channel<int>();
    const chnl::Sender<int> tx2 = tx;

    tx.push(5);
    REQUIRE(rx.pop() == 5);
}

TEST_CASE("push/pop: shared rx") {
    auto [tx, rx] = chnl::channel<int>();
    const chnl::Receiver<int> rx2 = rx;

    tx.push(5);
    REQUIRE(rx.pop() == 5);
}

TEST_CASE("push/pop: shared tx/rx") {
    auto [tx, rx] = chnl::channel<int>();
    const chnl::Sender<int> tx2 = tx;
    const chnl::Receiver<int> rx2 = rx;

    tx.push(5);
    REQUIRE(rx.pop() == 5);
}

TEST_CASE("push: rx hung up") {
    chnl::Sender<int> tx = chnl::channel<int>().first;

    REQUIRE_THROWS_AS(tx.push(5), chnl::SendError);
}

TEST_CASE("push: rx hung up, shared tx") {
    chnl::Sender<int> tx = chnl::channel<int>().first;
    const chnl::Sender<int> tx2 = tx;

    REQUIRE_THROWS_AS(tx.push(5), chnl::SendError);
}

TEST_CASE("pop: tx hung up") {
    chnl::Receiver<int> rx = chnl::channel<int>().second;

    REQUIRE_THROWS_AS(rx.pop(), chnl::RecvError);
}

TEST_CASE("pop: tx hung up, shared rx") {
    chnl::Receiver<int> rx = chnl::channel<int>().second;
    const chnl::Receiver<int> rx2 = rx;

    REQUIRE_THROWS_AS(rx.pop(), chnl::RecvError);
}

TEST_CASE("push/try_pop") {
    auto [tx, rx] = chnl::channel<int>();

    tx.push(5);
    const auto received = rx.try_pop();
    REQUIRE(received);
    REQUIRE(*received == 5);
}

TEST_CASE("push/try_pop: shared tx") {
    auto [tx, rx] = chnl::channel<int>();
    const chnl::Sender<int> tx2 = tx;

    tx.push(5);
    const auto received = rx.try_pop();
    REQUIRE(received);
    REQUIRE(*received == 5);
}

TEST_CASE("push/try_pop: shared rx") {
    auto [tx, rx] = chnl::channel<int>();
    const chnl::Receiver<int> rx2 = rx;

    tx.push(5);
    const auto received = rx.try_pop();
    REQUIRE(received);
    REQUIRE(*received == 5);
}

TEST_CASE("push/try_pop: shared tx/rx") {
    auto [tx, rx] = chnl::channel<int>();
    const chnl::Sender<int> tx2 = tx;
    const chnl::Receiver<int> rx2 = rx;

    tx.push(5);
    const auto received = rx.try_pop();
    REQUIRE(received);
    REQUIRE(*received == 5);
}

TEST_CASE("try_pop: empty") {
    auto [tx, rx] = chnl::channel<int>();

    const auto received = rx.try_pop();
    REQUIRE(!received);
}

TEST_CASE("try_pop: tx hung up") {
    chnl::Receiver<int> rx = chnl::channel<int>().second;

    REQUIRE_THROWS_AS(rx.try_pop(), chnl::RecvError);
}

TEST_CASE("delete non-empty channel") {
    auto [tx, rx] = chnl::channel<std::string>();

    tx.emplace("foo bar");
}

TEST_CASE("push/pop: fifo ordering") {
    auto [tx, rx] = chnl::channel<int>();

    tx.push(5);
    tx.push(10);

    REQUIRE(rx.pop() == 5);
    REQUIRE(rx.pop() == 10);
}

TEST_CASE("push/pop: tx in another thread") {
    auto [tx, rx] = chnl::channel<int>();

    std::thread([tx = tx]() mutable {
        tx.push(5);
    }).join();

    REQUIRE(rx.pop() == 5);
}

TEST_CASE("stress") {
    auto [tx, rx] = chnl::channel<int>();

    for (int i = 0; i < 1024; ++i) {
        tx.push(i);
    }

    for (int i = 0; i < 1024; ++i) {
        REQUIRE(rx.pop() == i);
    }
}

TEST_CASE("stress: multithreaded") {
    auto [tx, rx] = chnl::channel<int>();

    std::thread t([tx = tx]() mutable {
        for (int i = 0; i < 1024; ++i) {
            tx.push(i);
        }
    });

    for (int i = 0; i < 1024; ++i) {
        REQUIRE(rx.pop() == i);
    }

    t.join();
}
