#include "address_resolver.hpp"
#include <algorithm>
#include <cstring>
#include <expected>
#include <netdb.h>
#include <netinet/in.h>
#include <span>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

struct Message {
  Address address;
  std::span<const std::byte> data;

  auto operator==(const Message &message) const -> bool {
    return address == message.address &&
           std::ranges::equal(data.begin(), data.end(), message.data.begin(),
                              message.data.end());
  }
};

class UdpSocket {
public:
  UdpSocket() = default;
  explicit UdpSocket(std::size_t buffer_size) : buffer_(buffer_size) {};
  UdpSocket(UdpSocket &&socket) noexcept
      : address_{socket.address_}, bound_(socket.bound_),
        fd_(std::exchange(socket.fd_, -1)), ephemeral_(socket.ephemeral_),
        buffer_(std::move(socket.buffer_)) {}
  auto operator=(UdpSocket &&socket) -> UdpSocket & {
    if (this != &socket) {
      if (fd_ != -1) {
        ::close(fd_);
      }

      address_ = socket.address_;
      bound_ = socket.bound_;
      fd_ = std::exchange(socket.fd_, -1);
      ephemeral_ = socket.ephemeral_;
      buffer_ = std::move(socket.buffer_);
    }

    return *this;
  }
  UdpSocket(const UdpSocket &) = delete;
  auto operator=(const UdpSocket &) -> UdpSocket & = delete;
  ~UdpSocket() {
    if (fd_ != -1) {
      ::close(fd_);
    }
  }
  auto bind(std::span<const Address> addresses) -> std::error_code;
  auto read() -> std::expected<Message, std::error_code>;
  [[nodiscard]] auto write(const Message &message) -> std::error_code;
  auto address()
      -> std::expected<std::reference_wrapper<const Address>, std::error_code>;
  [[nodiscard]] int fd() const { return fd_; };

private:
  static constexpr std::size_t default_buffer_size{4096};
  Address address_;
  bool bound_{false};
  int fd_{-1};
  bool ephemeral_{false};
  std::vector<std::byte> buffer_{default_buffer_size};
};
