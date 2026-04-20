#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/adapt.hpp>
#include "../helpers/test_sources.hpp"

#include <thread>
#include <vector>

using namespace pipepp::core;

TEST(PipelineCoverageTest, ConstructorTruncatedUriDefaultConfig) {
    std::string long_uri(300, 'a');
    basic_pipeline<MockSource> pipe(MockSource{}, long_uri);
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_uri);
}

TEST(PipelineCoverageTest, SubscribeBufferOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::string long_topic(300, 't');
    pipe.subscribe(long_topic, 0);
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

TEST(PipelineCoverageTest, SubscribeCapacityOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_subscriptions + 1; ++i) {
        char topic[] = "a/b";
        topic[0] = static_cast<char>('a' + static_cast<int>(i % 26));
        pipe.subscribe(topic, 0);
    }
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(PipelineCoverageTest, ConfigureCapacityOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_config_ops + 1; ++i) {
        pipe.configure(NoopConfigure{});
    }
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(PipelineCoverageTest, AddStageCapacityOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_stages + 1; ++i) {
        pipe.add_stage(TrueStage{});
    }
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(PipelineCoverageTest, RunFullHappyPath) {
    int config_applied = 0;
    int sink_count = 0;

    basic_pipeline<SelfStoppingSource> pipe(SelfStoppingSource{});
    pipe.source().max_polls = 2;
    pipe.configure([&](auto&) -> result { ++config_applied; return {}; })
        .subscribe("test/topic", 1)
        .add_stage(TrueStage{})
        .set_sink(CountingSink{&sink_count});

    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_FALSE(pipe.is_running());
    EXPECT_EQ(config_applied, 1);
    EXPECT_GE(sink_count, 1);
}

TEST(PipelineCoverageTest, RunPendingErrorFromTruncatedUri) {
    std::string long_uri(300, 'x');
    basic_pipeline<MockSource> pipe(MockSource{}, long_uri);
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_uri);
}

TEST(PipelineCoverageTest, RunConnectFailure) {
    FaultSource src;
    src.fail_connect = true;
    basic_pipeline<FaultSource> pipe(std::move(src));
    pipe.subscribe("test/topic", 0);
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::connection_failed);
    EXPECT_EQ(pipe.source().disconnect_count, 0);
}

TEST(PipelineCoverageTest, RunConfigOpFailure) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.configure([](auto&) -> result {
        return result{unexpect, make_unexpected(error_code::invalid_argument)};
    });
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_argument);
    EXPECT_EQ(pipe.source().disconnect_count, 1);
}

TEST(PipelineCoverageTest, RunSubscribeFailure) {
    FaultSource src;
    src.fail_subscribe = true;
    basic_pipeline<FaultSource> pipe(std::move(src));
    pipe.subscribe("test/topic", 0);
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_argument);
    EXPECT_EQ(pipe.source().disconnect_count, 1);
}

TEST(PipelineCoverageTest, PollOnceWhileRunning) {
    basic_pipeline<MockSource> pipe(MockSource{});

    std::thread runner([&]() { pipe.run(); });

    while (!pipe.is_running()) {
        std::this_thread::yield();
    }

    EXPECT_TRUE(pipe.poll_once());

    pipe.stop();
    runner.join();
}

TEST(PipelineCoverageTest, EmitOversizedPayload) {
    int sink_count = 0;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
       .set_sink(CountingSink{&sink_count});

    std::vector<std::byte> large_payload(default_config::max_payload_len + 1, std::byte{0xAA});
    message_view mv("topic", large_payload, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 0);
}

TEST(PipelineCoverageTest, EmitOversizedTopic) {
    int sink_count = 0;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
       .set_sink(CountingSink{&sink_count});

    std::string long_topic(300, 'x');
    std::byte data[] = {std::byte{0x01}};
    message_view mv(long_topic, data, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 0);
}

TEST(PipelineCoverageTest, EmitWithoutSinkNoCrash) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{});

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);
    pipe.emit(mv);
}

TEST(PipelineCoverageTest, CheckReturnsOk) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.subscribe("ok/topic", 0)
       .add_stage(TrueStage{});
    auto r = pipe.check();
    EXPECT_TRUE(r.has_value());
}

TEST(PipelineCoverageTest, ConnectUriValid) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.connect_uri("mqtt://host:1883/path");
    auto r = pipe.check();
    EXPECT_TRUE(r.has_value());
}

TEST(PipelineCoverageTest, ConnectUriTruncated) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::string long_uri(300, 'x');
    pipe.connect_uri(long_uri);
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_uri);
}

TEST(PipelineCoverageTest, MoveAssignment) {
    basic_pipeline<MockSource> pipe1(MockSource{});
    pipe1.subscribe("topic/a", 1)
        .add_stage(TrueStage{});

    basic_pipeline<MockSource> pipe2(MockSource{});
    pipe2 = std::move(pipe1);

    EXPECT_FALSE(pipe2.is_running());
    auto r = pipe2.check();
    EXPECT_TRUE(r.has_value());
}

TEST(PipelineCoverageTest, EventLoopCallbackAndPoll) {
    int sink_count = 0;
    basic_pipeline<SelfStoppingSource> pipe(SelfStoppingSource{});
    pipe.source().max_polls = 3;
    pipe.add_stage(TrueStage{})
       .set_sink(CountingSink{&sink_count});

    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_GE(sink_count, 1);
    EXPECT_FALSE(pipe.is_running());
}

TEST(PipelineCoverageTest, RangeSourcePollWithoutCallback) {
    std::byte data[] = {std::byte{0x01}};
    message_view mv("test/topic", data, 0);
    std::array<message_view, 1> arr{mv};
    auto src = adapt(arr);
    src.connect();
    src.poll();
    EXPECT_FALSE(src.is_connected());
}
