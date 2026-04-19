#include <gtest/gtest.h>
#include <pipepp/core/pipeline.hpp>
#include "../helpers/test_sources.hpp"

namespace pipepp::core {

TEST(CapacityTest, SubscribeOverflowDetectedByCheck) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_subscriptions + 1; ++i) {
        char topic[] = "topic/x";
        topic[6] = static_cast<char>('0' + static_cast<int>(i % 10));
        pipe.subscribe(topic, 0);
    }
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(CapacityTest, SubscribeWithinCapacityNoError) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_subscriptions; ++i) {
        char topic[] = "topic/x";
        topic[6] = static_cast<char>('0' + static_cast<int>(i % 10));
        pipe.subscribe(topic, 0);
    }
    auto r = pipe.check();
    EXPECT_TRUE(r.has_value());
}

TEST(CapacityTest, ConfigureOverflowDetectedByCheck) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_config_ops + 1; ++i) {
        pipe.configure([](auto&) -> result { return {}; });
    }
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(CapacityTest, ConfigureWithinCapacityNoError) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_config_ops; ++i) {
        pipe.configure([](auto&) -> result { return {}; });
    }
    auto r = pipe.check();
    EXPECT_TRUE(r.has_value());
}

TEST(CapacityTest, SubscribeOverflowBlocksRun) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_subscriptions + 1; ++i) {
        char topic[] = "topic/x";
        topic[6] = static_cast<char>('0' + static_cast<int>(i % 10));
        pipe.subscribe(topic, 0);
    }
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(CapacityTest, ConfigureOverflowBlocksRun) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_config_ops + 1; ++i) {
        pipe.configure([](auto&) -> result { return {}; });
    }
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(CapacityTest, AddStageOverflowDetectedByCheck) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_stages + 1; ++i) {
        pipe.add_stage([](basic_message<default_config>&) { return true; });
    }
    auto r = pipe.check();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(CapacityTest, AddStageOverflowBlocksRun) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_stages + 1; ++i) {
        pipe.add_stage([](basic_message<default_config>&) { return true; });
    }
    auto r = pipe.run();
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), error_code::capacity_exceeded);
}

TEST(CapacityTest, AddStageWithinCapacityNoError) {
    basic_pipeline<MockSource> pipe(MockSource{});
    for (std::size_t i = 0; i < default_config::max_stages; ++i) {
        pipe.add_stage([](basic_message<default_config>&) { return true; });
    }
    auto r = pipe.check();
    EXPECT_TRUE(r.has_value());
}

}
