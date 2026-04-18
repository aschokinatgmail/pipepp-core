#pragma once

#include <cstddef>
#include <iterator>
#include <span>
#include <string_view>
#include <utility>

#include "concepts.hpp"
#include "config.hpp"
#include "error_code.hpp"
#include "expected.hpp"
#include "message.hpp"
#include "message_callback.hpp"
#include "uri.hpp"

namespace pipepp::core {

template<typename Iterator, typename Config = default_config>
class range_source {
public:
    range_source() = default;

    range_source(Iterator begin, Iterator end)
        : begin_(begin), end_(end), current_(begin) {}

    result connect(uri_view = {}) {
        current_ = begin_;
        connected_ = true;
        return {};
    }

    result disconnect() {
        connected_ = false;
        return {};
    }

    bool is_connected() const { return connected_; }

    result subscribe(std::string_view, int) { return {}; }
    result publish(std::string_view, std::span<const std::byte>, int) { return {}; }

    void set_message_callback(message_callback<Config> cb) {
        callback_ = std::move(cb);
    }

    void poll() {
        while (current_ != end_) {
            if (callback_) {
                callback_(*current_);
            }
            ++current_;
        }
    }

private:
    Iterator begin_{};
    Iterator end_{};
    Iterator current_{};
    bool connected_ = false;
    message_callback<Config> callback_;
};

template<typename Iterator>
range_source(Iterator, Iterator) -> range_source<Iterator>;

template<typename Range, typename Config = default_config>
auto adapt(Range&& range) {
    using std::begin;
    using std::end;
    return range_source<decltype(begin(range)), Config>(begin(range), end(range));
}

} // namespace pipepp::core