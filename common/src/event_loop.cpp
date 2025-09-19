#include "event_loop.hpp"

#include <cstdint>
#include <fcntl.h>
#include <sys/epoll.h>
#include <system_error>

std::error_code set_nonblocking(int fd) {
  unsigned int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    return {errno, std::system_category()};
  }

  return {};
}

std::error_code EventLoop::start() {
  if (fd_ == -1) {
    fd_ = epoll_create1(EPOLL_CLOEXEC);
  }
  if (fd_ == -1) {
    return {errno, std::system_category()};
  }

  while (true) {
    std::array<epoll_event, max_events> events{};
    for (int fd : removed_fds_) {
      handlers_.erase(fd);
    }
    removed_fds_.clear();

    const int number_of_events{epoll_wait(fd_, events.data(), max_events, -1)};

    if (number_of_events == -1) {
      return {errno, std::system_category()};
    }

    for (size_t i{0}; i < number_of_events; ++i) {
      const int fd = events.at(i).data.fd;

      if (!handlers_.contains(fd)) {
        continue;
      }

      Handler handler{handlers_.at(fd)};
      handler(events.at(i).events);
    }
  }
}

std::error_code EventLoop::add(int fd, uint32_t events, Handler handler) {
  epoll_event event{};
  event.events = events;
  event.data.fd = fd;

  if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
    return {errno, std::system_category()};
  }

  handlers_[fd] = std::move(handler);

  return {};
}

std::error_code EventLoop::remove(int fd) {
  if (epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
    return {errno, std::system_category()};
  }

  removed_fds_.push_back(fd);
  return {};
}

std::error_code EventLoop::modify(int fd, uint32_t events) const {
  epoll_event event{};
  event.events = events;
  event.data.fd = fd;
  if (epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &event) == -1) {
    return {errno, std::system_category()};
  }

  return {};
}
