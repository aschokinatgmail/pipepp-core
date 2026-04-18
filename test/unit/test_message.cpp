#include <gtest/gtest.h>
#include <pipepp/core/message.hpp>

using namespace pipepp::core;

TEST(MessageViewTest, DefaultConstruction) {
    message_view mv;
    EXPECT_TRUE(mv.topic.empty());
    EXPECT_TRUE(mv.payload.empty());
    EXPECT_TRUE(mv.empty());
    EXPECT_EQ(mv.qos, 0);
}

TEST(MessageViewTest, ConstructionWithQos) {
    std::byte data[] = {std::byte{0x01}, std::byte{0x02}};
    message_view mv("test/topic", data, 1);
    EXPECT_EQ(mv.topic, "test/topic");
    EXPECT_EQ(mv.payload.size(), 2u);
    EXPECT_EQ(mv.qos, 1);
}

TEST(BasicMessageTest, DefaultConstruction) {
    basic_message<default_config> msg;
    EXPECT_TRUE(msg.topic().empty());
    EXPECT_TRUE(msg.payload().empty());
    EXPECT_EQ(msg.qos(), 0);
}

TEST(BasicMessageTest, FromViewSuccess) {
    std::byte data[] = {std::byte{0xAA}, std::byte{0xBB}};
    message_view mv("sensors/temp", data, 1);
    basic_message<default_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(msg.topic(), "sensors/temp");
    EXPECT_EQ(msg.payload().size(), 2u);
    EXPECT_EQ(msg.qos(), 1);
}

TEST(BasicMessageTest, FromViewOverflow) {
    std::vector<std::byte> big_data(default_config::max_payload_len + 1, std::byte{0xFF});
    message_view mv("topic", big_data, 0);
    basic_message<default_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_FALSE(r.has_value());
}

TEST(BasicMessageTest, FromViewTopicOverflow) {
    std::string long_topic(default_config::max_topic_len + 1, 'x');
    std::byte data[] = {std::byte{0x01}};
    message_view mv(long_topic, data, 0);
    basic_message<default_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_FALSE(r.has_value());
}

TEST(BasicMessageTest, AsViewRoundTrip) {
    std::byte data[] = {std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    message_view original("test", data, 2);
    basic_message<default_config> msg;
    msg.from_view(original);
    message_view view = msg.as_view();
    EXPECT_EQ(view.topic, "test");
    EXPECT_EQ(view.payload.size(), 3u);
    EXPECT_EQ(view.qos, 2);
}

TEST(BasicMessageTest, EmbeddedConfigSmallerBuffers) {
    std::byte data[] = {std::byte{0x01}};
    message_view mv("abc", data, 0);
    basic_message<embedded_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
}