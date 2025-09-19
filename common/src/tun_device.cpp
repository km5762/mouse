#include "tun_device.hpp"
#include <cstring>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

std::expected<TunDevice, std::error_code>
TunDevice::create(std::string_view name, short flags) {
  TunDevice device{};
  ifreq ifr{};

  ifr.ifr_flags = IFF_TUN | IFF_NO_PI | flags;
  std::strncpy(static_cast<char *>(ifr.ifr_name), name.data(), IFNAMSIZ - 1);

  device.fd_ = open("/dev/net/tun", O_RDWR);
  if (device.fd_ < 0) {
    return std::unexpected{std::error_code{errno, std::system_category()}};
  }

  if (ioctl(device.fd_, TUNSETIFF, &ifr) == -1) {
    close(device.fd_);
    return std::unexpected{std::error_code{errno, std::system_category()}};
  }

  return device;
}

std::expected<std::span<std::byte>, std::error_code> TunDevice::read() {
  ssize_t bytes_read{::read(fd_, buffer_.data(), buffer_.size())};
  if (bytes_read == -1) {
    return std::unexpected{std::error_code{errno, std::system_category()}};
  }

  return std::span{buffer_.data(), static_cast<std::size_t>(bytes_read)};
}

std::error_code TunDevice::write(std::span<const std::byte> data) const {
  if (::write(fd_, data.data(), data.size()) == -1) {
    return {errno, std::system_category()};
  }

  return {};
}

std::expected<std::vector<TunDevice>, std::error_code>
TunDevice::create_multiqueue(std::string_view name, std::size_t size,
                             short flags) {
  std::vector<TunDevice> multiqueue(size);
  for (std::size_t i{0}; i < size; ++i) {
    auto device{TunDevice::create(name, IFF_MULTI_QUEUE | flags)};
    if (!device) {
      return std::unexpected{device.error()};
    }

    multiqueue[i] = *device;
  }

  return multiqueue;
}
