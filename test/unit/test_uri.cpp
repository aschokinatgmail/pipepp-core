#include <gtest/gtest.h>
#include <pipepp/core/uri.hpp>

using namespace pipepp::core;

TEST(UriTest, MinimalParseScheme) {
    auto v = minimal_uri_parser::parse("mqtt://broker:1883/topic");
    EXPECT_EQ(v.scheme, "mqtt");
}

TEST(UriTest, MinimalParseHostPort) {
    auto v = minimal_uri_parser::parse("mqtt://broker:1883/topic");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.port, "1883");
}

TEST(UriTest, MinimalParsePath) {
    auto v = minimal_uri_parser::parse("mqtt://broker:1883/sensors/temp");
    EXPECT_EQ(v.path, "/sensors/temp");
}

TEST(UriTest, MinimalParseQuery) {
    auto v = minimal_uri_parser::parse("mqtt://broker:1883/topic?qos=1");
    EXPECT_EQ(v.query, "qos=1");
}

TEST(UriTest, MinimalParseFragment) {
    auto v = minimal_uri_parser::parse("mqtt://broker:1883/topic#frag");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, MinimalParseQueryAndFragment) {
    auto v = minimal_uri_parser::parse("mqtt://broker:1883/topic?qos=1#frag");
    EXPECT_EQ(v.query, "qos=1");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, MinimalParseEmptyUri) {
    auto v = minimal_uri_parser::parse("");
    EXPECT_TRUE(v.empty());
}

TEST(UriTest, MinimalParseNoPort) {
    auto v = minimal_uri_parser::parse("tcp://localhost/path");
    EXPECT_EQ(v.host, "localhost");
    EXPECT_EQ(v.port, "");
}

TEST(UriTest, MinimalParseNoPath) {
    auto v = minimal_uri_parser::parse("mqtt://broker:1883");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.port, "1883");
    EXPECT_EQ(v.path, "");
}

TEST(UriTest, MinimalParseRelativePath) {
    auto v = minimal_uri_parser::parse("/sensors/temp?qos=2");
    EXPECT_EQ(v.path, "/sensors/temp");
    EXPECT_EQ(v.query, "qos=2");
}

TEST(UriTest, MinimalParseUserinfo) {
    auto v = minimal_uri_parser::parse("mqtt://user:pass@broker:1883/topic");
    EXPECT_EQ(v.userinfo(), "user:pass");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.port, "1883");
}

TEST(UriTest, Rfc3986ParseScheme) {
    auto v = rfc3986_uri_parser::parse("mqtt://broker:1883/topic");
    EXPECT_EQ(v.scheme, "mqtt");
}

TEST(UriTest, Rfc3986ParseIpv6) {
    auto v = rfc3986_uri_parser::parse("mqtt://[::1]:1883/topic");
    EXPECT_EQ(v.host, "[::1]");
    EXPECT_EQ(v.port, "1883");
}

TEST(UriTest, Rfc3986ParseIpv6NoPort) {
    auto v = rfc3986_uri_parser::parse("mqtt://[::1]/topic");
    EXPECT_EQ(v.host, "[::1]");
    EXPECT_EQ(v.port, "");
}

TEST(UriTest, Rfc3986ParseUserinfo) {
    auto v = rfc3986_uri_parser::parse("mqtt://user:pass@broker:1883/topic");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.port, "1883");
    EXPECT_EQ(v.userinfo(), "user:pass");
}

TEST(UriTest, Rfc3986ParseFragment) {
    auto v = rfc3986_uri_parser::parse("mqtt://broker/topic#frag");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, BasicUriParse) {
    auto u = basic_uri<default_config>::parse("mqtt://broker:1883/topic");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_EQ(u.view().scheme, "mqtt");
    EXPECT_EQ(u.view().host, "broker");
    EXPECT_EQ(u.view().port, "1883");
}

TEST(UriTest, BasicUriParseOrDefault) {
    auto u = basic_uri<default_config>::parse_or_default("");
    EXPECT_FALSE(static_cast<bool>(u));

    auto u2 = basic_uri<default_config>::parse_or_default("mqtt://broker/topic");
    EXPECT_TRUE(static_cast<bool>(u2));
}

TEST(UriTest, UriViewDefaultIsEmpty) {
    uri_view v{};
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.scheme, "");
    EXPECT_EQ(v.host, "");
}

