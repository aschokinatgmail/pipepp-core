#include <thread>
#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include <pipepp/core/connect_to.hpp>
#include <pipepp/core/adapt.hpp>
#include <pipepp/core/error_code.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(FaultTest, ConnectFailureReturnsError) {
    FaultSource fs;
    fs.fail_connect = true;
    basic_pipeline<FaultSource> pipe(std::move(fs));
    pipe.add_stage(TrueStage{})
       .set_sink(NoopSink{});
    pipe.subscribe("topic", 1);
    auto r = pipe.run();
    EXPECT_FALSE(r);
    EXPECT_EQ(pipe.source().connect_count, 1);
}

TEST(FaultTest, SubscribeFailureReturnsErrorAndDisconnects) {
    FaultSource fs;
    fs.fail_subscribe = true;
    basic_pipeline<FaultSource> pipe(std::move(fs));
    pipe.add_stage(TrueStage{})
       .set_sink(NoopSink{});
    pipe.subscribe("topic", 0);
    auto r = pipe.run();
    EXPECT_FALSE(r);
    EXPECT_EQ(pipe.source().disconnect_count, 1);
    EXPECT_FALSE(pipe.source().connected);
}

TEST(FaultTest, DisconnectFailureReported) {
    FaultSource fs;
    fs.fail_disconnect = true;
    fs.max_polls = 1;
    basic_pipeline<FaultSource> pipe(std::move(fs));
    pipe.set_sink(NoopSink{});
    pipe.subscribe("t", 0);
    auto r = pipe.run();
    EXPECT_TRUE(r);
}

TEST(FaultTest, RunHappyPathWithSubscriptions) {
    FaultSource fs;
    fs.max_polls = 2;
    std::atomic<int> sink_count{0};
    basic_pipeline<FaultSource> pipe(std::move(fs));
    pipe.subscribe("sensors/#", 1)
       .subscribe("status", 0)
       .add_stage(TrueStage{})
       .set_sink([&](const message_view&) { sink_count.fetch_add(1); });
    auto r = pipe.run();
    EXPECT_TRUE(r);
    EXPECT_EQ(sink_count.load(), 2);
    EXPECT_EQ(pipe.source().subscribe_count, 2);
    EXPECT_EQ(pipe.source().disconnect_count, 1);
}

TEST(FaultTest, ConfigOpAppliedDuringRun) {
    FaultSource fs;
    fs.max_polls = 1;
    int config_applied = 0;
    basic_pipeline<FaultSource> pipe(std::move(fs));
    pipe.configure([&](FaultSource& src) {
        ++config_applied;
        return result{};
    });
    pipe.set_sink(NoopSink{});
    pipe.subscribe("t", 0);
    auto r = pipe.run();
    EXPECT_TRUE(r);
    EXPECT_EQ(config_applied, 1);
}

TEST(FaultTest, ConfigOpFailureCancelsRun) {
    FaultSource fs;
    fs.max_polls = 1;
    basic_pipeline<FaultSource> pipe(std::move(fs));
    pipe.configure([](FaultSource&) {
        return result{unexpect, make_unexpected(error_code::invalid_argument)};
    });
    pipe.set_sink(NoopSink{});
    pipe.subscribe("t", 0);
    auto r = pipe.run();
    EXPECT_FALSE(r);
    EXPECT_EQ(pipe.source().disconnect_count, 1);
}

TEST(FaultTest, PollOnceRunningAndNotRunning) {
    SelfStoppingSource sss;
    sss.max_polls = 50;
    basic_pipeline<SelfStoppingSource> pipe(std::move(sss));
    EXPECT_FALSE(pipe.poll_once());
    pipe.set_sink(NoopSink{});
    pipe.subscribe("t", 0);
    std::thread runner([&]() { pipe.run(); });
    for (int i = 0; i < 200 && !pipe.is_running(); ++i) std::this_thread::yield();
    if (pipe.is_running()) pipe.poll_once();
    pipe.stop();
    runner.join();
    EXPECT_FALSE(pipe.poll_once());
}

TEST(FaultTest, EmitOversizedTopicDrops) {
    basic_pipeline<MockSource> pipe(MockSource{});
    int sink_count = 0;
    pipe.set_sink(CountingSink{&sink_count});
    std::string long_topic(default_config::max_topic_len + 10, 'x');
    std::byte data[] = {std::byte{0x01}};
    message_view mv(long_topic, data, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 0);
}

TEST(FaultTest, EmitOversizedPayloadDrops) {
    basic_pipeline<MockSource> pipe(MockSource{});
    int sink_count = 0;
    pipe.set_sink(CountingSink{&sink_count});
    std::vector<std::byte> big_payload(default_config::max_payload_len + 10, std::byte{0xFF});
    message_view mv("topic", big_payload, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 0);
}

TEST(FaultTest, EmitWithoutSinkNoCrash) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);
    pipe.emit(mv);
}

TEST(FaultTest, StageFilterDropsMessage) {
    basic_pipeline<MockSource> pipe(MockSource{});
    int sink_count = 0;
    pipe.add_stage([](basic_message<default_config>&) { return false; })
       .set_sink(CountingSink{&sink_count});
    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);
    pipe.emit(mv);
    EXPECT_EQ(sink_count, 0);
}

TEST(FaultTest, CheckPendingErrorFromTruncatedUri) {
    std::string long_uri(default_config::max_uri_len + 50, 'a');
    basic_pipeline<MockSource> pipe(MockSource{}, long_uri);
    auto r = pipe.check();
    EXPECT_FALSE(r);
    auto run_r = pipe.run();
    EXPECT_FALSE(run_r);
}

TEST(FaultTest, ConnectUriTruncated) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::string long_uri(default_config::max_uri_len + 50, 'b');
    pipe.connect_uri(long_uri);
    EXPECT_FALSE(pipe.check());
}

