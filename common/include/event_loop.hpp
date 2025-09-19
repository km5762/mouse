#include <any>
#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <sys/epoll.h>
#include <system_error>
#include <unordered_map>
#include <vector>

using Handler = std::function<void(uint32_t)>;

std::error_code set_nonblocking(int fd);

class EventLoop {
public:
  auto start() -> std::error_code;
  std::error_code add(int fd, uint32_t events, Handler handler);
  std::error_code remove(int fd);
  std::error_code modify(int fd, uint32_t events) const;
  int fd() const { return fd_; };

private:
  static constexpr int max_events{1024};

  int fd_{-1};
  std::unordered_map<int, Handler> handlers_;
  std::vector<int> removed_fds_;
};
