#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "error_code.hpp"
#include "expected.hpp"
#include "message.hpp"
#include "message_callback.hpp"
#include "uri.hpp"

namespace pipepp::core {

template<typename T, typename Config = default_config>
concept BusSource = requires(T& t, uri_view uri, std::string_view topic,
                               std::span<const std::byte> payload, int qos,
                               message_callback<Config>&& cb) {
    { t.connect(uri) }   -> std::same_as<result>;
    { t.disconnect() }    -> std::same_as<result>;
    { t.is_connected() }  -> std::same_as<bool>;
    { t.subscribe(topic, qos) } -> std::same_as<result>;
    { t.publish(topic, payload, qos) } -> std::same_as<result>;
    { t.set_message_callback(std::move(cb)) } -> std::same_as<void>;
    { t.poll() } -> std::same_as<void>;
};

template<typename T, typename Config = default_config>
concept MessageHandler = requires(T& t, basic_message<Config>& msg) {
    { t(msg) } -> std::same_as<bool>;
} || requires(const T& t, message_view msg) {
    { t(msg) } -> std::same_as<void>;
};

} // namespace pipepp::core