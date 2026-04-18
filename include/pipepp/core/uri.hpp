#pragma once

#include <cstddef>
#include <string_view>

#include "config.hpp"
#include "error_code.hpp"
#include "expected.hpp"
#include "fixed_string.hpp"

namespace pipepp::core {

struct uri_view {
    std::string_view scheme{};
    std::string_view authority{};
    std::string_view host{};
    std::string_view port{};
    std::string_view path{};
    std::string_view query{};
    std::string_view fragment{};
    std::string_view full_str{};

    constexpr bool empty() const noexcept { return scheme.empty() && authority.empty() && path.empty(); }

    constexpr std::string_view userinfo() const noexcept {
        if (auto at = authority.find('@'); at != std::string_view::npos) {
            return authority.substr(0, at);
        }
        return {};
    }
};

namespace detail {

struct parsed_path {
    std::string_view path;
    std::string_view query;
    std::string_view fragment;
};

constexpr parsed_path parse_path_query_fragment(std::string_view str) {
    parsed_path result;
    auto qpos = str.find('?');
    auto hpos = str.find('#');
    std::size_t path_len = str.size();
    if (qpos != std::string_view::npos) path_len = qpos;
    if (hpos != std::string_view::npos && hpos < path_len) path_len = hpos;
    if (path_len > 0) result.path = str.substr(0, path_len);

    if (qpos != std::string_view::npos) {
        auto after_q = str.substr(qpos + 1);
        auto hpos2 = after_q.find('#');
        if (hpos2 != std::string_view::npos) {
            result.query = after_q.substr(0, hpos2);
            result.fragment = after_q.substr(hpos2 + 1);
        } else {
            result.query = after_q;
        }
    } else if (hpos != std::string_view::npos) {
        result.fragment = str.substr(hpos + 1);
    }
    return result;
}

constexpr void parse_authority_minimal(std::string_view auth, uri_view& out) {
    out.authority = auth;
    auto at_sign = auth.find('@');
    std::string_view host_port = auth;
    if (at_sign != std::string_view::npos) host_port = auth.substr(at_sign + 1);
    auto colon = host_port.rfind(':');
    if (colon != std::string_view::npos) {
        out.host = host_port.substr(0, colon);
        out.port = host_port.substr(colon + 1);
    } else {
        out.host = host_port;
    }
}

constexpr void parse_authority_rfc3986(std::string_view auth, uri_view& out) {
    out.authority = auth;
    if (auth.empty()) return;
    auto at_sign = auth.find('@');
    std::string_view host_port = auth;
    if (at_sign != std::string_view::npos) host_port = auth.substr(at_sign + 1);

    if (host_port.size() >= 2 && host_port.front() == '[') {
        auto bracket_end = host_port.find(']');
        if (bracket_end != std::string_view::npos) {
            out.host = host_port.substr(0, bracket_end + 1);
            auto after = host_port.substr(bracket_end + 1);
            if (!after.empty() && after.front() == ':') {
                out.port = after.substr(1);
            }
        }
    } else {
        auto colon = host_port.rfind(':');
        if (colon != std::string_view::npos) {
            out.host = host_port.substr(0, colon);
            out.port = host_port.substr(colon + 1);
        } else {
            out.host = host_port;
        }
    }
}

constexpr void parse_schemeless(std::string_view uri, uri_view& result) {
    auto pp = parse_path_query_fragment(uri);
    result.path = pp.path;
    result.query = pp.query;
    result.fragment = pp.fragment;
}

} // namespace detail

struct minimal_uri_parser {
    static constexpr uri_view parse(std::string_view uri) {
        uri_view result{};
        if (uri.empty()) return result;
        result.full_str = uri;

        auto scheme_end = uri.find("://");
        if (scheme_end != std::string_view::npos) {
            result.scheme = uri.substr(0, scheme_end);
            auto rest = uri.substr(scheme_end + 3);
            auto path_start = rest.find('/');
            auto authority = (path_start != std::string_view::npos) ? rest.substr(0, path_start) : rest;
            detail::parse_authority_minimal(authority, result);
            if (path_start != std::string_view::npos) {
                auto pp = detail::parse_path_query_fragment(rest.substr(path_start));
                result.path = pp.path;
                result.query = pp.query;
                result.fragment = pp.fragment;
            }
        } else {
            detail::parse_schemeless(uri, result);
        }
        return result;
    }
};

struct rfc3986_uri_parser {
    static constexpr uri_view parse(std::string_view uri) {
        uri_view result{};
        if (uri.empty()) return result;
        result.full_str = uri;

        auto scheme_end = uri.find("://");
        if (scheme_end != std::string_view::npos) {
            result.scheme = uri.substr(0, scheme_end);
            auto rest = uri.substr(scheme_end + 3);

            auto path_start = rest.find('/');
            auto query_start = rest.find('?');
            auto frag_start = rest.find('#');
            std::size_t auth_end = rest.size();
            if (path_start != std::string_view::npos) auth_end = path_start;
            if (query_start != std::string_view::npos && query_start < auth_end) auth_end = query_start;
            if (frag_start != std::string_view::npos && frag_start < auth_end) auth_end = frag_start;

            detail::parse_authority_rfc3986(rest.substr(0, auth_end), result);
            auto after_auth = rest.substr(auth_end);
            if (!after_auth.empty()) {
                auto pp = detail::parse_path_query_fragment(after_auth);
                result.path = pp.path;
                result.query = pp.query;
                result.fragment = pp.fragment;
            }
        } else {
            detail::parse_schemeless(uri, result);
        }
        return result;
    }
};

template<typename Tag>
struct uri_parser_map;

template<>
struct uri_parser_map<minimal_uri_tag> {
    using type = minimal_uri_parser;
};

template<>
struct uri_parser_map<rfc3986_uri_tag> {
    using type = rfc3986_uri_parser;
};

template<typename Config = default_config>
class basic_uri {
public:
    using parser_type = typename uri_parser_map<typename Config::uri_parser>::type;

