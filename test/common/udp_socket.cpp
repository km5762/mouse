#include "udp_socket.hpp"
#include <gtest/gtest.h>

class UdpSocketTest : public testing::Test {
protected:
  const Address server_address_{.host = {"127.0.0.1"}, .port = server_port_};

  void start_server(std::function<void(UdpSocket &, const Message &)> handler) {
    server_ = std::thread{[&] {
      UdpSocket socket{};
      std::error_code error{socket.bind("121212")};
      ASSERT_FALSE(error) << error.message();

      {
        std::lock_guard lock{mtx_};
        server_ready_ = true;
      }

      server_running_ = true;
      while (server_running_) {
        auto message{socket.read()};
        ASSERT_TRUE(message) << message.error().message();
        handler(socket, *message);
      }
    }};

    {
      std::unique_lock lock{mtx_};
      cv_.wait(lock, [&] { return server_ready_; });
    }
  }

  void TearDown() override {
    server_running_ = false;
    if (server_.joinable()) {
      server_.join();
    }
  }

private:
  const uint16_t server_port_{7357};
  std::atomic<bool> server_running_{false};
  std::thread server_;
  std::condition_variable cv_;
  bool server_ready_{false};
  std::mutex mtx_;
};

TEST(UdpSocket, Bind) {
  UdpSocket socket{};
  std::error_code error{socket.bind("6789")};
  ASSERT_FALSE(error) << error.message();
}

TEST(UdpSocket, DoubleBind) {
  UdpSocket socket{};
  std::error_code error{socket.bind("6789")};
  error = socket.bind("12345");
  ASSERT_TRUE(error);
}

TEST(UdpSocket, BindUnterminatedPort) {
  UdpSocket socket{};
  const std::array<char, 3> port_unterminated = {'6', '7', '8'};
  std::error_code error{socket.bind(port_unterminated.data())};
  ASSERT_TRUE(error);
}

TEST(UdpSocket, SendBeforeBind) {
  UdpSocket socket{};
  std::error_code error{socket.write({})};
  ASSERT_TRUE(error);
}

TEST(UdpSocket, SendInvalidAddress) {
  UdpSocket socket{};
  std::string data{"hello"};
  std::error_code error{socket.write(
      {.address = {},
       .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}})};
  ASSERT_TRUE(error);
}

TEST_F(UdpSocketTest, Send) {
  start_server([](UdpSocket &socket, const Message &message) {});
  UdpSocket socket{};
  std::string data{"hello"};
  std::error_code error{socket.write(
      {.address = server_address_,
       .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}})};
  ASSERT_FALSE(error) << error.message();
}

TEST_F(UdpSocketTest, Read) {
  start_server([](UdpSocket &socket, const Message &message) {
    ASSERT_FALSE(socket.write(message));
  });
  UdpSocket socket{};
  std::string data{"hello"};
  Message request{
      .address = server_address_,
      .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}};
  std::error_code error{socket.write(request)};
  ASSERT_FALSE(error) << error.message();
  auto message{socket.read()};
  ASSERT_TRUE(message) << message.error();
  ASSERT_EQ(message, request);
}