TEST(FaultTest, ConnectUriValid) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.connect_uri("mqtt://broker:1883");
    EXPECT_TRUE(pipe.check());
}

TEST(FaultTest, SubscribeTopicTooLong) {
    basic_pipeline<MockSource> pipe(MockSource{});
    std::string long_topic(default_config::max_topic_len + 10, 't');
    pipe.subscribe(long_topic, 0);
    EXPECT_FALSE(pipe.check());
}

TEST(FaultTest, SubscribeOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i <= default_config::max_subscriptions; ++i) {
        pipe.subscribe("t", 0);
    }
    EXPECT_FALSE(pipe.check());
}

TEST(FaultTest, ConfigureOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i <= default_config::max_config_ops; ++i) {
        pipe.configure(NoopConfigure{});
    }
    EXPECT_FALSE(pipe.check());
}

TEST(FaultTest, AddStageOverflow) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i <= default_config::max_stages; ++i) {
        pipe.add_stage(TrueStage{});
    }
    EXPECT_FALSE(pipe.check());
}

TEST(FaultTest, CheckReturnsOkOnCleanPipeline) {
    basic_pipeline<MockSource> pipe(MockSource{});
    auto r = pipe.check();
    EXPECT_TRUE(r);
}

TEST(FaultTest, MoveAssignmentTransfersState) {
    basic_pipeline<MockSource> pipe1(MockSource{});
    pipe1.subscribe("a", 1);
    basic_pipeline<MockSource> pipe2(MockSource{});
    pipe2 = std::move(pipe1);
    EXPECT_TRUE(pipe2.check());
}

TEST(FaultTest, EventLoopCallbackInvokesEmit) {
    CallbackPollSource cps;
    cps.max_polls = 3;
    std::atomic<int> sink_count{0};
    basic_pipeline<CallbackPollSource> pipe(std::move(cps));
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view& mv) {
           EXPECT_EQ(mv.topic, "cb/topic");
           sink_count.fetch_add(1);
       });
    pipe.subscribe("t", 0);
    auto r = pipe.run();
    EXPECT_TRUE(r);
    EXPECT_EQ(sink_count.load(), 3);
}

TEST(FaultTest, ConnectToForwardingWithFaultSource) {
    FaultSource fs1;
    fs1.max_polls = 2;
    FaultSource fs2;
    fs2.max_polls = 0;

    int pipe2_sink_count = 0;
    basic_pipeline<FaultSource> pipe2(std::move(fs2));
    pipe2.set_sink(CountingSink{&pipe2_sink_count});

    basic_pipeline<FaultSource> pipe1(std::move(fs1));
    pipe1.set_sink([&](const message_view& mv) { pipe2.emit(mv); });
    pipe1.subscribe("t", 0);
    pipe1.run();
    EXPECT_EQ(pipe2_sink_count, 2);
}

TEST(FaultTest, AdaptRangeSourcePoll) {
    std::byte d1 = std::byte{0x01};
    std::byte d2 = std::byte{0x02};
    std::vector<message_view> msgs = {
        message_view("a", std::span<const std::byte>(&d1, 1), 0),
        message_view("b", std::span<const std::byte>(&d2, 1), 1),
    };
    auto src = adapt(msgs);
    EXPECT_TRUE(src.connect());
    EXPECT_TRUE(src.is_connected());
    EXPECT_TRUE(src.subscribe("t", 0));
    EXPECT_TRUE(src.publish("t", {}, 0));
    src.disconnect();
    EXPECT_FALSE(src.is_connected());
}

TEST(FaultTest, ErrorCodeDefaultCase) {
    auto msg = error_code_message(static_cast<error_code>(255));
    EXPECT_STREQ(msg, "unknown error");
}

TEST(FaultTest, ErrorCodeNoneCase) {
    auto msg = error_code_message(error_code::none);
    EXPECT_STREQ(msg, "no error");
}

TEST(FaultTest, RunWithNoSubscriptions) {
    CallbackPollSource cps;
    cps.max_polls = 1;
    int sink_count = 0;
    basic_pipeline<CallbackPollSource> pipe(std::move(cps));
    pipe.add_stage(TrueStage{})
       .set_sink(CountingSink{&sink_count});
    auto r = pipe.run();
    EXPECT_TRUE(r);
    EXPECT_EQ(sink_count, 1);
}

TEST(FaultTest, PipelineStopBeforeRun) {
    basic_pipeline<MockSource> pipe(MockSource{});
    pipe.stop();
    EXPECT_FALSE(pipe.is_running());
}

TEST(FaultTest, PipelineSourceAccess) {
    MockSource ms;
    ms.connected = true;
    basic_pipeline<MockSource> pipe(std::move(ms));
    EXPECT_TRUE(pipe.source().is_connected());
    const auto& cref = pipe.source();
    EXPECT_TRUE(cref.is_connected());
}

TEST(FaultTest, EmitFromMultipleThreadsWithFault) {
    FaultSource fs;
    fs.max_polls = 0;
    std::atomic<int> sink_count{0};
    basic_pipeline<FaultSource> pipe(std::move(fs));
    pipe.add_stage(TrueStage{})
       .set_sink([&](const message_view&) { sink_count.fetch_add(1, std::memory_order_relaxed); });

    std::byte data[] = {std::byte{0x01}};
    message_view mv("topic", data, 0);

    std::thread t1([&]() { for (int i = 0; i < 100; ++i) pipe.emit(mv); });
    std::thread t2([&]() { for (int i = 0; i < 100; ++i) pipe.emit(mv); });

    t1.join();
    t2.join();

    EXPECT_EQ(sink_count.load(std::memory_order_acquire), 200);
}
