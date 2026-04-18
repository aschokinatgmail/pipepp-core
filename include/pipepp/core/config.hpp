#pragma once

#include <cstddef>

namespace pipepp::core {

struct minimal_uri_tag {};
struct rfc3986_uri_tag {};

// Approximate sizeof(basic_pipeline<Source, default_config>) breakdown:
//   source_: sizeof(Source)
//   owned_uri_: fixed_string<max_uri_len+1> + uri_view ≈ max_uri_len + 144
//   config_ops_: max_config_ops * (small_fn_size + 32)
//   stages_: max_stages * (small_fn_size + 32)
//   sink_: (small_fn_size + 32)
//   subscriptions_: max_subscriptions * (max_topic_len + 1 + 8 + padding)
//   + ~40 bytes overhead (counters, atomic, spinlock, error_code)
//
// Total for a minimal source ≈ 84 + 400 + 4*160 + 8*160 + 160 + 8*280 + 40 ≈ 5164
struct default_config {
    using uri_parser = minimal_uri_tag;
    static constexpr std::size_t max_uri_len = 256;
    static constexpr std::size_t max_topic_len = 256;
    static constexpr std::size_t max_payload_len = 4096;
    static constexpr std::size_t max_stages = 8;
    static constexpr std::size_t max_config_ops = 4;
    static constexpr std::size_t max_subscriptions = 8;
    static constexpr std::size_t small_fn_size = 128;
    static constexpr std::size_t source_size = 2048;
    static constexpr std::size_t max_scheme_len = 16;
    static constexpr std::size_t max_schemes = 4;
};

// Approximate sizeof(basic_pipeline<Source, embedded_config>) breakdown:
//   Total for a minimal source ≈ 84 + 200 + 2*96 + 4*96 + 96 + 4*80 + 40 ≈ 1308
struct embedded_config {
    using uri_parser = minimal_uri_tag;
    static constexpr std::size_t max_uri_len = 128;
    static constexpr std::size_t max_topic_len = 64;
    static constexpr std::size_t max_payload_len = 512;
    static constexpr std::size_t max_stages = 4;
    static constexpr std::size_t max_config_ops = 2;
    static constexpr std::size_t max_subscriptions = 4;
    static constexpr std::size_t small_fn_size = 64;
    static constexpr std::size_t source_size = 512;
    static constexpr std::size_t max_scheme_len = 8;
    static constexpr std::size_t max_schemes = 2;
};

} // namespace pipepp::core