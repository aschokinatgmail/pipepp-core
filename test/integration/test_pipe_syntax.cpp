#include <gtest/gtest.h>
#include <pipepp/pipepp.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(PipeSyntaxTest, SubscribeAndFilter) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | subscribe("sensors/#", 0)
        | filter(TrueFilter{})
        | sink(NoopSink{});
}

TEST(PipeSyntaxTest, TransformAndSink) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | transform(TrueStage{})
        | sink(NoopSink{});
}

TEST(PipeSyntaxTest, MultipleStages) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | subscribe("a/#", 0)
        | transform(TrueStage{})
        | filter(TrueFilter{})
        | transform(TrueStage{})
        | sink(NoopSink{});
}

TEST(PipeSyntaxTest, ConfigureOnly) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | configure(NoopConfigure{})
        | sink(NoopSink{});
}

TEST(PipeSyntaxTest, PipeChainEmitAndReceive) {
    std::string received;
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | subscribe("pipe/syntax", 0)
        | transform(TrueStage{})
        | sink([&](const message_view& mv) { received = std::string(mv.topic); });
    
    std::byte data[] = {std::byte{0x01}};
    message_view mv("pipe/syntax", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(received, "pipe/syntax");
}

TEST(PipeSyntaxTest, PipeChainFilterDrops) {
    int sink_count = 0;
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | subscribe("drop/#", 0)
        | filter([](basic_message<default_config>&) { return false; })
        | sink(CountingSink{&sink_count});
    
    std::byte data[] = {std::byte{0x01}};
    message_view mv("drop/test", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 0);
}
