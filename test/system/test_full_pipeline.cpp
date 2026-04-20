#include <gtest/gtest.h>
#include <pipepp/pipepp.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

static_assert(BusSource<FullSource, default_config>);

TEST(SystemTest, FullPipelineEmitAndSink) {
    std::string sinked_topic;
    bool sink_called = false;

    basic_pipeline<FullSource> pipe(FullSource{});
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view& mv) {
            sink_called = true;
            sinked_topic = std::string(mv.topic);
        });

    std::byte data[] = {std::byte{0x42}};
    message_view mv("system/test", data, 1);
    pipe.emit(mv);

    EXPECT_TRUE(sink_called);
    EXPECT_EQ(sinked_topic, "system/test");
}

TEST(SystemTest, PipelineDropsFilteredMessages) {
    int pass_count = 0;
    int total_count = 0;

    basic_pipeline<FullSource> pipe(FullSource{});
    pipe.add_stage([&](basic_message<default_config>& m) {
            ++total_count;
            if (m.qos() > 0) { ++pass_count; return true; }
            return false;
        })
        .set_sink(NoopSink{});

    std::byte data[] = {std::byte{0x01}};
    pipe.emit(message_view("topic1", data, 1));
    pipe.emit(message_view("topic2", data, 0));
    pipe.emit(message_view("topic3", data, 2));

    EXPECT_EQ(total_count, 3);
    EXPECT_EQ(pass_count, 2);
}

TEST(SystemTest, EmbeddedConfigPipeline) {
    auto pipe = basic_pipeline<FullSource, embedded_config>(FullSource{})
        | subscribe("emb/#", 0)
        | filter(TrueFilter{})
        | sink(NoopSink{});
}

TEST(SystemTest, EmbeddedConfigEmitWithSpinlock) {
    int sink_count = 0;
    basic_pipeline<FullSource, embedded_config> pipe(FullSource{});
    pipe.add_stage(TrueStage{})
       .set_sink(CountingSink{&sink_count});

    std::byte data[] = {std::byte{0x01}};
    pipe.emit(message_view("emb/test", data, 1));

    EXPECT_GT(sink_count, 0);
}
