#include <algorithm>
#include <cstdint>
#include <cstring>
#include <expected>
#include <functional>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <span>
#include <string_view>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

struct Address {
  std::array<char, INET6_ADDRSTRLEN> host{};
  uint16_t port{};

  auto operator==(const Address &other) const -> bool {
    return host == other.host && port == other.port;
  }
};

struct Message {
  Address address;
  std::span<const std::byte> data;

  auto operator==(const Message &other) const -> bool {
    return address == other.address &&
           std::ranges::equal(data.begin(), data.end(), other.data.begin(),
                              other.data.end());
  }
};

using DnsResolver =
    std::function<std::expected<sockaddr_storage, std::error_code>(
        const Address &address)>;

static inline auto default_dns_resolver(const Address &address)
    -> std::expected<sockaddr_storage, std::error_code> {
  sockaddr_storage storage{};
  addrinfo *result{};
  addrinfo hints{.ai_family = AF_UNSPEC, .ai_socktype = SOCK_DGRAM};
  constexpr int max_port_digits{6};
  std::array<char, max_port_digits> port{};
  snprintf(port.data(), port.size(), "%u", address.port);
  int error{getaddrinfo(address.host.data(), port.data(), &hints, &result)};
  if (error != 0) {
    return std::unexpected{std::error_code{errno, std::generic_category()}};
  }
  std::memcpy(&storage, result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);
  return storage;
}

class UdpSocket {
public:
  UdpSocket() = default;
  UdpSocket(DnsResolver resolver, std::size_t buffer_size)
      : buffer_(buffer_size), resolver_{std::move(resolver)} {}
  explicit UdpSocket(std::size_t buffer_size) : buffer_(buffer_size) {};
  explicit UdpSocket(DnsResolver resolver) : resolver_{std::move(resolver)} {};
  UdpSocket(UdpSocket &&) = delete;
  auto operator=(UdpSocket &&) -> UdpSocket & = delete;
  UdpSocket(const UdpSocket &) = delete;
  auto operator=(const UdpSocket &) -> UdpSocket & = delete;
  ~UdpSocket() {
    if (close(socket_) != 0) {
      std::cerr << "~UdpSocket: Failed to close socket\n";
    }
  }

  auto bind(std::string_view port) -> std::error_code;
  auto read() -> std::expected<Message, std::error_code>;
  auto write(const Message &message) -> std::error_code;

private:
  static constexpr std::size_t default_buffer_size{65507};

  DnsResolver resolver_{default_dns_resolver};
  bool bound_{false};
  int socket_{-1};
  std::vector<std::byte> buffer_{default_buffer_size};
};
