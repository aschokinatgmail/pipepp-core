#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/pipeline_ops.hpp>
#include "../helpers/test_sources.hpp"

#include <thread>
#include <chrono>

namespace pipepp::core {

struct PollingSource {
    bool connected = false;
    int poll_count = 0;
    int max_polls = 3;
    message_callback<default_config> callback;

    result connect(uri_view = {}) { connected = true; poll_count = 0; return {}; }
    result disconnect() { connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config> cb) { callback = std::move(cb); }
    void poll() {
        ++poll_count;
        if (callback && poll_count <= max_polls) {
            std::byte data[] = {std::byte{0x01}};
            message_view mv("poll/topic", data, 0);
            callback(mv);
        }
    }
};


TEST(RunEmitTest, RunReturnsOk) {
    basic_pipeline<PollingSource> pipe(PollingSource{});
    pipe.source().max_polls = 3;
    int sink_count = 0;
    pipe.set_sink(CountingSink{&sink_count});

    std::thread runner([&]() { pipe.run(); });

    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    pipe.stop();
    runner.join();

    EXPECT_GT(sink_count, 0);
}

TEST(RunEmitTest, RunResetsRunningOnExit) {
    basic_pipeline<SelfStoppingSource> pipe(SelfStoppingSource{});
    pipe.source().max_polls = 1;
    int sink_count = 0;
    pipe.set_sink(CountingSink{&sink_count});

    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_FALSE(pipe.is_running());
    EXPECT_GE(sink_count, 1);
}

TEST(RunEmitTest, RunReturnsCapacityError) {
    basic_pipeline<PollingSource> pipe(PollingSource{});
    for (int i = 0; i < static_cast<int>(default_config::max_stages) + 1; ++i) {
        pipe.add_stage(TrueStage{});
    }
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(RunEmitTest, EmitDeliversToSink) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::string received;
    pipe.set_sink([&](const message_view& mv) { received = std::string(mv.topic); });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("direct/emit", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(received, "direct/emit");
}

TEST(RunEmitTest, EmitWithMultipleStages) {
    basic_pipeline<MockSource> pipe(MockSource{});
    int stage1_count = 0;
    int stage2_count = 0;
    int sink_count = 0;

    pipe.add_stage([&](basic_message<default_config>&) { ++stage1_count; return true; });
    pipe.add_stage([&](basic_message<default_config>&) { ++stage2_count; return true; });
    pipe.set_sink(CountingSink{&sink_count});

    std::byte data[] = {std::byte{0x01}};
    message_view mv("multi/stage", data, 0);
    pipe.emit(mv);

    EXPECT_EQ(stage1_count, 1);
    EXPECT_EQ(stage2_count, 1);
    EXPECT_EQ(sink_count, 1);
}

TEST(RunEmitTest, StageFiltersMessage) {
    basic_pipeline<MockSource> pipe(MockSource{});
    int sink_count = 0;
    pipe.add_stage([](basic_message<default_config>&) { return false; });
    pipe.set_sink(CountingSink{&sink_count});

    std::byte data[] = {std::byte{0x01}};
    message_view mv("filtered", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 0);
}

TEST(RunEmitTest, MoveDoesNotCopyRunningState) {
    basic_pipeline<MockSource> pipe1(MockSource{});
    auto pipe2 = std::move(pipe1);
    EXPECT_FALSE(pipe2.is_running());
}

TEST(RunEmitTest, PollOnceReturnsFalseWhenNotRunning) {
    basic_pipeline<MockSource> pipe(MockSource{});
    EXPECT_FALSE(pipe.poll_once());
}

}
