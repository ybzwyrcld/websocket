// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-09 10:40
// Description:  No.

#ifndef WEBSOCKET_CLIENT_H_
#define WEBSOCKET_CLIENT_H_

#include <functional>
#include <string>


namespace libwebsocket {

using ClientReceiveCallback =
    std::function<void(const int &fd, const char *buffer, const int &size)>;
class Client {
 public:
  Client() {}
  ~Client() {}
  void Init(const std::string &ip, const int &port);
  bool Run(const int &time_out = 3000);
  void Disconnect(void);
  void OnReceived(const ClientReceiveCallback &callback) {
    callback_ = callback;
  }
  int SendData(const char *buffer, const int &size);
  int SendData(const std::string &msg) {
    return SendData(msg.c_str(), msg.size());
  }

 private:
  int RecvData(const int &sock, char *recv_buf);
  void ThreadHandler(const int &time_out);

  int epoll_fd_;
  int socket_fd_;
  int server_port_;
  std::string server_ip_;
  ClientReceiveCallback callback_;
};

}  // namespace libwebsocket

#endif  // WEBSOCKET_CLIENT_H_
