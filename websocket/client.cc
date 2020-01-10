// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-09 10:40
// Description:  No.

#include "client.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <memory>
#include <thread>  // NOLINT.
#include <vector>

#include "websocket.h"
#include "base64.h"
#include "sha1.h"


namespace libwebsocket {

namespace {

constexpr int kMaxBufferLength = 65536;

int EpollRegister(const int &epoll_fd, const int &fd) {
  struct epoll_event ev;
  int ret, flags;
  // Important: make the fd non-blocking.
  flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  ev.events = EPOLLIN;
  // ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = fd;
  do {
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
  } while (ret < 0 && errno == EINTR);
  return ret;
}

int EpollUnregister(const int &epoll_fd, const int &fd) {
  int ret;
  do {
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
  } while (ret < 0 && errno == EINTR);

  return ret;
}

std::string GetRandomString(const int &length) {
  int flag, i;
  std::string result;
  auto speed = static_cast<uint32_t>(time(NULL));
  for (i = 0; i < length - 1; i++) {
    flag = rand_r(&speed) % 3;
    switch (flag) {
      case 0:
        result.push_back(static_cast<char>('A' + rand_r(&speed) % 26));
        break;
      case 1:
        result.push_back(static_cast<char>('a' + rand_r(&speed) % 26));
        break;
      case 2:
        result.push_back(static_cast<char>('0' + rand_r(&speed) % 10));
        break;
      default:
        result.push_back('x');
        break;
    }
  }
  return result;
}

}  // namespace

Client::~Client() {
  if (socket_fd_ > 0) {
    close(socket_fd_);
  }
  if (epoll_fd_ > 0) {
    close(epoll_fd_);
  }
}

void Client::Init(const std::string &ip, const int &port) {
  server_ip_ = ip;
  server_port_ = port;
}

bool Client::Run(const int &time_out) {
  int ret;
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("Create socket fail\n");
    return false;
  }
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port_);
  server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
  if (connect(socket_fd, reinterpret_cast<struct sockaddr *>(&server_addr),
              sizeof(struct sockaddr_in)) < 0) {
    printf("Connect to server failed!!!\n");
    return false;
  }
  // Set socket non-blocking.
  int flags = fcntl(socket_fd, F_GETFL);
  fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
  std::string key = GetRandomString(20);
  key = base64_encode(
      reinterpret_cast<const uint8_t *>(key.c_str()), key.size());
  // Get authentication key.
  std::string authen_key = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA1 sha;
  uint32_t message_digest[5];
  sha.Reset();
  sha << authen_key.c_str();
  sha.Result(message_digest);
  for (int i = 0; i < 5; i++) {
    message_digest[i] = htonl(message_digest[i]);
  }
  authen_key = base64_encode(
      reinterpret_cast<const uint8_t *>(message_digest), 20);
  // Generate request data.
  std::string request =
      "GET / HTTP/1.1\r\n"
      "Connection: Upgrade\r\n"
      "Host: " + server_ip_ + ":" + std::to_string(server_port_) + "\r\n"
      "Origin: null\r\n"
      "Sec-WebSocket-Extensions: x-webkit-deflate-frame\r\n"
      "Sec-WebSocket-Key: " + key + "\r\n"
      "Sec-WebSocket-Version: 13\r\n"
      "Upgrade: websocket\r\n";
  // printf("%s\n", request.c_str());
  if (send(socket_fd, request.c_str(), request.size(), 0) !=
      static_cast<int>(request.size())) {
    printf("Send failed!!!\n");
    close(socket_fd);
    return false;
  }
  // Waitting for respond.
  int timeout = 3;
  std::unique_ptr<char[]> buffer(new char[kMaxBufferLength],
                                 std::default_delete<char[]>());
  while (timeout--) {
    ret = recv(socket_fd, buffer.get(), kMaxBufferLength, 0);
    if (ret > 0) {
      std::string respond = buffer.get();
      if ((respond.find("HTTP/1.1 101") == std::string::npos) ||
          (respond.find(authen_key) == std::string::npos)) {
        printf("Authentication failed!!!\n");
        close(socket_fd);
        return false;
      }
      timeout = 3;
      break;
    } else if (ret == 0) {
      printf("Remote socket close!!!\n");
      close(socket_fd);
      return false;
    }
    sleep(1);
  }
  if (timeout <= 0) return false;
  // TCP socket keepalive.
  int keepalive = 1;     // Enable keepalive attributes.
  int keepidle = 30;     // Time out for starting detection.
  int keepinterval = 5;  // Time interval for sending packets during detection.
  int keepcount = 3;     // Max times for sending packets during detection.
  setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive,
             sizeof(keepalive));
  setsockopt(socket_fd, SOL_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
  setsockopt(socket_fd, SOL_TCP, TCP_KEEPINTVL, &keepinterval,
             sizeof(keepinterval));
  setsockopt(socket_fd, SOL_TCP, TCP_KEEPCNT, &keepcount, sizeof(keepcount));
  socket_fd_ = socket_fd;
  // Listen.
  epoll_fd_ = epoll_create(1);
  EpollRegister(epoll_fd_, socket_fd_);
  is_running_ = true;
  is_stop_ = false;
  std::thread thread = std::thread(&Client::ThreadHandler, this, time_out);
  thread.detach();
  printf("Service is running!\n");
  return true;
}

