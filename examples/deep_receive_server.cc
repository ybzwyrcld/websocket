// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-16 10:04
// Description:  No.

#include <chrono>  // NOLINT.
#include <thread>  // NOLINT.

#include "websocket/server.h"
#include "websocket/websocket.h"


using libwebsocket::WebSocketServer;
using libwebsocket::WebSocketMsg;

int main(void) {
  WebSocketServer server;
  WebSocketMsg websocket_msg {};
  websocket_msg.msg_head.bit.fin = 1;
  websocket_msg.msg_head.bit.opcode = libwebsocket::kOPCodeText;
  websocket_msg.msg_head.bit.mask = 0;
  std::vector<char> recv_data;
  std::vector<char> payload_content;
  std::vector<char> send_data;
  server.Init();
  server.SetServerAccessPoint("127.0.0.1", 8081);
  // Set the callback function when receiving data.
  server.OnDeepReceived([&] (WebSocketServer::Socket const& socket,
      char const* buffer, int const& size) -> void {
    recv_data.assign(buffer, buffer + size);
    // Handling split packet manually.
    if (libwebsocket::WebSocketFrameParse(recv_data, &websocket_msg) == 0) {
      // TODO(mengyuming@hotmail.com): Just return the received data at now.
      websocket_msg.msg_head.bit.fin = 1;
      websocket_msg.msg_head.bit.opcode = libwebsocket::kOPCodeText;
      websocket_msg.msg_head.bit.mask = 0;
      if (libwebsocket::WebSocketFramePackaging(websocket_msg, &send_data) == 0) {
        server.SendToOne(socket, send_data.data(), send_data.size());
      }
    }
  });
  if ((server.InitServer() == 0) &&
      server.Run()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while (server.service_is_running()) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    server.Stop();
  }
  return 0;
}
