#include "address_resolver.hpp"
#include <cstring>
#include <sys/socket.h>

auto AddressResolver::resolve(const Query &query)
    -> std::expected<std::vector<Address>, std::error_code> {
  auto cached{cache_.find(query)};
  if (cached != cache_.end()) {
    return cached->second;
  }

  addrinfo *results{};
  addrinfo hints{};
  hints.ai_flags = query.flags;
  hints.ai_family = query.family;
  hints.ai_socktype = query.type;
  hints.ai_protocol = query.protocol;
  int error{getaddrinfo(query.host ? query.host->data() : nullptr,
                        query.service ? query.service->data() : nullptr, &hints,
                        &results)};
  if (error != 0) {
    return std::unexpected{std::error_code{error, std::generic_category()}};
  }

  std::vector<Address> addresses{};
  for (addrinfo *result{results}; result != nullptr; result = result->ai_next) {
    addresses.emplace_back(
        *reinterpret_cast<sockaddr_storage *>(result->ai_addr),
        result->ai_addrlen);
  }

  freeaddrinfo(results);
  cache_[query] = addresses;
  return addresses;
}
