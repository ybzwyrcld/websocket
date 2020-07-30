// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 10:22
// Description:  No.


#include <chrono>  // NOLINT.
#include <thread>  // NOLINT.

#include "websocket/server.h"
#include "websocket/websocket.h"


using libwebsocket::WebSocketServer;
using libwebsocket::WebSocket;

int main(void) {
  WebSocketServer server;
  WebSocket websocket;;
  websocket.set_fin(1);
  websocket.set_opcode(1);
  websocket.set_mask(0);
  std::vector<char> payload_content;
  std::vector<char> send_data;
  server.Init();
  server.SetServerAccessPoint("127.0.0.1", 8081);
  // Set the callback function when receiving data.
  server.OnReceived([&] (const WebSocketServer::Socket& fd,
      char const* buffer, int const& size) -> void {
    payload_content.clear();
    send_data.clear();
    payload_content.assign(buffer, buffer + size);
    // TODO(mengyuming@hotmail.com): Just return the received data at now.
    websocket.FormDataGenerate(payload_content, &send_data);
    send(fd, send_data.data(), send_data.size(), 0);
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
