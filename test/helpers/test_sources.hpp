#pragma once

#include <pipepp/pipepp.hpp>

using namespace pipepp::core;

struct MockSource {
    int connect_count = 0;
    int disconnect_count = 0;
    int subscribe_count = 0;
    bool connected = false;
    std::string last_topic;
    int last_qos = 0;

    result connect(uri_view = {}) { ++connect_count; connected = true; return {}; }
    result disconnect() { ++disconnect_count; connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view topic, int qos) {
        ++subscribe_count;
        last_topic = std::string(topic);
        last_qos = qos;
        return {};
    }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config>) {}
    void poll() {}
};

struct CountingSource {
    int connect_count = 0;
    int disconnect_count = 0;
    int subscribe_count = 0;
    bool connected = false;

    result connect(uri_view = {}) { ++connect_count; connected = true; return {}; }
    result disconnect() { ++disconnect_count; connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view, int) { ++subscribe_count; return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config>) {}
    void poll() {}
};

struct FullSource {
    bool connected = false;
    std::string last_topic;
    int last_qos = -1;
    int publish_count = 0;
    message_callback<default_config> callback;

    result connect(uri_view = {}) { connected = true; return {}; }
    result disconnect() { connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view topic, int qos) {
        last_topic = std::string(topic);
        last_qos = qos;
        return {};
    }
    result publish(std::string_view, std::span<const std::byte>, int) {
        ++publish_count;
        return {};
    }
    void set_message_callback(message_callback<default_config> cb) { callback = std::move(cb); }
    void poll() {}
};

struct SelfStoppingSource {
    bool connected = false;
    int poll_count = 0;
    int max_polls = 1;
    message_callback<default_config> callback;

    result connect(uri_view = {}) { connected = true; poll_count = 0; return {}; }
    result disconnect() { connected = false; return {}; }
    bool is_connected() const { return connected; }
    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }
    void set_message_callback(message_callback<default_config> cb) { callback = std::move(cb); }
    void poll() {
        ++poll_count;
        if (callback && poll_count <= max_polls) {
            std::byte data[] = {std::byte{0x01}};
            message_view mv("auto/topic", data, 0);
            callback(mv);
        }
        if (poll_count >= max_polls) {
            connected = false;
        }
    }
};