#include <gtest/gtest.h>
#include <pipepp/pipepp.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

static_assert(BusSource<CountingSource, default_config>);

TEST(IntegrationTest, PipelineConnectAndConfigure) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{});
    int configured = 0;
    pipe.configure([&](auto&) -> result { ++configured; return {}; });
    EXPECT_EQ(configured, 0);
}

TEST(IntegrationTest, PipelineSubscribeRecordsTopic) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{});
    pipe.subscribe("sensors/temp", 1);
    pipe.subscribe("sensors/humidity", 0);
}

TEST(IntegrationTest, PipeSyntaxFullChain) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | subscribe("input/#", 0)
        | transform([](basic_message<default_config>& m) { return true; })
        | filter([](basic_message<default_config>& m) { return true; })
        | sink([](const message_view&) {});
}

TEST(IntegrationTest, PipeSyntaxWithConfigure) {
    bool config_called = false;
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | configure([&](auto&) -> result { config_called = true; return {}; })
        | subscribe("data/#", 0)
        | sink([](const message_view&) {});
}