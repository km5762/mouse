#include "tun_device.hpp"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

auto main(int /*argc*/, char * /*argv*/[]) -> int {
  constexpr auto tun_device_name{"mouse"};
  constexpr auto tun_multiqueue_size{16};
  auto multique{
      TunDevice::create_multiqueue(tun_device_name, tun_multiqueue_size)};
  if (!multique) {
    std::cerr << "TunDevice::allocate_multiqueue: "
              << multique.error().message();
    return EXIT_FAILURE;
  }
}
