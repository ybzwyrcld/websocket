// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 16:49
// Description:  No.

#include "server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <thread>  // NOLINT.
#include <vector>

#include "websocket.h"


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

}  // namespace

Server::Server() {}
Server::~Server() {}

void Server::Init(const std::string &ip, const int &port,
                  const int &max_count) {
  server_ip_ = ip;
  server_port_ = port;
  max_count_ = max_count;
}

bool Server::Run(const int &time_out) {
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ == -1) {
    printf("Create socket failed!!!");
    return false;
  }
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(sockaddr_in));
  server_addr.sin_family = AF_INET;
  if (server_ip_.empty()) {
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
  }
  server_addr.sin_port = htons(server_port_);
  if (bind(listen_fd_, reinterpret_cast<struct sockaddr *>(&server_addr),
           sizeof(server_addr)) < 0) {
    printf("Bind socket failed!!!");
    return false;
  }
  if (listen(listen_fd_, 5) < 0) {
    printf("Listen socket Failed!!!");
    return false;
  }
  epoll_fd_ = epoll_create(max_count_);
  EpollRegister(epoll_fd_, listen_fd_);
  std::thread thread = std::thread(&Server::ThreadHandler, this, time_out);
  thread.detach();
  printf("Service is running!\n");
  return true;
}

int Server::SendToAll(char *send_buf, const int &size) {
  for (auto fd : valid_fds_) {
    send(fd, send_buf, size, 0);
  }
  return 0;
}

int Server::AcceptNewConnect(void) {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t clilen = sizeof(struct sockaddr);
  int sock = accept(
      listen_fd_, reinterpret_cast<struct sockaddr *>(&client_addr), &clilen);
  // TCP socket keepalive.
  int alive = 1;     // Enable keepalive attributes.
  int idle = 30;     // Time out for starting detection.
  int interval = 5;  // Time interval for sending packets during detection.
  int count = 3;     // Max times for sending packets during detection.
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof(alive));
  setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
  setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
  setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));
  EpollRegister(epoll_fd_, sock);
  return sock;
}

int Server::RecvData(const int &sock, char *recv_buf) {
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

void Server::ThreadHandler(const int &time_out) {
  int ret;
  int alive_count;
  // struct epoll_event *epoll_events = new struct epoll_event[max_count_];
  std::unique_ptr<struct epoll_event[]> epoll_events(
      new struct epoll_event[max_count_],
      std::default_delete<struct epoll_event[]>());
  std::unique_ptr<char[]> send_buf(new char[kMaxBufferLength],
                                   std::default_delete<char[]>());
  std::unique_ptr<char[]> recv_buf(new char[kMaxBufferLength],
                                   std::default_delete<char[]>());
  WebSocket websocket;
  std::string request, respond;
  while (1) {
    ret = epoll_wait(epoll_fd_, epoll_events.get(), max_count_, time_out);
    if (ret == 0) {
      printf("Time out\n");
      continue;
    } else if (ret == -1) {
      printf("Error\n");
    } else {
      alive_count = ret;
      for (int i = 0; i < alive_count; ++i) {
        if (epoll_events[i].data.fd == listen_fd_) {  // New connection.
          if (epoll_events[i].events & EPOLLIN) {
            AcceptNewConnect();
          }
        } else {
          int fd = epoll_events[i].data.fd;
          if (epoll_events[i].events & EPOLLIN) {
            int ret = RecvData(fd, recv_buf.get());
            if (ret > 0) {
              // First handshake.
              if (std::count(valid_fds_.begin(), valid_fds_.end(), fd) == 0) {
                request = recv_buf.get();
                websocket.HandShake(request, &respond);
                send(fd, respond.c_str(), respond.size(), 0);
                valid_fds_.push_back(fd);
              } else {
                callback_(fd, recv_buf.get(), ret);
              }
            } else if (ret == 0) {
              printf("Disconnect\n");
              EpollUnregister(epoll_fd_, fd);
              valid_fds_.erase(
                  std::find(valid_fds_.begin(), valid_fds_.end(), fd));
              close(fd);
              // DealDisconnect(epoll_events_[i].data.fd);
            }
          }
        }
      }
    }
  }
}

}  // namespace libwebsocket
