#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/pipeline_ops.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(PipelineOpsTest, TransformReturningInt) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage([](basic_message<default_config>&) { return 42; })
        .set_sink([&](const message_view&) { });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);
    pipe.emit(mv);
}

TEST(PipelineOpsTest, TransformReturningZeroDropsMessage) {
    bool sink_called = false;
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.add_stage([](basic_message<default_config>&) { return 0; })
        .set_sink([&](const message_view&) { sink_called = true; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);
    pipe.emit(mv);
    EXPECT_FALSE(sink_called);
}

TEST(PipelineOpsTest, ExecuteFreeFunction) {
    auto pipe = basic_pipeline<SelfStoppingSource>(SelfStoppingSource{});
    int sink_count = 0;
    pipe.set_sink([&](const message_view&) { ++sink_count; });
    auto r = execute(pipe);
    EXPECT_TRUE(r.has_value());
    EXPECT_GE(sink_count, 1);
}

TEST(PipelineOpsTest, PipeSyntaxSinkReceivesMessage) {
    std::string received;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | transform([](basic_message<default_config>& m) { return true; })
        | sink([&](const message_view& mv) { received = std::string(mv.topic); });
    std::byte data[] = {std::byte{0xAB}};
    message_view mv("pipe/sink", data, 1);
    pipe.emit(mv);
    EXPECT_EQ(received, "pipe/sink");
}

TEST(PipelineOpsTest, PipeSyntaxTransformNonBoolToInt) {
    bool sink_called = false;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | transform([](basic_message<default_config>&) { return 1; })
        | sink([&](const message_view&) { sink_called = true; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("int/transform", data, 0);
    pipe.emit(mv);
    EXPECT_TRUE(sink_called);
}

TEST(PipelineOpsTest, PipeSyntaxFilterDropsMessage) {
    bool sink_called = false;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | filter([](basic_message<default_config>&) { return false; })
        | sink([&](const message_view&) { sink_called = true; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("filtered", data, 0);
    pipe.emit(mv);
    EXPECT_FALSE(sink_called);
}

TEST(PipelineOpsTest, PipeSyntaxFilterPassesMessage) {
    int count = 0;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | filter([](basic_message<default_config>&) { return true; })
        | sink([&](const message_view&) { ++count; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("pass", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(count, 1);
}

TEST(PipelineOpsTest, SubscribeWithQos) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | subscribe("sensors/temp", 2)
        | sink([](const message_view&) {});
}

TEST(PipelineOpsTest, ConfigureAppliedViaPipe) {
    int config_count = 0;
    auto pipe = basic_pipeline<SelfStoppingSource>(SelfStoppingSource{})
        | configure([&](auto&) -> result { ++config_count; return {}; })
        | sink([](const message_view&) {});
    auto r = pipe.run();
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(config_count, 1);
}

TEST(PipelineOpsTest, EmitWithoutSinkDoesNotCrash) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | transform([](basic_message<default_config>&) { return true; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("no/sink", data, 0);
    pipe.emit(mv);
}

TEST(PipelineOpsTest, MultipleFiltersAllPass) {
    int count = 0;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | filter([](basic_message<default_config>&) { return true; })
        | filter([](basic_message<default_config>&) { return true; })
        | sink([&](const message_view&) { ++count; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("multi/filter", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(count, 1);
}

TEST(PipelineOpsTest, MultipleFiltersOneDrops) {
    int count = 0;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | filter([](basic_message<default_config>&) { return true; })
        | filter([](basic_message<default_config>&) { return false; })
        | sink([&](const message_view&) { ++count; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("drop/mid", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(count, 0);
}

TEST(PipelineOpsTest, TransformChainedWithFilter) {
    int count = 0;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | transform([](basic_message<default_config>&) { return true; })
        | filter([](basic_message<default_config>&) { return true; })
        | sink([&](const message_view&) { ++count; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("xform/filter", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(count, 1);
}

TEST(PipelineOpsTest, PipelineMovedViaPipeSyntax) {
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | subscribe("a/b", 1)
        | transform([](basic_message<default_config>&) { return true; })
        | filter([](basic_message<default_config>&) { return true; })
        | configure([](auto&) -> result { return {}; })
        | sink([](const message_view&) {});
    EXPECT_FALSE(pipe.is_running());
}

TEST(PipelineOpsTest, ProcessAliasWorksLikeTransform) {
    bool sink_called = false;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | process([](basic_message<default_config>&) { return true; })
        | sink([&](const message_view&) { sink_called = true; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("process/test", data, 0);
    pipe.emit(mv);
    EXPECT_TRUE(sink_called);
}

TEST(PipelineOpsTest, ProcessDropsOnFalse) {
    bool sink_called = false;
    auto pipe = basic_pipeline<MockSource>(MockSource{})
        | process([](basic_message<default_config>&) { return false; })
        | sink([&](const message_view&) { sink_called = true; });
    std::byte data[] = {std::byte{0x01}};
    message_view mv("process/drop", data, 0);
    pipe.emit(mv);
    EXPECT_FALSE(sink_called);
}