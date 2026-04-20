#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/pipeline_ops.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

static_assert(BusSource<MockSource, default_config>, "MockSource must satisfy BusSource");

TEST(PipelineTest, BasicConstruction) {
    basic_pipeline<MockSource> pipe(MockSource{});
    EXPECT_FALSE(pipe.is_running());
}

TEST(PipelineTest, MethodChaining) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
        .set_sink(NoopSink{});
}

TEST(PipelineTest, SubscribeMethod) {
    auto pipe = basic_pipeline<MockSource>(MockSource{});
    pipe.subscribe("test/topic", 1);
}

TEST(PipelineTest, ConfigureMethod) {
    int configured = 0;
    auto pipe = basic_pipeline<MockSource>(MockSource{});
    pipe.configure([&configured](auto&) -> result { ++configured; return {}; });
}

TEST(PipelineTest, PipeSyntaxTransform) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | transform(TrueStage{})
        | sink(NoopSink{});
}

TEST(PipelineTest, PipeSyntaxFilter) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | filter(TrueFilter{})
        | sink(NoopSink{});
}

TEST(PipelineTest, PipeSyntaxSubscribe) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | subscribe("test/topic", 0)
        | sink(NoopSink{});
}

TEST(PipelineTest, PipeSyntaxConfigure) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | configure(NoopConfigure{})
        | sink(NoopSink{});
}

TEST(PipelineTest, PipeSyntaxChaining) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | subscribe("sensors/#", 0)
        | transform(TrueStage{})
        | filter(TrueFilter{})
        | sink(NoopSink{});
}

TEST(PipelineTest, EmitMessage) {
    bool sink_called = false;
    std::string received_topic;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view& mv) {
            sink_called = true;
            received_topic = std::string(mv.topic);
        });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("emit/topic", data, 0);
    pipe.emit(mv);

    EXPECT_TRUE(sink_called);
    EXPECT_EQ(received_topic, "emit/topic");
}

TEST(PipelineTest, FilterDropsMessage) {
    int sink_count = 0;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage([](basic_message<default_config>& m) { return false; })
        .set_sink(CountingSink{&sink_count});

    std::byte data[] = {std::byte{0x01}};
    message_view mv("filtered", data, 0);
    pipe.emit(mv);

    EXPECT_EQ(sink_count, 0);
}

TEST(PipelineTest, CapacityOverflowDetectedByRun) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (int i = 0; i < static_cast<int>(default_config::max_stages) + 1; ++i) {
        pipe.add_stage(TrueStage{});
    }
    auto check_result = pipe.check();
    EXPECT_FALSE(check_result.has_value());
    EXPECT_EQ(check_result.error(), error_code::capacity_exceeded);
}

TEST(PipelineTest, NoErrorWithinCapacity) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage(TrueStage{});
    auto check_result = pipe.check();
    EXPECT_TRUE(check_result.has_value());
}

TEST(PipelineTest, PollOnceReturnsFalseWhenNotRunning) {
    basic_pipeline<MockSource> pipe(MockSource{});
    EXPECT_FALSE(pipe.poll_once());
}

TEST(PipelineTest, ConnectUriStoresOwnedCopy) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.connect_uri("mqtt://broker:1883/topic");
    auto check_result = pipe.check();
    EXPECT_TRUE(check_result.has_value());
}

TEST(PipelineTest, ConnectUriTruncatedReturnsInvalidUri) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::string long_uri(512, 'x');
    pipe.connect_uri(long_uri);
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_uri);
}

TEST(PipelineTest, ConnectUriTruncatedBlocksRun) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::string long_uri(512, 'x');
    pipe.connect_uri(long_uri);
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_uri);
}

struct tiny_pipe_config : default_config {
    static constexpr std::size_t max_uri_len = 8;
};

TEST(PipelineTest, ConstructorWithTruncatedUriReturnsInvalidUri) {
    basic_pipeline<MockSource, tiny_pipe_config> pipe(MockSource{}, "mqtt://very-long-host:1883/topic");
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_uri);
}

TEST(PipelineTest, ConstructorWithTruncatedUriBlocksRun) {
    basic_pipeline<MockSource, tiny_pipe_config> pipe(MockSource{}, "mqtt://very-long-host:1883/topic");
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_uri);
}

TEST(PipelineTest, ConstructorWithValidUriNoError) {
    basic_pipeline<MockSource, tiny_pipe_config> pipe(MockSource{}, "a://b");
    auto r = pipe.check();
    EXPECT_TRUE(r.has_value());
}