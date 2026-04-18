#include <gtest/gtest.h>
#include <pipepp/core/config.hpp>
#include <pipepp/core/message.hpp>
#include <pipepp/core/expected.hpp>
#include <pipepp/core/small_function.hpp>

using namespace pipepp::core;

TEST(EmbeddedConfigTest, SizeConstraints) {
    using C = embedded_config;
    static_assert(C::max_topic_len <= default_config::max_topic_len);
    static_assert(C::max_payload_len <= default_config::max_payload_len);
    static_assert(C::max_stages <= default_config::max_stages);
    static_assert(C::small_fn_size <= default_config::small_fn_size);
    static_assert(C::source_size <= default_config::source_size);
}

TEST(EmbeddedConfigTest, MessageFitsInEmbeddedBuffers) {
    basic_message<embedded_config> msg;
    std::byte data[] = {std::byte{0x01}};
    message_view mv("small", data, 0);
    auto r = msg.from_view(mv);
    EXPECT_TRUE(r.has_value());
}

TEST(EmbeddedConfigTest, MessageOverflowInEmbeddedBuffers) {
    basic_message<embedded_config> msg;
    std::vector<std::byte> big_data(embedded_config::max_payload_len + 1, std::byte{0xFF});
    message_view mv("x", big_data, 0);
    auto r = msg.from_view(mv);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::buffer_overflow);
}

TEST(EmbeddedConfigTest, SmallFunctionInEmbeddedSize) {
    small_function<int(int), embedded_config::small_fn_size> fn([](int x) { return x + 1; });
    EXPECT_TRUE(static_cast<bool>(fn));
    EXPECT_EQ(fn(41), 42);
}