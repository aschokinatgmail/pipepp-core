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

static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic").scheme == "mqtt");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic").host == "broker");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic").port == "1883");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic").path == "/topic");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic?qos=1").query == "qos=1");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic#frag").fragment == "frag");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic?qos=1#frag").query == "qos=1");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883/topic?qos=1#frag").fragment == "frag");
static_assert(minimal_uri_parser::parse("").empty());
static_assert(minimal_uri_parser::parse("mqtt://broker:1883").host == "broker");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883").port == "1883");
static_assert(minimal_uri_parser::parse("mqtt://broker:1883").path.empty());
static_assert(minimal_uri_parser::parse("/path?q=1#f").path == "/path");
static_assert(minimal_uri_parser::parse("/path?q=1#f").query == "q=1");
static_assert(minimal_uri_parser::parse("/path?q=1#f").fragment == "f");
static_assert(minimal_uri_parser::parse("mqtt://user@h").userinfo() == "user");
static_assert(uri_view{}.empty());
static_assert(uri_view{}.userinfo().empty());

static_assert(rfc3986_uri_parser::parse("mqtt://broker:1883/topic").scheme == "mqtt");
static_assert(rfc3986_uri_parser::parse("mqtt://[::1]:1883/topic").host == "[::1]");
static_assert(rfc3986_uri_parser::parse("mqtt://[::1]:1883/topic").port == "1883");
static_assert(rfc3986_uri_parser::parse("mqtt://[::1]/topic").host == "[::1]");
static_assert(rfc3986_uri_parser::parse("mqtt://[::1]/topic").port.empty());
static_assert(rfc3986_uri_parser::parse("mqtt://user@h/topic").userinfo() == "user");

template<auto Uri>
constexpr bool nttp_scheme_is(std::string_view expected) {
    return minimal_uri_parser::parse(static_cast<std::string_view>(Uri)).scheme == expected;
}

static_assert(nttp_scheme_is<fixed_string_literal("mqtt://broker:1883")>("mqtt"));
static_assert(!nttp_scheme_is<fixed_string_literal("tcp://host")>("mqtt"));
static_assert(fixed_string_literal("hello").size() == 5);
static_assert(fixed_string_literal("") == std::string_view(""));

TEST(UriTest, ConstexprMinimalScheme) {
    constexpr auto v = minimal_uri_parser::parse("mqtt://broker:1883/topic");
    EXPECT_EQ(v.scheme, "mqtt");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.port, "1883");
    EXPECT_EQ(v.path, "/topic");
}

TEST(UriTest, ConstexprRfc3986Ipv6) {
    constexpr auto v = rfc3986_uri_parser::parse("mqtt://[::1]:1883/topic");
    EXPECT_EQ(v.host, "[::1]");
    EXPECT_EQ(v.port, "1883");
}

TEST(UriTest, BasicUriParseEmptyDirect) {
    auto u = basic_uri<default_config>::parse("");
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, BasicUriStorageAccessor) {
    auto u = basic_uri<default_config>::parse("mqtt://broker/topic");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_EQ(u.storage(), "mqtt://broker/topic");
}

TEST(UriTest, Rfc3986ParseFragmentInAuthority) {
    auto v = rfc3986_uri_parser::parse("mqtt://broker#frag");
    EXPECT_EQ(v.scheme, "mqtt");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.fragment, "frag");
}

TEST(UriTest, Rfc3986ParseQueryInAuthority) {
    auto v = rfc3986_uri_parser::parse("mqtt://broker?q=1");
    EXPECT_EQ(v.scheme, "mqtt");
    EXPECT_EQ(v.host, "broker");
    EXPECT_EQ(v.query, "q=1");
}

TEST(UriTest, FixedStringLiteralBasic) {
    constexpr auto lit = fixed_string_literal("mqtt://broker:1883");
    EXPECT_EQ(lit.size(), 18u);
    EXPECT_EQ(static_cast<std::string_view>(lit), "mqtt://broker:1883");
}

TEST(UriTest, FixedStringLiteralEmpty) {
    constexpr auto lit = fixed_string_literal("");
    EXPECT_TRUE(lit.empty());
}

TEST(UriTest, FixedStringLiteralNttpParse) {
    auto v = minimal_uri_parser::parse(static_cast<std::string_view>(fixed_string_literal("tcp://host:80/path")));
    EXPECT_EQ(v.scheme, "tcp");
    EXPECT_EQ(v.host, "host");
    EXPECT_EQ(v.port, "80");
    EXPECT_EQ(v.path, "/path");
}

TEST(UriTest, EmbeddedConfigParseAllFields) {
    auto u = basic_uri<embedded_config>::parse("mqtt://broker:1883/sensors/temp?qos=1#frag");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
    EXPECT_EQ(u.view().scheme, "mqtt");
    EXPECT_EQ(u.view().host, "broker");
    EXPECT_EQ(u.view().port, "1883");
    EXPECT_EQ(u.view().path, "/sensors/temp");
    EXPECT_EQ(u.view().query, "qos=1");
    EXPECT_EQ(u.view().fragment, "frag");
}

