// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-09 10:40
// Description:  No.

#ifndef WEBSOCKET_CLIENT_H_
#define WEBSOCKET_CLIENT_H_

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

// Websocket客户端.
//
// Example:
//     WebSocketClient client;
//     client.Init();
//     client.SetRemoteAccessPoint("127.0.0.1", 8081);
//     // 输出接收到的消息解析内容.
//     client.OnReceived([] (WebSocketClient::Socket const& fd,
//         const char *buffer, const int &size) -> void {
//     printf("Recv[%d]: %s\n", size, buffer);
//   });
//   if ((client.ConnectRemote() == 0 ) &&
//       client.Run()) {
//     std::vector<char> msg;
//     msg.assign(str, str + sizeof(str) -1);
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     // 定时发送消息.
//     while (client.service_is_running()) {
//       client.SendData(msg);
//       std::this_thread::sleep_for(std::chrono::seconds(1));
//     }
//   }
class WebSocketClient {
 public:
  WebSocketClient() {}
  ~WebSocketClient() { Stop(); }

  // 重要参数初始化.
  void Init(void);
  // 设置远程服务器地址.
  void SetRemoteAccessPoint(std::string const& ip, int const& port) {
    server_ip_ = ip;
    server_port_ = port;
  }
  // 连接到远程服务器.
  int ConnectRemote(void);

  // 启动服务线程.
  bool Run(void);
  // 停止服务线程.
  void Stop(void);
  // 获取当前服务线程运行状态.
  bool service_is_running(void) const {
    return service_is_running_;
  }

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

  // 直接发送原始数据.
  int SendRawData(char const* buffer, int const& size);
  int SendRawData(std::vector<char> const& msg) {
    return SendRawData(msg.data(), msg.size());
  }
  // 封装并发送协议格式数据.
  int SendData(char const*buffer, int const& size);
  int SendData(std::vector<char> const& msg) {
    return SendData(msg.data(), msg.size());
  }

 private:
  // 服务线程处理函数.
  void ThreadHandler(void);

  Socket socket_;  // 通用TCP连接socket.
  std::atomic_bool is_connected_;  // 与服务端TCP连接状态.
  std::string server_ip_;  // 服务端IP地址.
  int server_port_;  // 服务端端口.
  std::thread service_thread_;  // 服务线程.
  std::atomic_bool service_is_running_;  // 服务线程运行标志.
  ReceiveCallback deep_callback_;  // 原始消息回调函数.
  ReceiveCallback callback_;  // 解析后的消息回调函数.
};

}  // namespace libwebsocket

#endif  // WEBSOCKET_CLIENT_H_
