#include <gtest/gtest.h>
#include <pipepp/pipepp.hpp>
#include "../helpers/test_sources.hpp"

using namespace pipepp::core;

TEST(PipeSyntaxTest, SubscribeAndFilter) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | subscribe("sensors/#", 0)
        | filter([](basic_message<default_config>&) { return true; })
        | sink([](const message_view&) {});
}

TEST(PipeSyntaxTest, TransformAndSink) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | transform([](basic_message<default_config>& m) { return true; })
        | sink([](const message_view&) {});
}

TEST(PipeSyntaxTest, MultipleStages) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | subscribe("a/#", 0)
        | transform([](basic_message<default_config>&) { return true; })
        | filter([](basic_message<default_config>&) { return true; })
        | transform([](basic_message<default_config>&) { return true; })
        | sink([](const message_view&) {});
}

TEST(PipeSyntaxTest, ConfigureOnly) {
    auto pipe = basic_pipeline<CountingSource>(CountingSource{})
        | configure([](auto&) -> result { return {}; })
        | sink([](const message_view&) {});
}