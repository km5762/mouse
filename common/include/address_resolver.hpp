#include <cstring>
#include <expected>
#include <functional>
#include <netdb.h>
#include <optional>
#include <sys/socket.h>
#include <system_error>
#include <vector>

static inline void hash_combine(std::size_t &seed, std::size_t value) {
  seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct Address {
  sockaddr_storage storage{};
  socklen_t length{};

  Address() = default;
  Address(sockaddr_storage storage, socklen_t length)
      : storage{storage}, length{length} {};

  auto operator==(const Address &address) const -> bool {
    if (storage.ss_family != address.storage.ss_family ||
        length != address.length) {
      return false;
    }

    return std::memcmp(&storage, &address.storage, length) == 0;
  }
};

class AddressResolver {
public:
  struct Query {
    std::optional<std::string> host;
    std::optional<std::string> service;
    int flags{};
    int family{};
    int type{};
    int protocol{};

    auto operator==(const Query &query) const -> bool {
      return (host == query.host) && (service == query.service) &&
             flags == query.flags && family == query.family &&
             type == query.type && protocol == query.protocol;
    }

    struct Hash {
      auto operator()(const Query &query) const -> std::size_t {
        std::size_t hash{query.host ? std::hash<std::string>{}(*query.host)
                                    : 0};
        hash_combine(
            hash, query.service ? std::hash<std::string>{}(*query.service) : 0);
        hash_combine(hash, query.flags);
        hash_combine(hash, query.family);
        hash_combine(hash, query.type);
        hash_combine(hash, query.protocol);

        return hash;
      }
    };
  };

  auto resolve(const Query &query)
      -> std::expected<std::vector<Address>, std::error_code>;

private:
  std::unordered_map<Query, std::vector<Address>, Query::Hash> cache_;
};
