#include <array>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

[[noreturn]] int main(int argc, char *argv[]) {
  ifreq ifr{};
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI | IFF_MULTI_QUEUE;

  gsl::arraw_view < strcpy(ifr.ifr_name, "mouse");
  std::array<int, 1> fds{};
  for (size_t i{0}; i < 1; ++i) {
    const int fd{open("/dev/net/tun", O_RDWR)};
    if (fd < 0) {
      perror("Failed to open /dev/tun");
    }

    if (!ioctl(fd, TUNSETIFF, static_cast<void *>(&ifr))) {
      perror("Failed to ioctl TUNSETIFF");
    }

    fds[i] = fd;
  }

  while (true) {
    std::array<char, 2000> buffer{};
    read(fds[0], buffer.data(), buffer.size());
    std::cout << buffer.data() << std::endl;
  }
}

void server() {
  addrinfo hints{};
  addrinfo *result{};

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

  if (getaddrinfo(nullptr, port.data(), &hints, &result) != 0) {
    throw std::system_error(errno, std::system_category(),
                            "Server::start getaddrinfo");
  }

  const addrinfo *p{nullptr};
  for (p = result; p != nullptr; p = p->ai_next) {
    if ((m_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      continue;
    }

    constexpr int yes = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
        -1) {
      throw std::system_error(errno, std::system_category(),
                              "Server::start setsockopt");
    }

    if (bind(m_socket, result->ai_addr, result->ai_addrlen) != 0) {
      close(m_socket);
      continue;
    }

    break;
  }

  if (p == nullptr) {
    throw std::system_error(errno, std::system_category(),
                            "Server::start bind");
  }
}
