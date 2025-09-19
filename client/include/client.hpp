#include "tun_device.hpp"

#include <cstdint>
#include <system_error>

class Client {
public:
  std::error_code start();

private:
  void handle_tun_event(uint32_t events, const TunDevice &device);
};