TEST(UriTest, ConfigSelectsMinimalParser) {
    static_assert(std::is_same_v<basic_uri<default_config>::parser_type, minimal_uri_parser>,
                  "default_config should use minimal_uri_parser");
}

TEST(UriTest, MinimalParseSchemelessQueryAndFragment) {
    auto v = minimal_uri_parser::parse("/path?key=val#frag");
    EXPECT_EQ(v.path, "/path");
    EXPECT_EQ(v.query, "key=val");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, MinimalParseSchemelessQueryOnly) {
    auto v = minimal_uri_parser::parse("/path?key=val");
    EXPECT_EQ(v.path, "/path");
    EXPECT_EQ(v.query, "key=val");
}

TEST(UriTest, MinimalParseSchemelessFragmentOnly) {
    auto v = minimal_uri_parser::parse("/path#frag");
    EXPECT_EQ(v.path, "/path");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, MinimalParseSchemelessPlainPath) {
    auto v = minimal_uri_parser::parse("/a/b/c");
    EXPECT_EQ(v.path, "/a/b/c");
}

TEST(UriTest, Rfc3986ParseWithQueryAndFragment) {
    auto v = rfc3986_uri_parser::parse("mqtt://broker:1883/topic?key=val#frag");
    EXPECT_EQ(v.scheme, "mqtt");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.port, "1883");
    EXPECT_EQ(v.path, "/topic");
    EXPECT_EQ(v.query, "key=val");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, Rfc3986ParseQueryOnly) {
    auto v = rfc3986_uri_parser::parse("mqtt://broker/topic?key=val");
    EXPECT_EQ(v.query, "key=val");
    EXPECT_EQ(v.fragment, "");
}

TEST(UriTest, Rfc3986ParseSchemelessQueryAndFragment) {
    auto v = rfc3986_uri_parser::parse("/path?key=val#frag");
    EXPECT_EQ(v.path, "/path");
    EXPECT_EQ(v.query, "key=val");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, Rfc3986ParseSchemelessQueryOnly) {
    auto v = rfc3986_uri_parser::parse("/path?key=val");
    EXPECT_EQ(v.path, "/path");
    EXPECT_EQ(v.query, "key=val");
}

TEST(UriTest, Rfc3986ParseSchemelessFragmentOnly) {
    auto v = rfc3986_uri_parser::parse("/path#frag");
    EXPECT_EQ(v.path, "/path");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, Rfc3986ParseSchemelessPlainPath) {
    auto v = rfc3986_uri_parser::parse("/a/b/c");
    EXPECT_EQ(v.path, "/a/b/c");
}

TEST(UriTest, Rfc3986ParseEmptyAuthority) {
    auto v = rfc3986_uri_parser::parse("mqtt:///topic");
    EXPECT_EQ(v.scheme, "mqtt");
    EXPECT_EQ(v.path, "/topic");
}

struct tiny_config : default_config {
    static constexpr std::size_t max_uri_len = 8;
};

TEST(UriTest, BasicUriOverflowReturnsEmpty) {
    auto u = basic_uri<tiny_config>::parse("mqtt://very-long-host:1883/topic");
    EXPECT_FALSE(static_cast<bool>(u));
}

TEST(UriTest, BasicUriTruncatedFlagFalse) {
    auto u = basic_uri<default_config>::parse("mqtt://broker:1883/topic");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, BasicUriTruncatedFlagTrue) {
    auto u = basic_uri<tiny_config>::parse("mqtt://very-long-host:1883/topic");
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_TRUE(u.truncated());
}

TEST(UriTest, BasicUriRebindPreservesOffsets) {
    auto u = basic_uri<default_config>::parse("mqtt://broker:1883/sensors/temp?qos=1#frag");
    EXPECT_TRUE(static_cast<bool>(u));
    auto& v = u.view();
    EXPECT_EQ(v.scheme, "mqtt");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.port, "1883");
    EXPECT_EQ(v.path, "/sensors/temp");
    EXPECT_EQ(v.query, "qos=1");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, UriViewUserinfoEmpty) {
    uri_view v{};
    EXPECT_EQ(v.userinfo(), "");
}

struct rfc_config : default_config {
    using uri_parser = rfc3986_uri_tag;
};

TEST(UriTest, ConfigSelectsRfcParser) {
    static_assert(std::is_same_v<basic_uri<rfc_config>::parser_type, rfc3986_uri_parser>,
                  "rfc_config should use rfc3986_uri_parser");
}