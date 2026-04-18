#pragma once

#include <cstdint>

namespace pipepp::core {

enum class error_code : uint8_t {
    none = 0,
    buffer_overflow = 1,
    not_connected = 2,
    connection_failed = 3,
    invalid_argument = 7,
    out_of_range = 8,
    capacity_exceeded = 9,
    disconnected = 10,
    timeout = 11,
    stage_dropped = 12,
    invalid_uri = 13,
};

inline constexpr const char* error_code_message(error_code ec) noexcept {
    switch (ec) {
    case error_code::none:               return "no error";
    case error_code::buffer_overflow:   return "buffer overflow";
    case error_code::not_connected:     return "not connected";
    case error_code::connection_failed: return "connection failed";
    case error_code::invalid_argument:  return "invalid argument";
    case error_code::out_of_range:      return "out of range";
    case error_code::capacity_exceeded: return "capacity exceeded";
    case error_code::disconnected:      return "disconnected";
    case error_code::timeout:           return "timeout";
    case error_code::stage_dropped:     return "stage dropped";
    case error_code::invalid_uri:       return "invalid uri";
    default:                            return "unknown error";
    }
}

} // namespace pipepp::core