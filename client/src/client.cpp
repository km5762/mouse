#include "client.hpp"
#include "event_loop.hpp"

#include <system_error>

std::error_code Client::start() {
  EventLoop event_loop{};

  constexpr auto tun_name{"mouse"};
  constexpr auto tun_size{16};
  auto tun_multiqueue{TunDevice::create_multiqueue(tun_name, tun_size)};
  if (!tun_multiqueue) {
    return tun_multiqueue.error();
  }

  for (const TunDevice &device : *tun_multiqueue) {
    std::error_code error{set_nonblocking(device.fd())};
    if (error) {
      return error;
    }

    error =
        event_loop.add(device.fd(), EPOLLIN | EPOLLOUT, [&](uint32_t events) {
          handle_tun_event(events, device);
        });
    if (error) {
      return error;
    }
  }
}
