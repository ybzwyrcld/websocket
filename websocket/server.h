// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 16:50
// Description:  No.

#ifndef WEBSOCKET_SERVER_H_
#define WEBSOCKET_SERVER_H_

#if defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include <atomic>
#include <functional>
#include <string>
#include <vector>
#include <thread>


namespace libwebsocket {

// WebSocket服务端.
//
// Example:
//    WebSocketServer server;
//    WebSocket websocket;;
//    websocket.set_fin(1);
//    websocket.set_opcode(1);
//    websocket.set_mask(0);
//    std::vector<char> payload_content;
//    std::vector<char> send_data;
//    server.Init();
//    server.SetServerAccessPoint("127.0.0.1", 8081);
//    // 设置接收到解析后数据的回调函数, 这里只是将接收到的数据返回.
//    server.OnReceived([&] (const WebSocketServer::Socket& fd,
//        const char *buffer, const int &size) {
//      payload_content.clear();
//      send_data.clear();
//      payload_content.assign(buffer, buffer + size);
//      websocket.FormDataGenerate(payload_content, &send_data);
//      send(fd, send_data.data(), send_data.size(), 0);
//    });
//    if ((server.InitServer() == 0) &&
//        server.Run()) {
//      std::this_thread::sleep_for(std::chrono::seconds(1));
//      while (server.service_is_running()) {
//        std::this_thread::sleep_for(std::chrono::seconds(5));
//       }
//       server.Stop();
//     }
class WebSocketServer {
 public:
  WebSocketServer() {}
  ~WebSocketServer() { Stop(); }

  // 部分成员变量初始化.
  void Init(void);
  // 设置服务端地址.
  void SetServerAccessPoint(std::string const& ip, int const& port) {
    server_ip_ = ip;
    server_port_ = port;
  }
  // 初始化服务端.
  int InitServer(void);

  // 启动服务线程.
  bool Run(void);
  // 停止服务线程.
  void Stop(void);
  // 后台服务线程运行状态.
  bool service_is_running(void) const { return service_is_running_; }

  // 通用套接字类型定义.
  using Socket = decltype(socket(0, 0, 0));
  // 接收到数据时的回调函数定义.
  using ReceiveCallback = std::function<
      void (Socket const& fd, char const* buffer, int const& size)>;
  // 设置接收数据回调函数, 数据为解析后的消息内容.
  void OnReceived(ReceiveCallback const& callback) {
    callback_ = callback;
  }
  // 设置接收数据回调函数, 数据为未解析的消息内容.
  void OnDeepReceived(ReceiveCallback const &callback) {
    deep_callback_ = callback;
  }
  // 发送消息给指定的处于已连接状态的客户端.
  int SendToOne(Socket const& socket, char const* buffer, int const& size);
  // 发送消息给所有处于已连接状态的客户端.
  int SendToAll(char const* buffer, int const& size);

 private:
  // 等待客户端连接线程处理函数.
  void WaitHandler(void);
  // 主服务线程处理函数.
  void ServiceHandler(void);

  Socket listen_socket_;  // 监听客户端连接的套接字.
  std::string server_ip_;  // 服务端IP地址.
  int server_port_;  // 服务端端口.
  std::atomic_bool is_ready_;  // 服务端socket状态.
  std::vector<Socket> valid_sockets_;  // 已认证socket连接.
  std::thread waiting_thread_;  // 等待客户端连接线程.
  std::atomic_bool waiting_is_running_;  // 等待客户端连接线程运行标志.
  std::thread service_thread_;  // 主服务线程.
  std::atomic_bool service_is_running_;  // 主服务线程运行标志.
  ReceiveCallback deep_callback_;  // 原始消息回调函数.
  ReceiveCallback callback_;  // 解析后的消息回调函数.
};

}  // namespace libwebsocket

#endif  // WEBSOCKET_SERVER_H_
