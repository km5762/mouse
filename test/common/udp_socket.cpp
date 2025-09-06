#include "udp_socket.hpp"
#include "asserts.hpp"
#include <gtest/gtest.h>
#include <netdb.h>
#include <sys/socket.h>
#include <system_error>

constexpr uint16_t client_port{61137};

class UdpSocketTest : public testing::Test {
protected:
  UdpSocket socket_;
  AddressResolver resolver_{};

  void start_server(const std::function<void(const Message &)> &handler) {
    server_ = std::thread{[this, handler] {
      auto addresses{resolver_.resolve({.service = "6789",
                                        .flags = AI_PASSIVE,
                                        .family = AF_INET,
                                        .type = SOCK_DGRAM})};
      EXPECT_TRUE(addresses) << addresses.error().message();
      std::error_code error{socket_.bind(*addresses)};
      EXPECT_FALSE(error) << error.message();

      {
        std::lock_guard lock{mtx_};
        server_ready_ = true;
        cv_.notify_all();
      }

      while (true) {
        auto message{socket_.read()};
        EXPECT_TRUE(message) << message.error().message();
        if (std::string_view(
                reinterpret_cast<const char *>(message->data.data()),
                message->data.size()) == "fin") {
          std::string ack{"ack"};
          std::error_code error{socket_.write(
              {.address = message->address,
               .data = {reinterpret_cast<const std::byte *>(ack.data()),
                        ack.size()}})};
          break;
        }
        handler(*message);
      }
    }};

    {
      std::unique_lock lock{mtx_};
      cv_.wait(lock, [&] { return server_ready_; });
    }
  }

  void TearDown() override {
    UdpSocket socket{};
    std::string message{"fin"};
    std::error_code error = socket.write(
        {.address = *socket_.address(),
         .data = {reinterpret_cast<const std::byte *>(message.data()),
                  message.size()}});
    EXPECT_FALSE(error) << error.message();
    auto ack{socket.read()};
    EXPECT_TRUE(ack) << ack.error().message();
    if (server_.joinable()) {
      server_.join();
    }
  }

private:
  const uint16_t server_port_{7357};
  std::thread server_;
  std::condition_variable cv_;
  bool server_ready_{false};
  std::mutex mtx_;
};

TEST(UdpSocket, Bind) {
  UdpSocket socket{};
  AddressResolver resolver{};

  auto addresses{resolver.resolve({.service = "12345"})};
  ASSERT_TRUE(addresses) << addresses.error().message();
  std::error_code error{socket.bind(*addresses)};
  ASSERT_FALSE(error) << error.message();
}

TEST(UdpSocket, DoubleBind) {
  UdpSocket socket{};
  AddressResolver resolver{};

  auto addresses{resolver.resolve({.service = "12345"})};
  ASSERT_TRUE(addresses) << addresses.error().message();
  std::error_code error{socket.bind(*addresses)};
  ASSERT_FALSE(error) << error.message();
  error = socket.bind(*addresses);
  ASSERT_TRUE(error);
}

TEST_F(UdpSocketTest, EphemeralWrite) {
  start_server([](const Message &message) {});
  UdpSocket socket{};
  std::string data{"hello"};
  std::error_code error{socket.write(
      {.address = *socket_.address(),
       .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}})};
  ASSERT_FALSE(error) << error.message();
}

TEST_F(UdpSocketTest, WriteInvalidAddress) {
  start_server([](const Message &message) {});
  UdpSocket socket{};
  std::string data{"hello"};
  std::error_code error{socket.write(
      {.address = {},
       .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}})};
  ASSERT_TRUE(error);
}

TEST_F(UdpSocketTest, DoubleEphemeralWrite) {
  start_server([](const Message &message) {});
  UdpSocket socket{};
  std::string data{"hello"};
  std::error_code error{socket.write(
      {.address = *socket_.address(),
       .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}})};
  ASSERT_FALSE(error) << error.message();
  error = socket.write(
      {.address = *socket_.address(),
       .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}});
  ASSERT_FALSE(error) << error.message();
}

TEST_F(UdpSocketTest, Read) {
  start_server(
      [&](const Message &message) { EXPECT_FALSE(socket_.write(message)); });
  UdpSocket socket{};
  std::string data{"hello"};
  Message request{
      .address = *socket_.address(),
      .data = {reinterpret_cast<std::byte *>(data.data()), data.size()}};
  std::error_code error{socket.write(request)};
  ASSERT_FALSE(error) << error.message();
  auto message{socket.read()};
  ASSERT_TRUE(message) << message.error();
  ASSERT_EQ(
      std::string_view(reinterpret_cast<const char *>(message->data.data()),
                       message->data.size()),
      data);
}
