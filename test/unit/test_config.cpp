#include <gtest/gtest.h>
#include <pipepp/core/config.hpp>

TEST(ConfigTest, DefaultConfigValues) {
    using C = pipepp::core::default_config;
    static_assert(C::max_topic_len == 256);
    static_assert(C::max_payload_len == 4096);
    static_assert(C::max_stages == 8);
    static_assert(C::max_config_ops == 4);
    static_assert(C::max_subscriptions == 8);
    static_assert(C::small_fn_size == 128);
    static_assert(C::source_size == 2048);
}

TEST(ConfigTest, EmbeddedConfigValues) {
    using C = pipepp::core::embedded_config;
    static_assert(C::max_topic_len == 64);
    static_assert(C::max_payload_len == 512);
    static_assert(C::max_stages == 4);
    static_assert(C::max_config_ops == 2);
    static_assert(C::max_subscriptions == 4);
    static_assert(C::small_fn_size == 64);
    static_assert(C::source_size == 512);
}