void Client::Stop(void) {
  is_running_ = false;
  while (!is_stop_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

int Client::SendData(const char *buffer, const int &size) {
  WebSocket websocket;
  websocket.set_fin(1);
  websocket.set_mask(1);
  websocket.set_opcode(0x1);
  websocket.set_payload_length(size);
  std::vector<char> msgin, msgout;
  msgin.assign(buffer, buffer + size);
  websocket.FormDataGenerate(msgin, &msgout);
  return send(socket_fd_, msgout.data(), msgout.size(), 0);
}

int Client::RecvData(const int &sock, char *recv_buf) {
  char buf[1024];
  int len = 0;
  int ret = 0;
  while (ret >= 0) {
    ret = recv(sock, buf, sizeof(buf), 0);
    if (ret <= 0) {
      EpollUnregister(epoll_fd_, sock);
      close(sock);
      break;
    } else if (ret < 1024) {
      memcpy(recv_buf, buf, ret);
      len += ret;
      break;
    } else {
      memcpy(recv_buf, buf, sizeof(buf));
      len += ret;
    }
  }
  return (ret <= 0 ? ret : len);
}

void Client::ThreadHandler(const int &time_out) {
  int ret;
  std::unique_ptr<struct epoll_event> epoll_event(new struct epoll_event);
  std::unique_ptr<char[]> recv_buf(new char[kMaxBufferLength],
                                   std::default_delete<char[]>());
  WebSocket websocket;
  std::vector<char> recv_data, payload_content;
  while (is_running_) {
    ret = epoll_wait(epoll_fd_, epoll_event.get(), 1, time_out);
    if (ret == 0) {
      printf("Time out\n");
      continue;
    } else if (ret == -1) {
      printf("Error\n");
    } else {
      if (epoll_event->events & EPOLLIN) {
        ret = RecvData(socket_fd_, recv_buf.get());
        if (ret > 0) {
          recv_data.clear();
          payload_content.clear();
          recv_data.assign(recv_buf.get(), recv_buf.get() + ret);
          websocket.FormDataParse(recv_data, &payload_content);
          callback_(socket_fd_, payload_content.data(), payload_content.size());
        } else if (ret == 0) {
          printf("Disconnect\n");
          EpollUnregister(epoll_fd_, socket_fd_);
          close(socket_fd_);
          break;
        }
      }
    }
  }
  if (socket_fd_ > 0) {
    close(socket_fd_);
    socket_fd_ = -1;
  }
  if (epoll_fd_ > 0) {
    close(epoll_fd_);
    epoll_fd_ = -1;
  }
  is_stop_ = true;
}

}  // namespace libwebsocket
