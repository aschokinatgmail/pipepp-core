#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

#include "error_code.hpp"
#include "expected.hpp"

namespace pipepp::core {

template<std::size_t N>
class fixed_string {
public:
    constexpr fixed_string() : size_(0) { data_[0] = '\0'; }

    constexpr fixed_string(const char* str, std::size_t len) : size_(std::min(len, N)) {
        for (std::size_t i = 0; i < size_; ++i)
            data_[i] = str[i];
        data_[size_] = '\0';
    }

    constexpr fixed_string(const char* str) : size_(0) {
        while (str[size_] && size_ < N)
            ++size_;
        for (std::size_t i = 0; i < size_; ++i)
            data_[i] = str[i];
        data_[size_] = '\0';
    }

    constexpr fixed_string(std::string_view sv)
        : fixed_string(sv.data(), sv.size()) {}

    template<std::size_t M, typename = std::enable_if_t<(M <= N)>>
    constexpr fixed_string(const fixed_string<M>& other) : size_(other.size()) {
        for (std::size_t i = 0; i < other.size(); ++i)
            data_[i] = other[i];
        data_[size_] = '\0';
    }

    constexpr std::size_t size() const noexcept { return size_; }
    constexpr std::size_t capacity() const noexcept { return N; }
    constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr const char* data() const noexcept { return data_; }
    constexpr const char* c_str() const noexcept { return data_; }

    constexpr char operator[](std::size_t i) const { return data_[i]; }
    constexpr char& operator[](std::size_t i) { return data_[i]; }

    constexpr operator std::string_view() const noexcept {
        return std::string_view(data_, size_);
    }

    constexpr const char* begin() const noexcept { return data_; }
    constexpr const char* end() const noexcept { return data_ + size_; }

    constexpr bool operator==(const fixed_string& other) const {
        if (size_ != other.size_) return false;
        for (std::size_t i = 0; i < size_; ++i)
            if (data_[i] != other.data_[i]) return false;
        return true;
    }

    constexpr bool operator==(std::string_view sv) const {
        return std::string_view(data_, size_) == sv;
    }

    constexpr bool operator==(const char* s) const {
        return std::string_view(data_, size_) == std::string_view(s);
    }

    constexpr bool operator!=(const fixed_string& other) const { return !(*this == other); }
    constexpr bool operator!=(std::string_view sv) const { return !(*this == sv); }

    constexpr bool operator<(const fixed_string& other) const {
        return std::string_view(data_, size_) < std::string_view(other.data_, other.size_);
    }

    constexpr bool operator<(std::string_view sv) const {
        return std::string_view(data_, size_) < sv;
    }

    constexpr result checked_from(std::string_view sv) {
        if (sv.size() > N)
            return result{unexpect, make_unexpected(error_code::buffer_overflow)};
        size_ = sv.size();
        for (std::size_t i = 0; i < size_; ++i)
            data_[i] = sv[i];
        data_[size_] = '\0';
        return {};
    }

private:
    char data_[N + 1]{};
    std::size_t size_;
};

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N - 1>;

} // namespace pipepp::core