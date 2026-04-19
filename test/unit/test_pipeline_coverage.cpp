#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include "../helpers/test_sources.hpp"

#include <thread>
#include <vector>

using namespace pipepp::core;

namespace {

struct FailingConnectSource {
    bool connected = false;
    int disconnect_count = 0;
    message_callback<default_config> callback;

    result connect(uri_view = {}) {
        return result{unexpect, make_unexpected(error_code::connection_failed)};
    }
    result disconnect() { ++disconnect_count; connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config> cb) { callback = std::move(cb); }
    void poll() {}
};

struct FailingSubscribeSource {
    bool connected = false;
    int disconnect_count = 0;
    message_callback<default_config> callback;

    result connect(uri_view = {}) { connected = true; return {}; }
    result disconnect() { ++disconnect_count; connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view, int) {
        return result{unexpect, make_unexpected(error_code::invalid_argument)};
    }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config> cb) { callback = std::move(cb); }
    void poll() {}
};

struct IdleSource {
    bool connected = false;
    message_callback<default_config> callback;

    result connect(uri_view = {}) { connected = true; return {}; }
    result disconnect() { connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config> cb) { callback = std::move(cb); }
    void poll() {}
};

} // namespace

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
        pipe.configure([](auto&) -> result { return {}; });
    }
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(PipelineCoverageTest, AddStageCapacityOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_stages + 1; ++i) {
        pipe.add_stage([](basic_message<default_config>&) { return true; });
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
        .add_stage([](basic_message<default_config>&) { return true; })
        .set_sink([&](const message_view&) { ++sink_count; });

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
    basic_pipeline<FailingConnectSource> pipe(FailingConnectSource{});
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
    basic_pipeline<FailingSubscribeSource> pipe(FailingSubscribeSource{});
    pipe.subscribe("test/topic", 0);
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::invalid_argument);
    EXPECT_EQ(pipe.source().disconnect_count, 1);
}

TEST(PipelineCoverageTest, PollOnceWhileRunning) {
    basic_pipeline<IdleSource> pipe(IdleSource{});

    std::thread runner([&]() { pipe.run(); });

    while (!pipe.is_running()) {
        std::this_thread::yield();
    }

    EXPECT_TRUE(pipe.poll_once());

    pipe.stop();
    runner.join();
}

TEST(PipelineCoverageTest, EmitOversizedPayload) {
    bool sink_called = false;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage([](basic_message<default_config>&) { return true; })
       .set_sink([&](const message_view&) { sink_called = true; });

    std::vector<std::byte> large_payload(default_config::max_payload_len + 1, std::byte{0xAA});
    message_view mv("topic", large_payload, 0);
    pipe.emit(mv);
    EXPECT_FALSE(sink_called);
}

TEST(PipelineCoverageTest, EmitOversizedTopic) {
    bool sink_called = false;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage([](basic_message<default_config>&) { return true; })
       .set_sink([&](const message_view&) { sink_called = true; });

    std::string long_topic(300, 'x');
    std::byte data[] = {std::byte{0x01}};
    message_view mv(long_topic, data, 0);
    pipe.emit(mv);
    EXPECT_FALSE(sink_called);
}

TEST(PipelineCoverageTest, EmitWithoutSinkNoCrash) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage([](basic_message<default_config>&) { return true; });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);
    pipe.emit(mv);
}

TEST(PipelineCoverageTest, CheckReturnsOk) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.subscribe("ok/topic", 0)
       .add_stage([](basic_message<default_config>&) { return true; });
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
        .add_stage([](basic_message<default_config>&) { return true; });

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
    pipe.add_stage([](basic_message<default_config>&) { return true; })
       .set_sink([&](const message_view&) { ++sink_count; });

    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_GE(sink_count, 1);
    EXPECT_FALSE(pipe.is_running());
}
