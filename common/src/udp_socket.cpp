#include "udp_socket.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <expected>
#include <functional>
#include <linux/filter.h>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

auto UdpSocket::bind(std::span<const Address> addresses) -> std::error_code {
  if (bound_) {
    return std::make_error_code(std::errc::address_in_use);
  }
  for (std::size_t i{0}; i < addresses.size(); ++i) {
    socket_ = ::socket(addresses[i].storage.ss_family, SOCK_DGRAM, 0);
    if (socket_ == -1) {
      continue;
    }

    constexpr int yes = 1;
    setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (::bind(socket_,
               reinterpret_cast<const sockaddr *>(&addresses[i].storage),
               addresses[i].length) != 0) {
      int bind_errno = errno;
      ::close(socket_);
      socket_ = -1;

      if (i == addresses.size() - 1) {
        return {bind_errno, std::system_category()};
      }
      continue;
    }

    address_ = addresses[i];
    bound_ = true;
    return {};
  }

  return std::make_error_code(std::errc::invalid_argument);
}

auto UdpSocket::read() -> std::expected<Message, std::error_code> {
  sockaddr_storage address{};
  socklen_t address_length{sizeof(address)};
  assert(!buffer_.empty());
  ssize_t bytes_read{recvfrom(socket_, buffer_.data(), buffer_.size(), 0,
                              reinterpret_cast<sockaddr *>(&address),
                              &address_length)};
  if (bytes_read < 0) {
    return std::unexpected{std::error_code{errno, std::system_category()}};
  }
  return Message{
      .address = {address, address_length},
      .data = {buffer_.data(), static_cast<std::size_t>(bytes_read)}};
}

auto UdpSocket::write(const Message &message) -> std::error_code {
  if (!bound_) {
    if (socket_ != -1) {
      ::close(socket_);
    }
    socket_ = ::socket(message.address.storage.ss_family, SOCK_DGRAM, 0);
    if (socket_ == -1) {
      return {errno, std::system_category()};
    }
    ephemeral_ = true;
  }

  if (sendto(socket_, message.data.data(), message.data.size(), 0,
             reinterpret_cast<const sockaddr *>(&message.address.storage),
             message.address.length) < 0) {
    return {errno, std::system_category()};
  }

  bound_ = true;
  return {};
}

auto UdpSocket::address()
    -> std::expected<std::reference_wrapper<const Address>, std::error_code> {
  if (!bound_) {
    return std::unexpected{
        std::make_error_code(std::errc::bad_file_descriptor)};
  }

  if (ephemeral_) {
    socklen_t length{sizeof(address_.storage)};
    if (::getsockname(socket_, reinterpret_cast<sockaddr *>(&address_.storage),
                      &length) != 0) {
      std::cerr << strerror(errno) << "\n";
      return std::unexpected{std::error_code{errno, std::system_category()}};
    }
    address_.length = length;
  }

  return address_;
}
