#include <thread>
#include <vector>
#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/message.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(ThreadSafetyTest, AtomicStop) {
    basic_pipeline<MockSource> pipe(MockSource{});
    EXPECT_FALSE(pipe.is_running());
    pipe.stop();
    EXPECT_FALSE(pipe.is_running());
}

TEST(ThreadSafetyTest, ConcurrentEmitStop) {
    bool sink_called = false;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view&) { sink_called = true; });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);

    std::thread emitter([&]() {
        for (int i = 0; i < 100; ++i) {
            pipe.emit(mv);
        }
    });

    std::thread stopper([&]() {
        pipe.stop();
    });

    emitter.join();
    stopper.join();
}

TEST(ThreadSafetyTest, EmitFromMultipleThreads) {
    std::atomic<int> count{0};
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view&) { count.fetch_add(1, std::memory_order_relaxed); });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);

    std::thread t1([&]() { for (int i = 0; i < 50; ++i) pipe.emit(mv); });
    std::thread t2([&]() { for (int i = 0; i < 50; ++i) pipe.emit(mv); });

    t1.join();
    t2.join();

    EXPECT_EQ(count.load(std::memory_order_acquire), 100);
}

TEST(ThreadSafetyTest, PipelineWithUriView) {
    basic_pipeline<MockSource> pipe(MockSource{}, "");
    EXPECT_FALSE(pipe.is_running());
}

TEST(ThreadSafetyTest, CallbackDispatchAcrossThreads) {
    std::atomic<int> count{0};
    basic_pipeline<FullSource> pipe(FullSource{});
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view&) { count.fetch_add(1, std::memory_order_relaxed); });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);

    pipe.source().set_message_callback([&pipe](const message_view& msg) {
        pipe.emit(msg);
    });

    std::thread callback_thread([&]() {
        for (int i = 0; i < 50; ++i) {
            if (pipe.source().callback) {
                pipe.source().callback(mv);
            }
        }
    });

    std::thread emit_thread([&]() {
        for (int i = 0; i < 50; ++i) {
            pipe.emit(mv);
        }
    });

    callback_thread.join();
    emit_thread.join();

    EXPECT_EQ(count.load(std::memory_order_acquire), 100);
}

TEST(ThreadSafetyTest, EmitWhileRunInEventLoop) {
    std::atomic<int> count{0};
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view&) { count.fetch_add(1, std::memory_order_relaxed); });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);

    std::thread run_thread([&]() {
        pipe.run();
    });

    while (!pipe.is_running()) {
        std::this_thread::yield();
    }

    for (int i = 0; i < 100; ++i) {
        pipe.emit(mv);
    }

    pipe.stop();
    run_thread.join();

    EXPECT_EQ(count.load(std::memory_order_acquire), 100);
}

TEST(ThreadSafetyTest, StressManyThreadsEmit) {
    std::atomic<int> count{0};
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view&) { count.fetch_add(1, std::memory_order_relaxed); });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);

    constexpr int num_threads = 8;
    constexpr int emits_per_thread = 200;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < emits_per_thread; ++i) {
                pipe.emit(mv);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(count.load(std::memory_order_acquire), num_threads * emits_per_thread);
}
