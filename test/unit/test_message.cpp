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

TEST(MessageViewTest, EmptyFalseWhenTopicNonEmpty) {
    message_view mv("a", {}, 0);
    EXPECT_FALSE(mv.empty());
}

TEST(MessageViewTest, EmptyFalseWhenPayloadNonEmpty) {
    std::byte b = std::byte{0x01};
    message_view mv("", {&b, 1}, 0);
    EXPECT_FALSE(mv.empty());
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

TEST(BasicMessageTest, FromViewZeroLength) {
    message_view mv("", {}, 0);
    basic_message<default_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(msg.topic().empty());
    EXPECT_TRUE(msg.payload().empty());
    EXPECT_EQ(msg.qos(), 0);
}

TEST(BasicMessageTest, FromViewExactCapacityTopic) {
    std::string exact_topic(default_config::max_topic_len, 'a');
    std::byte data[] = {std::byte{0x01}};
    message_view mv(exact_topic, data, 0);
    basic_message<default_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(msg.topic().size(), default_config::max_topic_len);
}

TEST(BasicMessageTest, FromViewExactCapacityPayload) {
    std::vector<std::byte> exact_payload(default_config::max_payload_len, std::byte{0xAB});
    message_view mv("t", exact_payload, 0);
    basic_message<default_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(msg.payload().size(), default_config::max_payload_len);
}

TEST(BasicMessageTest, AsViewPayloadByteContent) {
    std::byte data[] = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
    message_view original("hex", data, 3);
    basic_message<default_config> msg;
    msg.from_view(original);
    auto view = msg.as_view();
    ASSERT_EQ(view.payload.size(), 4u);
    EXPECT_EQ(view.payload[0], std::byte{0xDE});
    EXPECT_EQ(view.payload[1], std::byte{0xAD});
    EXPECT_EQ(view.payload[2], std::byte{0xBE});
    EXPECT_EQ(view.payload[3], std::byte{0xEF});
    EXPECT_EQ(view.qos, 3);
}

TEST(BasicMessageTest, AsViewEmpty) {
    basic_message<default_config> msg;
    message_view view = msg.as_view();
    EXPECT_TRUE(view.topic.empty());
    EXPECT_TRUE(view.payload.empty());
    EXPECT_EQ(view.qos, 0);
    EXPECT_TRUE(view.empty());
}

TEST(EmbeddedMessageTest, DefaultConstruction) {
    basic_message<embedded_config> msg;
    EXPECT_TRUE(msg.topic().empty());
    EXPECT_TRUE(msg.payload().empty());
    EXPECT_EQ(msg.qos(), 0);
}

TEST(EmbeddedMessageTest, FromViewSuccess) {
    std::byte data[] = {std::byte{0x01}};
    message_view mv("abc", data, 0);
    basic_message<embedded_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(msg.topic(), "abc");
    EXPECT_EQ(msg.payload().size(), 1u);
    EXPECT_EQ(msg.payload()[0], std::byte{0x01});
    EXPECT_EQ(msg.qos(), 0);
}

TEST(EmbeddedMessageTest, FromViewTopicOverflow) {
    std::string long_topic(embedded_config::max_topic_len + 1, 'z');
    message_view mv(long_topic, {}, 0);
    basic_message<embedded_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_FALSE(r.has_value());
}

TEST(EmbeddedMessageTest, FromViewPayloadOverflow) {
    std::vector<std::byte> big_payload(embedded_config::max_payload_len + 1, std::byte{0xFF});
    message_view mv("t", big_payload, 0);
    basic_message<embedded_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_FALSE(r.has_value());
}

TEST(EmbeddedMessageTest, AsViewRoundTrip) {
    std::byte data[] = {std::byte{0x10}, std::byte{0x20}, std::byte{0x30}};
    message_view original("emb/test", data, 2);
    basic_message<embedded_config> msg;
    msg.from_view(original);
    auto view = msg.as_view();
    EXPECT_EQ(view.topic, "emb/test");
    ASSERT_EQ(view.payload.size(), 3u);
    EXPECT_EQ(view.payload[0], std::byte{0x10});
    EXPECT_EQ(view.payload[1], std::byte{0x20});
    EXPECT_EQ(view.payload[2], std::byte{0x30});
    EXPECT_EQ(view.qos, 2);
}

TEST(EmbeddedMessageTest, FromViewExactCapacityTopic) {
    std::string exact_topic(embedded_config::max_topic_len, 'q');
    message_view mv(exact_topic, {}, 1);
    basic_message<embedded_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(msg.topic().size(), embedded_config::max_topic_len);
    EXPECT_EQ(msg.qos(), 1);
}

TEST(EmbeddedMessageTest, FromViewExactCapacityPayload) {
    std::vector<std::byte> exact_payload(embedded_config::max_payload_len, std::byte{0xCC});
    message_view mv("x", exact_payload, 0);
    basic_message<embedded_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(msg.payload().size(), embedded_config::max_payload_len);
}

TEST(EmbeddedMessageTest, FromViewZeroLength) {
    message_view mv("", {}, 0);
    basic_message<embedded_config> msg;
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(msg.topic().empty());
    EXPECT_TRUE(msg.payload().empty());
}

TEST(EmbeddedMessageTest, AsViewEmpty) {
    basic_message<embedded_config> msg;
    message_view view = msg.as_view();
    EXPECT_TRUE(view.empty());
    EXPECT_TRUE(view.topic.empty());
    EXPECT_TRUE(view.payload.empty());
    EXPECT_EQ(view.qos, 0);
}
