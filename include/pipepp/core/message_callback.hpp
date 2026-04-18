#pragma once

#include "config.hpp"
#include "fixed_string.hpp"
#include "message.hpp"
#include "small_function.hpp"

namespace pipepp::core {

template<typename Config = default_config>
struct subscription {
    fixed_string<Config::max_topic_len> topic{};
    int qos = 0;
};

template<typename Config = default_config>
using message_callback = small_function<void(const message_view&), Config::small_fn_size>;

} // namespace pipepp::core