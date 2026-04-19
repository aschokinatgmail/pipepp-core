#include <gtest/gtest.h>
#include <pipepp/pipepp.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(ConnectToTest, PipeSyntaxForwardMessage) {
    std::string received_topic;
    int received_qos = -1;

    auto pipe2 = basic_pipeline<FullSource>(FullSource{})
        | sink([&](const message_view& mv) {
              received_topic = std::string(mv.topic);
              received_qos = mv.qos;
          });

    auto pipe1 = basic_pipeline<FullSource>(FullSource{})
        | connect_to(pipe2);

    std::byte data[] = {std::byte{0x42}};
    message_view mv(" fwd/topic ", data, 1);
    pipe1.emit(mv);

    EXPECT_EQ(received_topic, " fwd/topic ");
    EXPECT_EQ(received_qos, 1);
}

TEST(ConnectToTest, PipeSyntaxPrefixRemap) {
    std::string received_topic;

    auto pipe2 = basic_pipeline<FullSource>(FullSource{})
        | sink([&](const message_view& mv) {
              received_topic = std::string(mv.topic);
          });

    auto pipe1 = basic_pipeline<FullSource>(FullSource{})
        | connect_to(pipe2, "output/");

    std::byte data[] = {std::byte{0x01}};
    message_view mv("input/data", data, 0);
    pipe1.emit(mv);

    EXPECT_EQ(received_topic, "output/input/data");
}

TEST(ConnectToTest, WithRemapCustomFunction) {
    std::string received_topic;

    auto pipe2 = basic_pipeline<FullSource>(FullSource{})
        | sink([&](const message_view& mv) {
              received_topic = std::string(mv.topic);
          });

    auto pipe1 = with_remap(
        basic_pipeline<FullSource>(FullSource{}),
        &pipe2,
        [](std::string_view topic) -> std::string_view {
            if (topic.starts_with("old/")) {
                return topic.substr(4);
            }
            return topic;
        });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("old/sensor/temp", data, 0);
    pipe1.emit(mv);

    EXPECT_EQ(received_topic, "sensor/temp");
}

TEST(ConnectToTest, WithRemapEmptyPrefixFallsBack) {
    std::string received_topic;

    auto pipe2 = basic_pipeline<FullSource>(FullSource{})
        | sink([&](const message_view& mv) {
              received_topic = std::string(mv.topic);
          });

    auto pipe1 = with_remap(
        basic_pipeline<FullSource>(FullSource{}),
        &pipe2,
        [](std::string_view) -> std::string_view { return {}; });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("original/topic", data, 0);
    pipe1.emit(mv);

    EXPECT_EQ(received_topic, "original/topic");
}

TEST(ConnectToTest, PipeSyntaxPreservesPayload) {
    std::vector<std::byte> received_payload;

    auto pipe2 = basic_pipeline<FullSource>(FullSource{})
        | sink([&](const message_view& mv) {
              received_payload.assign(mv.payload.begin(), mv.payload.end());
          });

    auto pipe1 = basic_pipeline<FullSource>(FullSource{})
        | connect_to(pipe2);

    std::byte data[] = {std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
    message_view mv("payload/test", data, 0);
    pipe1.emit(mv);

    ASSERT_EQ(received_payload.size(), 3u);
    EXPECT_EQ(received_payload[0], std::byte{0xAA});
    EXPECT_EQ(received_payload[1], std::byte{0xBB});
    EXPECT_EQ(received_payload[2], std::byte{0xCC});
}

TEST(ConnectToTest, PrefixRemapPreservesPayloadAndQos) {
    std::vector<std::byte> received_payload;
    int received_qos = -1;
    std::string received_topic;

    auto pipe2 = basic_pipeline<FullSource>(FullSource{})
        | sink([&](const message_view& mv) {
              received_topic = std::string(mv.topic);
              received_payload.assign(mv.payload.begin(), mv.payload.end());
              received_qos = mv.qos;
          });

    auto pipe1 = basic_pipeline<FullSource>(FullSource{})
        | connect_to(pipe2, "prefix/");

    std::byte data[] = {std::byte{0xFF}};
    message_view mv("data", data, 2);
    pipe1.emit(mv);

    EXPECT_EQ(received_topic, "prefix/data");
    ASSERT_EQ(received_payload.size(), 1u);
    EXPECT_EQ(received_payload[0], std::byte{0xFF});
    EXPECT_EQ(received_qos, 2);
}

TEST(ConnectToTest, ThreeStageChaining) {
    std::string final_topic;

    auto pipe3 = basic_pipeline<FullSource>(FullSource{})
        | sink([&](const message_view& mv) {
              final_topic = std::string(mv.topic);
          });

    auto pipe2 = basic_pipeline<FullSource>(FullSource{})
        | connect_to(pipe3, "stage2/");

    auto pipe1 = basic_pipeline<FullSource>(FullSource{})
        | connect_to(pipe2, "stage1/");

    std::byte data[] = {std::byte{0x01}};
    message_view mv("start", data, 0);
    pipe1.emit(mv);

    EXPECT_EQ(final_topic, "stage2/stage1/start");
}
