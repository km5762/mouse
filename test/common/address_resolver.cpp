#include "address_resolver.hpp"
#include <gtest/gtest.h>
#include <system_error>

class AddressResolverTest : public ::testing::Test {
protected:
  AddressResolver resolver;
};

TEST_F(AddressResolverTest, ResolveValidHost) {
  AddressResolver::Query query{.host = "localhost",
                               .service = "http",
                               .flags = AI_PASSIVE,
                               .family = AF_UNSPEC,
                               .type = SOCK_STREAM,
                               .protocol = 0};

  auto result = resolver.resolve(query);
  EXPECT_TRUE(result.has_value()) << "Expected resolve to succeed";

  if (result) {
    EXPECT_FALSE(result->empty()) << "Expected at least one address";
    for (const auto &addr : *result) {
      EXPECT_GT(addr.length, 0u) << "Address length should be > 0";
    }
  }
}

TEST_F(AddressResolverTest, ResolveInvalidHost) {
  AddressResolver::Query query{.host = "invalid.host.local", .service = "http"};

  auto result = resolver.resolve(query);
  EXPECT_FALSE(result.has_value()) << "Expected resolve to fail";
  if (!result) {
    EXPECT_EQ(result.error().value(), EAI_NONAME);
  }
}

TEST_F(AddressResolverTest, ResolveCached) {
  AddressResolver::Query query{.host = "localhost",
                               .service = "http",
                               .flags = AI_PASSIVE,
                               .family = AF_UNSPEC,
                               .type = SOCK_STREAM,
                               .protocol = 0};

  auto first = resolver.resolve(query);
  ASSERT_TRUE(first.has_value());

  const auto &cached_vector = resolver.resolve(query).value();
  EXPECT_EQ(cached_vector.size(), first->size());
}
