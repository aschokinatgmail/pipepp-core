#pragma once

#include <array>
#include <cstddef>

#include "message.hpp"
#include "config.hpp"

namespace pipepp::core {

template<typename Config = default_config>
class sink_buffer {
public:
    static constexpr std::size_t max_messages = Config::max_stages * 2;

    sink_buffer() = default;

    void push(const message_view& mv) {
        if (count_ < max_messages) {
            auto r = messages_[count_].from_view(mv);
            if (r) ++count_;
        }
    }

    std::size_t size() const noexcept { return count_; }
    bool empty() const noexcept { return count_ == 0; }

    const message_view operator[](std::size_t i) const {
        return messages_[i].as_view();
    }

    void clear() noexcept { count_ = 0; }

private:
    std::array<basic_message<Config>, max_messages> messages_{};
    std::size_t count_ = 0;
};

} // namespace pipepp::core