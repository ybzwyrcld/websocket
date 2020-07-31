// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-16 09:46
// Description:  No.

#include <stdio.h>
#include <unistd.h>

#include <chrono>  // NOLINT.
#include <thread>  // NOLINT.

#include "websocket/client.h"
#include "websocket/websocket.h"


using libwebsocket::WebSocketClient;
using libwebsocket::WebSocketMsg;

namespace {

constexpr char str[] = "hello websocket";

}  // namespace

int main(void) {
  WebSocketClient client;
  WebSocketMsg websocket_msg {};
  websocket_msg.msg_head.bit.fin = 1;
  websocket_msg.msg_head.bit.opcode = libwebsocket::kOPCodeBinary;
  websocket_msg.msg_head.bit.mask = 1;
  client.Init();
  client.SetRemoteAccessPoint("127.0.0.1", 8081);
  client.OnReceived([](WebSocketClient::Socket const& fd,
      char const* buffer, int const& size) -> void {
    printf("Recv[%d]: %s\n", size, buffer);
  });
  if ((client.ConnectRemote() == 0) &&
      client.Run()) {
    std::vector<char> msg1;
    std::vector<char> msg2;
    std::vector<char> msgout;
    msg1.assign(str, str + 6);
    msg2.assign(str + 6, str + sizeof(str)-1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while (client.service_is_running()) {
      websocket_msg.msg_head.bit.fin = 0;
      if (libwebsocket::WebSocketFramePackaging(websocket_msg, &msgout) == 0) {
        client.SendRawData(msgout);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      websocket_msg.msg_head.bit.fin = 1;
      if (libwebsocket::WebSocketFramePackaging(websocket_msg, &msgout) == 0) {
        client.SendRawData(msgout);
      }
    }
    client.Stop();
  }
  return 0;
}
