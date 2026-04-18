#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "error_code.hpp"
#include "expected.hpp"
#include "message.hpp"
#include "small_function.hpp"

namespace pipepp::core {

template<typename Config = default_config>
class config_fn {
public:
    config_fn() = default;

    template<typename Source, typename F>
    static config_fn create(F&& f) {
        auto wrapper = [fn = std::forward<F>(f)](void* src) mutable -> result {
            return fn(*static_cast<Source*>(src));
        };
        return config_fn(std::move(wrapper));
    }

    result apply(void* source) const {
        return fn_(source);
    }

    explicit operator bool() const noexcept { return static_cast<bool>(fn_); }

private:
    template<typename F>
        requires (!std::is_same_v<std::decay_t<F>, config_fn>)
    explicit config_fn(F&& f) : fn_(std::forward<F>(f)) {}

    small_function<result(void*), Config::small_fn_size> fn_;
};

template<typename Config = default_config>
class process_fn {
public:
    process_fn() = default;

    template<typename F>
        requires (!std::is_same_v<std::decay_t<F>, process_fn>)
    process_fn(F&& f) : fn_(std::forward<F>(f)) {}

    bool operator()(basic_message<Config>& msg) const {
        return fn_(msg);
    }

    explicit operator bool() const noexcept { return static_cast<bool>(fn_); }

private:
    small_function<bool(basic_message<Config>&), Config::small_fn_size> fn_;
};

template<typename Config = default_config>
class sink_fn {
public:
    sink_fn() = default;

    template<typename F>
        requires (!std::is_same_v<std::decay_t<F>, sink_fn>)
    sink_fn(F&& f) : fn_(std::forward<F>(f)) {}

    void operator()(const message_view& msg) const {
        fn_(msg);
    }

    explicit operator bool() const noexcept { return static_cast<bool>(fn_); }

private:
    small_function<void(const message_view&), Config::small_fn_size> fn_;
};

} // namespace pipepp::core