TEST(UriTest, EmbeddedConfigParseOverflow) {
    std::string long_uri(200, 'a');
    auto u = basic_uri<embedded_config>::parse(long_uri);
    EXPECT_TRUE(u.truncated());
    EXPECT_FALSE(static_cast<bool>(u));
}

TEST(UriTest, EmbeddedConfigParseOrDefaultEmpty) {
    auto u = basic_uri<embedded_config>::parse_or_default("");
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, EmbeddedConfigParseOrDefaultNonEmpty) {
    auto u = basic_uri<embedded_config>::parse_or_default("tcp://host/path");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_EQ(u.view().scheme, "tcp");
    EXPECT_EQ(u.view().host, "host");
    EXPECT_EQ(u.view().path, "/path");
}

TEST(UriTest, EmbeddedConfigParseEmpty) {
    auto u = basic_uri<embedded_config>::parse("");
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, RfcConfigBasicUriParseAllFields) {
    auto u = basic_uri<rfc_config>::parse("mqtt://broker:1883/path?q=1#frag");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
    EXPECT_EQ(u.view().scheme, "mqtt");
    EXPECT_EQ(u.view().host, "broker");
    EXPECT_EQ(u.view().port, "1883");
    EXPECT_EQ(u.view().authority, "broker:1883");
    EXPECT_EQ(u.view().path, "/path");
    EXPECT_EQ(u.view().query, "q=1");
    EXPECT_EQ(u.view().fragment, "frag");
}

TEST(UriTest, RfcConfigBasicUriParseOrDefaultEmpty) {
    auto u = basic_uri<rfc_config>::parse_or_default("");
    EXPECT_FALSE(static_cast<bool>(u));
}

TEST(UriTest, RfcConfigBasicUriParseOrDefaultNonEmpty) {
    auto u = basic_uri<rfc_config>::parse_or_default("mqtt://broker:1883/topic");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_EQ(u.view().scheme, "mqtt");
    EXPECT_EQ(u.view().host, "broker");
    EXPECT_EQ(u.view().port, "1883");
}

TEST(UriTest, RfcConfigBasicUriParseEmpty) {
    auto u = basic_uri<rfc_config>::parse("");
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, TinyConfigExactFitNoTruncation) {
    auto u = basic_uri<tiny_config>::parse("tcp://ab");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
    EXPECT_EQ(u.view().scheme, "tcp");
    EXPECT_EQ(u.view().host, "ab");
}

TEST(UriTest, BasicUriDefaultConstructedIsFalse) {
    basic_uri<default_config> u;
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, EmbeddedConfigDefaultConstructedIsFalse) {
    basic_uri<embedded_config> u;
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, RfcConfigDefaultConstructedIsFalse) {
    basic_uri<rfc_config> u;
    EXPECT_FALSE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
}

TEST(UriTest, BasicUriParseWithAuthorityUserinfo) {
    auto u = basic_uri<default_config>::parse("mqtt://user:pass@broker:1883/topic?q=1#frag");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
    EXPECT_EQ(u.view().authority, "user:pass@broker:1883");
    EXPECT_EQ(u.view().host, "broker");
    EXPECT_EQ(u.view().port, "1883");
    EXPECT_EQ(u.view().path, "/topic");
    EXPECT_EQ(u.view().query, "q=1");
    EXPECT_EQ(u.view().fragment, "frag");
}

TEST(UriTest, RfcConfigParseIpv6AllFields) {
    auto u = basic_uri<rfc_config>::parse("mqtt://[::1]:1883/path?q=1#frag");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_FALSE(u.truncated());
    EXPECT_EQ(u.view().host, "[::1]");
    EXPECT_EQ(u.view().port, "1883");
    EXPECT_EQ(u.view().path, "/path");
    EXPECT_EQ(u.view().query, "q=1");
    EXPECT_EQ(u.view().fragment, "frag");
}

TEST(UriTest, BasicUriParseSchemelessAllFields) {
    auto u = basic_uri<default_config>::parse("/path?key=val#frag");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_EQ(u.view().scheme, "");
    EXPECT_EQ(u.view().host, "");
    EXPECT_EQ(u.view().port, "");
    EXPECT_EQ(u.view().path, "/path");
    EXPECT_EQ(u.view().query, "key=val");
    EXPECT_EQ(u.view().fragment, "frag");
}

TEST(UriTest, EmbeddedConfigParseWithUserinfo) {
    auto u = basic_uri<embedded_config>::parse("mqtt://user@host:123/p?q=1#f");
    EXPECT_TRUE(static_cast<bool>(u));
    EXPECT_EQ(u.view().authority, "user@host:123");
    EXPECT_EQ(u.view().host, "host");
    EXPECT_EQ(u.view().port, "123");
    EXPECT_EQ(u.view().path, "/p");
    EXPECT_EQ(u.view().query, "q=1");
    EXPECT_EQ(u.view().fragment, "f");
}

TEST(UriTest, TinyConfigOverflowOneOver) {
    std::string uri(9, 'x');
    auto u = basic_uri<tiny_config>::parse(uri);
    EXPECT_TRUE(u.truncated());
    EXPECT_FALSE(static_cast<bool>(u));
}