    basic_uri() = default;

    static basic_uri parse(std::string_view uri_str) {
        basic_uri result;
        auto r = result.storage_.checked_from(uri_str);
        if (!r) {
            result.truncated_ = true;
            result.storage_ = fixed_string<Config::max_uri_len>();
            return result;
        }
        result.view_ = parser_type::parse(std::string_view(result.storage_));
        rebind_view(result);
        return result;
    }

    static basic_uri parse_or_default(std::string_view uri_str) {
        if (uri_str.empty()) return {};
        return parse(uri_str);
    }

    const uri_view& view() const noexcept { return view_; }
    const auto& storage() const noexcept { return storage_; }

    explicit operator bool() const noexcept { return !view_.empty(); }

    bool truncated() const noexcept { return truncated_; }

private:
    static void rebind_view(basic_uri& u) {
        if (u.view_.full_str.empty()) return;
        auto base = std::string_view(u.storage_);
        auto rebind = [&](std::string_view sv) -> std::string_view {
            if (sv.empty()) return sv;
            auto offset = static_cast<std::size_t>(sv.data() - u.view_.full_str.data());
            return base.substr(offset, sv.size());
        };
        u.view_.scheme = rebind(u.view_.scheme);
        u.view_.authority = rebind(u.view_.authority);
        u.view_.host = rebind(u.view_.host);
        u.view_.port = rebind(u.view_.port);
        u.view_.path = rebind(u.view_.path);
        u.view_.query = rebind(u.view_.query);
        u.view_.fragment = rebind(u.view_.fragment);
        u.view_.full_str = base;
    }

    fixed_string<Config::max_uri_len> storage_{};
    uri_view view_{};
    bool truncated_ = false;
};

using uri = basic_uri<default_config>;

} // namespace pipepp::core