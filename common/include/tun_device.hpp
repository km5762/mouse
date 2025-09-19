#include <expected>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

class TunDevice {
public:
  static std::expected<TunDevice, std::error_code> create(std::string_view name,
                                                          short flags = 0);
  std::expected<std::span<std::byte>, std::error_code> read();
  [[nodiscard]] std::error_code write(std::span<const std::byte> data) const;
  std::expected<std::vector<TunDevice>,
                std::error_code> static create_multiqueue(std::string_view name,
                                                          std::size_t size,
                                                          short flags = 0);
  [[nodiscard]] int fd() const { return fd_; };

private:
  static constexpr std::size_t mtu_{2000};
  int fd_{};
  std::array<std::byte, mtu_> buffer_{};
};
