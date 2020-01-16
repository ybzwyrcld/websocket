// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 16:50
// Description:  No.

#ifndef WEBSOCKET_SERVER_H_
#define WEBSOCKET_SERVER_H_

#include <functional>
#include <string>
#include <vector>


namespace libwebsocket {

using ServerReceiveCallback =
    std::function<void(const int &fd, const char *buffer, const int &size)>;

class Server {
 public:
  Server() : callback_([](const int &fd, const char *, const int &) {}),
        deep_callback_([](const int &fd, const char *, const int &) {}) {}
  ~Server();
  void Init(const std::string &ip, const int &port, const int &max_count);
  bool Run(const int &time_out = 3000);
  void Stop(void);
  void OnReceived(const ServerReceiveCallback &callback) {
    callback_ = callback;
  }
  void OnDeepReceived(const ServerReceiveCallback &callback) {
    deep_callback_ = callback;
  }
  int SendToAll(char *send_buf, const int &size);
  bool is_running(void) const { return is_running_; }

 private:
  int AcceptNewConnect(void);
  int RecvData(const int &sock, char *recv_buf);
  void ThreadHandler(const int &time_out);

  bool is_running_ = false;
  bool is_stop_ = false;
  int listen_fd_ = -1;
  int epoll_fd_ = -1;
  int server_port_;
  int max_count_;
  std::string server_ip_;
  std::vector<int> valid_fds_;
  ServerReceiveCallback deep_callback_;
  ServerReceiveCallback callback_;
};

}  // namespace libwebsocket

#endif  // WEBSOCKET_SERVER_H_
