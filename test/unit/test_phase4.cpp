#include <gtest/gtest.h>
#include <pipepp/core/adapt.hpp>
#include <pipepp/core/as_range.hpp>
#include <pipepp/core/connect_to.hpp>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/pipeline_ops.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(AdaptTest, RangeSourceSatisfiesBusSource) {
    static_assert(BusSource<range_source<const message_view*>, default_config>,
                  "range_source must satisfy BusSource");
}

TEST(AdaptTest, AdaptContainerOfMessages) {
    std::byte data1[] = {std::byte{0x01}};
    std::byte data2[] = {std::byte{0x02}};
    message_view msgs[] = {
        message_view("topic/a", data1, 0),
        message_view("topic/b", data2, 1)
    };
    auto src = adapt(msgs);
    static_assert(BusSource<decltype(src), default_config>,
                  "adapted source must satisfy BusSource");
}

TEST(AdaptTest, RangeSourceDeliversMessages) {
    std::byte data[] = {std::byte{0x42}};
    message_view msgs[] = {
        message_view("test/1", data, 0),
        message_view("test/2", data, 1)
    };
    std::vector<std::string> received;
    auto src = adapt(msgs);
    src.connect();
    src.set_message_callback(message_callback<default_config>([&](const message_view& mv) {
        received.push_back(std::string(mv.topic));
    }));
    src.poll();
    EXPECT_EQ(received.size(), 2u);
    EXPECT_EQ(received[0], "test/1");
    EXPECT_EQ(received[1], "test/2");
}

TEST(AdaptTest, RangeSourceWithPipeline) {
    std::byte data[] = {std::byte{0x01}};
    message_view msgs[] = {
        message_view("input/a", data, 0)
    };
    auto src = adapt(msgs);
    int count = 0;
    auto pipe = basic_pipeline<decltype(src)>(std::move(src))
        | filter([](basic_message<default_config>&) { return true; })
        | sink([&](const message_view&) { ++count; });
    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_GT(count, 0);
}

TEST(AdaptTest, RangeSourceCompletesAfterExhaustion) {
    std::byte data[] = {std::byte{0x01}};
    message_view msgs[] = {
        message_view("complete/a", data, 0),
        message_view("complete/b", data, 1)
    };
    auto src = adapt(msgs);
    int count = 0;
    auto pipe = basic_pipeline<decltype(src)>(std::move(src))
        | sink([&](const message_view&) { ++count; });
    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_FALSE(pipe.is_running());
    EXPECT_EQ(count, 2);
}

TEST(AsRangeTest, SinkBufferPushAndAccess) {
    sink_buffer<default_config> buf;
    std::byte data[] = {std::byte{0x01}};
    message_view mv1("topic/a", data, 0);
    message_view mv2("topic/b", data, 1);
    buf.push(mv1);
    buf.push(mv2);
    EXPECT_EQ(buf.size(), 2u);
    EXPECT_FALSE(buf.empty());
    EXPECT_STREQ(buf[0].topic.data(), "topic/a");
    EXPECT_STREQ(buf[1].topic.data(), "topic/b");
}

TEST(AsRangeTest, SinkBufferClear) {
    sink_buffer<default_config> buf;
    std::byte data[] = {std::byte{0x01}};
    buf.push(message_view("x", data, 0));
    EXPECT_EQ(buf.size(), 1u);
    buf.clear();
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_TRUE(buf.empty());
}

TEST(AsRangeTest, SinkBufferOverflow) {
    sink_buffer<default_config> buf;
    std::byte data[] = {std::byte{0x01}};
    constexpr std::size_t max = default_config::max_stages * 2;
    for (std::size_t i = 0; i < max + 5; ++i) {
        buf.push(message_view("x", data, 0));
    }
    EXPECT_EQ(buf.size(), max);
}

TEST(ConnectToTest, ConnectTwoPipelines) {
    auto pipe1 = basic_pipeline<MockSource>(MockSource{})
        | transform([](basic_message<default_config>&) { return true; });
    auto pipe2 = basic_pipeline<MockSource>(MockSource{});

    auto connected = std::move(pipe1) | connect_to(pipe2);
}

TEST(ConnectToTest, ConnectToWithPipeSyntax) {
    auto pipe1 = basic_pipeline<MockSource>(MockSource{})
        | transform([](basic_message<default_config>&) { return true; });
    auto pipe2 = basic_pipeline<MockSource>(MockSource{});
    auto connected = std::move(pipe1) | connect_to(pipe2);
}

TEST(ConnectToTest, ConnectToEmitsToTarget) {
    std::byte data[] = {std::byte{0x01}};
    message_view mv("source/topic", data, 0);

    std::string received_topic;
    auto pipe2 = basic_pipeline<MockSource>(MockSource{})
        | sink([&](const message_view& m) {
            received_topic = std::string(m.topic);
        });

    auto pipe1 = basic_pipeline<MockSource>(MockSource{})
        | sink([&pipe2](const message_view& m) {
            pipe2.emit(m);
        });
    pipe1.emit(mv);
    EXPECT_EQ(received_topic, "source/topic");
}

TEST(ConnectToTest, WithRemapChangesTopic) {
    std::byte data[] = {std::byte{0x01}};
    std::string received_topic;

    auto pipe2 = basic_pipeline<MockSource>(MockSource{})
        | sink([&](const message_view& m) {
            received_topic = std::string(m.topic);
        });

    auto pipe1 = basic_pipeline<MockSource>(MockSource{});
    auto remapped = with_remap(std::move(pipe1), &pipe2,
        [](std::string_view topic) -> std::string_view {
            return "remapped/topic";
        });

    message_view mv("original/topic", data, 0);
    remapped.emit(mv);
    EXPECT_EQ(received_topic, "remapped/topic");
}

TEST(ConnectToTest, WithRemapEmptyFallsBackToOriginal) {
    std::byte data[] = {std::byte{0x01}};
    std::string received_topic;

    auto pipe2 = basic_pipeline<MockSource>(MockSource{})
        | sink([&](const message_view& m) {
            received_topic = std::string(m.topic);
        });

    auto pipe1 = basic_pipeline<MockSource>(MockSource{});
    auto remapped = with_remap(std::move(pipe1), &pipe2,
        [](std::string_view) -> std::string_view {
            return {};
        });

    message_view mv("original/topic", data, 0);
    remapped.emit(mv);
    EXPECT_EQ(received_topic, "original/topic");
}

TEST(ConnectToTest, WithRemapPreservesPayload) {
    std::byte data[] = {std::byte{0xAB}, std::byte{0xCD}};
    std::string received_topic;
    std::size_t received_size = 0;

    auto pipe2 = basic_pipeline<MockSource>(MockSource{})
        | sink([&](const message_view& m) {
            received_topic = std::string(m.topic);
            received_size = m.payload.size();
        });

    auto pipe1 = basic_pipeline<MockSource>(MockSource{});
    auto remapped = with_remap(std::move(pipe1), &pipe2,
        [](std::string_view) -> std::string_view {
            return "new/topic";
        });

    message_view mv("old/topic", data, 1);
    remapped.emit(mv);
    EXPECT_EQ(received_topic, "new/topic");
    EXPECT_EQ(received_size, 2u);
}