#include <gtest/gtest.h>
#include <pipepp/core/concepts.hpp>
#include <pipepp/core/error_code.hpp>
#include <pipepp/core/expected.hpp>
#include <pipepp/core/message_callback.hpp>
#include <pipepp/core/uri.hpp>

using namespace pipepp::core;

struct ValidSource {
    result connect(uri_view) { return {}; }
    result disconnect() { return {}; }
    bool is_connected() const { return true; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config>) {}
    void poll() {}
};

struct InvalidSourceNoConnect {
    result disconnect() { return {}; }
    bool is_connected() const { return false; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config>) {}
    void poll() {}
};

struct InvalidSourceWrongReturnType {
    bool connect(uri_view) { return true; }
    result disconnect() { return {}; }
    bool is_connected() const { return false; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config>) {}
    void poll() {}
};

struct InvalidSourceNoCallback {
    result connect(uri_view) { return {}; }
    result disconnect() { return {}; }
    bool is_connected() const { return true; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
};

struct InvalidSourceNoPoll {
    result connect(uri_view) { return {}; }
    result disconnect() { return {}; }
    bool is_connected() const { return true; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config>) {}
};

TEST(ConceptTest, ValidBusSource) {
    static_assert(BusSource<ValidSource>, "ValidSource must satisfy BusSource");
}

TEST(ConceptTest, InvalidBusSourceNoConnect) {
    static_assert(!BusSource<InvalidSourceNoConnect>, "Missing connect should not satisfy BusSource");
}

TEST(ConceptTest, InvalidBusSourceWrongReturn) {
    static_assert(!BusSource<InvalidSourceWrongReturnType>, "Wrong return type should not satisfy BusSource");
}

TEST(ConceptTest, InvalidBusSourceNoCallback) {
    static_assert(!BusSource<InvalidSourceNoCallback>, "Missing set_message_callback should not satisfy BusSource");
}

TEST(ConceptTest, InvalidBusSourceNoPoll) {
    static_assert(!BusSource<InvalidSourceNoPoll>, "Missing poll should not satisfy BusSource");
}