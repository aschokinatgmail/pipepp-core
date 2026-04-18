#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <span>

#include "config.hpp"
#include "error_code.hpp"
#include "expected.hpp"

namespace pipepp::core {

struct message_view {
    std::string_view topic{};
    std::span<const std::byte> payload{};
    std::uint8_t qos = 0;

    constexpr message_view() = default;
    constexpr message_view(std::string_view t, std::span<const std::byte> p, std::uint8_t q = 0)
        : topic(t), payload(p), qos(q) {}

    constexpr bool empty() const noexcept { return topic.empty() && payload.empty(); }
};


template<typename Config = default_config>
class basic_message {
public:
    basic_message() = default;

    result from_view(const message_view& mv) {
        if (mv.topic.size() > Config::max_topic_len ||
            mv.payload.size() > Config::max_payload_len) {
            return result{unexpect, make_unexpected(error_code::buffer_overflow)};
        }
        topic_len_ = mv.topic.size();
        std::memcpy(topic_buf_, mv.topic.data(), topic_len_);
        topic_buf_[topic_len_] = '\0';
        payload_len_ = mv.payload.size();
        std::memcpy(payload_buf_, mv.payload.data(), payload_len_);
        qos_ = mv.qos;
        return {};
    }

    message_view as_view() const noexcept {
        return message_view{
            std::string_view(topic_buf_, topic_len_),
            std::span<const std::byte>(payload_buf_, payload_len_),
            qos_
        };
    }

    std::string_view topic() const noexcept { return {topic_buf_, topic_len_}; }
    std::span<const std::byte> payload() const noexcept { return {payload_buf_, payload_len_}; }
    std::uint8_t qos() const noexcept { return qos_; }

private:
    char topic_buf_[Config::max_topic_len + 1]{};
    std::size_t topic_len_ = 0;
    std::byte payload_buf_[Config::max_payload_len]{};
    std::size_t payload_len_ = 0;
    std::uint8_t qos_ = 0;
};

using message = basic_message<default_config>;

} // namespace pipepp::core