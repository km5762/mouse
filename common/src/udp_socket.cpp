#include "udp_socket.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <expected>
#include <linux/filter.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

auto UdpSocket::bind(std::string_view port) -> std::error_code {
  addrinfo hints{
      .ai_flags = AI_PASSIVE,
      .ai_family = AF_UNSPEC,
      .ai_socktype = SOCK_DGRAM,
  };
  addrinfo *result{};

  int error{(
      getaddrinfo(nullptr, std::string(port.data()).c_str(), &hints, &result))};
  if (error != 0) {
    return {error, std::generic_category()};
  }

  const addrinfo *ptr{nullptr};
  for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
    if (socket_ == -1) {
      socket_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    };

    if (socket_ == -1) {
      continue;
    }

    constexpr int yes = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
        -1) {
      return {errno, std::system_category()};
    }

    if (::bind(socket_, ptr->ai_addr, ptr->ai_addrlen) != 0) {
      close(socket_);
      continue;
    }

    break;
  }

  if (ptr == nullptr) {
    return {errno, std::system_category()};
  }

  bound_ = true;
  freeaddrinfo(result);
  return {};
}

auto UdpSocket::read() -> std::expected<Message, std::error_code> {
  sockaddr_storage address{};
  socklen_t address_length{sizeof(address)};
  ssize_t bytes_read{recvfrom(socket_, buffer_.data(), buffer_.size(), 0,
                              reinterpret_cast<sockaddr *>(&address),
                              &address_length)};
  if (bytes_read < 0) {
    return std::unexpected{std::error_code{errno, std::system_category()}};
  }

  std::array<char, INET6_ADDRSTRLEN> host{};
  uint16_t port{};
  if (address.ss_family == AF_INET) {
    const auto *ipv4{reinterpret_cast<const sockaddr_in *>(&address)};
    inet_ntop(AF_INET, &(ipv4->sin_addr), host.data(), host.size());
    port = ipv4->sin_port;
  } else if (address.ss_family == AF_INET6) {
    const auto *ipv6{reinterpret_cast<const sockaddr_in6 *>(&address)};
    inet_ntop(AF_INET, &(ipv6->sin6_addr), host.data(), host.size());
    port = ipv6->sin6_port;
  } else {
    return std::unexpected{
        std::make_error_code(std::errc::address_family_not_supported)};
  }

  return Message{
      .address = {.host = host, .port = port},
      .data = {buffer_.data(), static_cast<std::size_t>(bytes_read)}};
}

auto UdpSocket::write(const Message &message) -> std::error_code {
  auto address{resolver_(message.address)};
  if (!address) {
    return address.error();
  }

  if (sendto(socket_, buffer_.data(), buffer_.size(), 0,
             reinterpret_cast<sockaddr *>(&address.value()),
             sizeof(*address)) != 0) {
    return {errno, std::system_category()};
  }

  return {};
}
