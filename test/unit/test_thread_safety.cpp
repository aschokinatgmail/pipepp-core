#include <thread>
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
    pipe.add_stage([](basic_message<default_config>& m) { return true; })
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
    pipe.add_stage([](basic_message<default_config>& m) { return true; })
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