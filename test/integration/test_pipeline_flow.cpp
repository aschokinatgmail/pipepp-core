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

TEST(IntegrationTest, PipelineEmitAndSinkReceives) {
    std::string received;
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | transform([](basic_message<default_config>& m) { return true; })
        | filter([](basic_message<default_config>& m) { return true; })
        | sink([&](const message_view& mv) { received = std::string(mv.topic); });
    
    std::byte data[] = {std::byte{0x01}};
    message_view mv("integration/test", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(received, "integration/test");
}

TEST(IntegrationTest, PipelineFilterBlocksMessage) {
    bool sink_called = false;
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | filter([](basic_message<default_config>&) { return false; })
        | sink([&](const message_view&) { sink_called = true; });
    
    std::byte data[] = {std::byte{0x01}};
    message_view mv("blocked/topic", data, 0);
    pipe.emit(mv);
    EXPECT_FALSE(sink_called);
}

TEST(IntegrationTest, PipelineMultiStageTransform) {
    int transform_count = 0;
    std::string final_topic;
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | transform([&](basic_message<default_config>&) { ++transform_count; return true; })
        | transform([&](basic_message<default_config>&) { ++transform_count; return true; })
        | sink([&](const message_view& mv) { final_topic = std::string(mv.topic); });
    
    std::byte data[] = {std::byte{0x01}};
    message_view mv("multi/stage", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(transform_count, 2);
    EXPECT_EQ(final_topic, "multi/stage");
}