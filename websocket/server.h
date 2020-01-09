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
  Server();
  ~Server();
  void Init(const std::string &ip, const int &port, const int &max_count);
  bool Run(const int &time_out = 3000);
  void OnReceived(const ServerReceiveCallback &callback) {
    callback_ = callback;
  }
  int SendToAll(char *send_buf, const int &size);

 private:
  int AcceptNewConnect(void);
  int RecvData(const int &sock, char *recv_buf);
  void ThreadHandler(const int &time_out);

  int listen_fd_;
  int epoll_fd_;
  int server_port_;
  int max_count_;
  std::string server_ip_;
  std::vector<int> valid_fds_;
  ServerReceiveCallback callback_;
};

}  // namespace libwebsocket

#endif  // WEBSOCKET_SERVER_H_
