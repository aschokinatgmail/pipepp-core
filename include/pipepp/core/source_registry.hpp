#pragma once

#include <atomic>
#include <cstddef>
#include <string_view>
#include <utility>

#include "any_source.hpp"
#include "config.hpp"
#include "fixed_string.hpp"
#include "uri.hpp"

namespace pipepp::core {

template<typename Config = default_config>
class source_registry {
public:
    using source_type = any_source<Config>;
    using factory_fn = source_type(*)(void*, uri_view);

    source_registry() = default;

    template<typename Source>
        requires BusSource<std::decay_t<Source>, Config>
    bool register_scheme(fixed_string<Config::max_scheme_len> scheme) {
        auto idx = count_.load(std::memory_order_acquire);
        if (idx >= Config::max_schemes) return false;
        entries_[idx].scheme = scheme;
        entries_[idx].factory = &factory_impl<std::decay_t<Source>>;
        entries_[idx].context = nullptr;
        std::atomic_thread_fence(std::memory_order_release);
        count_.store(idx + 1, std::memory_order_release);
        return true;
    }

    source_type create(std::string_view uri_or_scheme) const {
        std::string_view scheme = uri_or_scheme;
        auto scheme_end = uri_or_scheme.find("://");
        if (scheme_end != std::string_view::npos) {
            scheme = uri_or_scheme.substr(0, scheme_end);
        }
        auto count = count_.load(std::memory_order_acquire);
        for (std::size_t i = 0; i < count; ++i) {
            if (entries_[i].scheme == scheme) {
                uri_view uri;
                if (scheme_end != std::string_view::npos) {
                    uri = basic_uri<Config>::parse(uri_or_scheme).view();
                } else {
                    uri.scheme = scheme;
                    uri.full_str = uri_or_scheme;
                }
                return entries_[i].factory(entries_[i].context, uri);
            }
        }
        return source_type{};
    }

    bool has_scheme(std::string_view scheme) const noexcept {
        auto count = count_.load(std::memory_order_acquire);
        for (std::size_t i = 0; i < count; ++i) {
            if (entries_[i].scheme == scheme) return true;
        }
        return false;
    }

    std::size_t size() const noexcept { return count_.load(std::memory_order_acquire); }

private:
    struct entry {
        fixed_string<Config::max_scheme_len> scheme{};
        factory_fn factory = nullptr;
        void* context = nullptr;
    };

    template<typename Source>
    static source_type factory_impl(void*, uri_view) {
        return source_type(Source{});
    }

    std::array<entry, Config::max_schemes> entries_{};
    std::atomic<std::size_t> count_{0};
};

template<typename Config = default_config>
class source_factory {
public:
    template<typename Source>
        requires BusSource<std::decay_t<Source>, Config>
    static any_source<Config> create() {
        return any_source<Config>(Source{});
    }
};

} // namespace pipepp::core