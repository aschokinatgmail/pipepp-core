#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#include "pipeline.hpp"

namespace pipepp::core {

template<typename Source, typename Config = default_config>
    requires BusSource<std::remove_reference_t<Source>, Config>
class pipeline_allocator {
public:
    static constexpr std::size_t required_size = sizeof(basic_pipeline<Source, Config>);
    static constexpr std::size_t required_align = alignof(basic_pipeline<Source, Config>);

    static basic_pipeline<Source, Config>* create(void* buffer, std::size_t buffer_size, Source src) {
        if (!buffer || buffer_size < required_size) return nullptr;
        if (reinterpret_cast<std::uintptr_t>(buffer) % required_align != 0) return nullptr;
        return ::new (buffer) basic_pipeline<Source, Config>(std::move(src));
    }

    static basic_pipeline<Source, Config>* create_uri(void* buffer, std::size_t buffer_size,
                                                       Source src, std::string_view uri_str) {
        if (!buffer || buffer_size < required_size) return nullptr;
        if (reinterpret_cast<std::uintptr_t>(buffer) % required_align != 0) return nullptr;
        return ::new (buffer) basic_pipeline<Source, Config>(std::move(src), uri_str);
    }

    static void destroy(basic_pipeline<Source, Config>* ptr) {
        if (ptr) ptr->~basic_pipeline();
    }
};

template<typename Source, typename Config = default_config>
std::size_t pipeline_size() noexcept {
    return sizeof(basic_pipeline<std::remove_reference_t<Source>, Config>);
}

template<typename Source, typename Config = default_config>
std::size_t pipeline_align() noexcept {
    return alignof(basic_pipeline<std::remove_reference_t<Source>, Config>);
}

} // namespace pipepp::core
