#include <gtest/gtest.h>
#include <pipepp/core/stages.hpp>
#include <pipepp/core/config.hpp>
#include <pipepp/core/error_code.hpp>

using namespace pipepp::core;

TEST(ConfigFnTest, CreateAndApply) {
    int source_val = 10;
    auto fn = config_fn<default_config>::create<int>(
        [](int& src) -> result { return {}; });
    auto r = fn.apply(static_cast<void*>(&source_val));
    EXPECT_TRUE(r.has_value());
}

TEST(ConfigFnTest, CreateWithLambda) {
    int config_val = 0;
    auto fn = config_fn<default_config>::create<int>(
        [&config_val](int& src) -> result { config_val = 42; return {}; });
    int source_val = 10;
    fn.apply(static_cast<void*>(&source_val));
    EXPECT_EQ(config_val, 42);
}

TEST(ConfigFnTest, DefaultIsNull) {
    config_fn<default_config> fn;
    EXPECT_FALSE(static_cast<bool>(fn));
}

TEST(ProcessFnTest, Lambda) {
    process_fn<default_config> pf([](basic_message<default_config>& msg) { return true; });
    EXPECT_TRUE(static_cast<bool>(pf));
    basic_message<default_config> msg;
    EXPECT_TRUE(pf(msg));
}

TEST(ProcessFnTest, FilterDrop) {
    process_fn<default_config> pf([](basic_message<default_config>& msg) { return false; });
    basic_message<default_config> msg;
    EXPECT_FALSE(pf(msg));
}

TEST(ProcessFnTest, ConstInvocation) {
    const process_fn<default_config> pf([](basic_message<default_config>& msg) { return true; });
    basic_message<default_config> msg;
    EXPECT_TRUE(pf(msg));
}

TEST(SinkFnTest, Lambda) {
    bool called = false;
    sink_fn<default_config> sf([&called](const message_view& mv) { called = true; });
    EXPECT_TRUE(static_cast<bool>(sf));
    message_view mv;
    sf(mv);
    EXPECT_TRUE(called);
}

TEST(SinkFnTest, ConstInvocation) {
    bool called = false;
    const sink_fn<default_config> sf([&called](const message_view&) { called = true; });
    message_view mv;
    sf(mv);
    EXPECT_TRUE(called);
}

TEST(SinkFnTest, DefaultIsNull) {
    sink_fn<default_config> sf;
    EXPECT_FALSE(static_cast<bool>(sf));
}

TEST(ProcessFnTest, EmbeddedConfig) {
    process_fn<embedded_config> pf([](basic_message<embedded_config>& msg) { return true; });
    basic_message<embedded_config> msg;
    EXPECT_TRUE(pf(msg));
}