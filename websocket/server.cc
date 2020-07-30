// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 16:49
// Description:  No.

#include "server.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#if defined(__linux__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>  // NOLINT.
#include <vector>

#include "websocket.h"
#include "socket_util.h"


namespace libwebsocket {

namespace {

constexpr int kMaxBufferLength = 4096;

}  // namespace

// 重要参数初始化.
void WebSocketServer::Init(void) {
  callback_ = [] (Socket const&, char const*, int const&) { return; };
  deep_callback_ = [] (Socket const&, char const*, int const&) { return; };
  is_ready_.store(false);
  waiting_is_running_.store(false);
  service_is_running_.store(false);
}

// 创建一个套接字, 并绑定到指定IP和端口上.
int WebSocketServer::InitServer(void) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(server_port_));
#if defined(__linux__)
  addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
  listen_socket_= socket(AF_INET, SOCK_STREAM, 0);
  if (listen_socket_ == -1) {
    printf("%s[%d]: Create socket failed!!!\n", __FUNCTION__, __LINE__);
    return -1;
  }
#elif defined(_WIN32)
  WSADATA ws_data;
  if (WSAStartup(MAKEWORD(2,2), &ws_data) != 0) {
    return -1;
  }
  listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_socket_ == INVALID_SOCKET) {
    printf("%s[%d]: Create socket failed!!!\n", __FUNCTION__, __LINE__);
    WSACleanup();
    return -1;
  }
  addr.sin_addr.S_un.S_addr = inet_addr(server_ip_.c_str());
#endif
  if (Bind(listen_socket_, reinterpret_cast<struct sockaddr *>(&addr),
           sizeof(addr)) == -1) {
    printf("%s[%d]: Connect to remote server failed!!!\n",
           __FUNCTION__, __LINE__);
    Close(listen_socket_);
#if defined(_WIN32)
    WSACleanup();
#endif
    return -1;
  }
  is_ready_.store(true);
  return 0;
}

// 开启等待客户端连接和与客户端通信线程.
bool WebSocketServer::Run(void) {
  if (!is_ready_) return false;
  service_thread_ = std::thread(&WebSocketServer::ServiceHandler, this);
  service_thread_.detach();
  waiting_thread_ = std::thread(&WebSocketServer::WaitHandler, this);
  waiting_thread_.detach();
  return true;
}

// 停止服务线程, 关闭并清空所有已连接的套接字.
void WebSocketServer::Stop(void) {
  if (listen_socket_ > 0) {
    service_is_running_.store(false);
    waiting_is_running_.store(false);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (auto& socket : valid_sockets_) {
      Close(socket);
    }
    valid_sockets_.clear();
    Close(listen_socket_);
    listen_socket_ = 0;
#if defined(_WIN32)
    WSACleanup();
#endif
    is_ready_.store(false);
  }
}

int WebSocketServer::SendToOne(Socket const& socket,
                               char const* buffer, int const& size) {
  for (auto client : valid_sockets_) {
    if (socket == client) {
      return Send(socket, buffer, size, 0);
    }
  }
  return -1;
}

int WebSocketServer::SendToAll(char const* buffer, int const& size) {
  for (auto socket : valid_sockets_) {
    Send(socket, buffer, size, 0);
  }
  return 0;
}

// 等待客户端连接线程处理函数.
// 若有客户端进行连接, 先进行握手操作, 认证成功后
// 将转到主服务线程中进行数据交换.
void WebSocketServer::WaitHandler(void) {
  waiting_is_running_.store(true);
  if (Listen(listen_socket_, 10) < 0) {
    waiting_is_running_.store(false);
    Stop();
    return;
  }
  struct sockaddr_in addr;
  int len = sizeof(addr);
  std::vector<uint8_t> msg;
  std::string request;
  std::string respond;
  int ret = -1;
  int timeout_ms = 3*10;  // 超时时间, 100ms.
  auto tp = std::chrono::steady_clock::now();
  std::unique_ptr<char[]> buffer(
      new char[kMaxBufferLength], std::default_delete<char[]>());
  WebSocket websocket;
  while(waiting_is_running_) {
    // 等待新的连接.
    Socket socket = Accept(listen_socket_,
        reinterpret_cast<struct sockaddr *>(&addr), &len);
    if (socket <= 0) {
      printf("%s[%d]: Invalid socket!!!\n", __FUNCTION__, __LINE__);
      break;
    }
    tp = std::chrono::steady_clock::now();
    while (1) {
      if ((ret = Recv(socket, buffer.get(), kMaxBufferLength, 0)) > 0) {
        msg.assign(buffer.get(), buffer.get()+ret);
        break;
      } else if (ret == 0) {
        printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
        break;
      } else {
#if defined(__linux__)
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
#elif defined(_WIN32)
        auto wsa_errno = WSAGetLastError();
        if (wsa_errno == WSAEINTR || wsa_errno == WSAEWOULDBLOCK) continue;
#endif
        printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
        break;
      }
      // 检测超时退出.
      if (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now()-tp).count() >= timeout_ms) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (msg.empty()) {
      Close(socket);
      continue;
    }
    request.assign(msg.begin(), msg.end());
    if (!websocket.IsHandShake(request)) {
      Close(socket);
      continue;
    }
    websocket.HandShake(request, &respond);
    Send(socket, respond.c_str(), respond.size(), 0);
    // 设置非阻塞模式.
#if defined(__linux__)
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#elif defined(_WIN32)
    unsigned long ul = 1;
    if (ioctlsocket(socket, FIONBIO, (unsigned long *)&ul) == SOCKET_ERROR) {
      printf("%s[%d]: Set socket nonblock failed !!!\n",
             __FUNCTION__, __LINE__);
      Close(socket);
      continue;
    }
#endif
    // 保存已通过认证的socket.
    valid_sockets_.push_back(socket);
  }
  waiting_is_running_.store(false);
  Stop();
}

void WebSocketServer::ServiceHandler(void) {
  service_is_running_.store(true);
  int ret = -1;
  bool alive = false;
  WebSocket websocket;
  std::unique_ptr<char[]> buffer(
    new char[kMaxBufferLength], std::default_delete<char[]>());
  std::vector<char> msg;
  std::vector<char> payload_content;
  while(service_is_running_) {
    for (auto it = valid_sockets_.begin(); it != valid_sockets_.end(); ++it) {
      if ((ret = Recv(*it, buffer.get(), kMaxBufferLength, 0)) > 0) {
        if (!alive) alive = true;
        deep_callback_(*it, buffer.get(), ret);
        payload_content.clear();
        msg.assign(buffer.get(), buffer.get() + ret);
        if (websocket.FormDataParse(msg, &payload_content) > 0) {
          callback_(*it, payload_content.data(), payload_content.size());
        }
        continue;
      } else if (ret <= 0) {
        if (ret < 0) {
#if defined(__linux__)
          if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
          }
#elif defined(_WIN32)
          auto wsa_errno = WSAGetLastError();
          if (wsa_errno == WSAEINTR || wsa_errno == WSAEWOULDBLOCK) continue;
#endif
        }
        printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
        valid_sockets_.erase(it);
        Close(*it);
        if (!alive) alive = true;
        break;  // 删除连接时不再继续遍历, 而是重新开始遍历.
      }
    }
    if (!alive) {
      std::this_thread::sleep_for(std::chrono:: milliseconds(10));
    }
    alive = false;
  }
  service_is_running_.store(false);
  Stop();
}

}  // namespace libwebsocket
