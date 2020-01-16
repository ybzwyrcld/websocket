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


using libwebsocket::Client;
using libwebsocket::WebSocket;

namespace {

constexpr char str[] = "hello websocket";

}  // namespace

int main(void) {
  Client client;
  WebSocket websocket;
  websocket.set_mask(1);
  client.Init("127.0.0.1", 8081);
  client.OnReceived([](const int &fd, const char *buffer, const int &size) {
    printf("Recv[%d]: %s\n", size, buffer);
  });
  if (client.Run() != true) return -1;
  std::vector<char> msg1;
  std::vector<char> msg2;
  std::vector<char> msgout;
  msg1.assign(str, str + 6);
  msg2.assign(str + 6, str + sizeof(str)-1);
  while (1) {
    websocket.set_fin(0);
    websocket.set_opcode(0);
    websocket.FormDataGenerate(msg1, &msgout);
    client.SendRawData(msgout);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    websocket.set_fin(1);
    websocket.FormDataGenerate(msg2, &msgout);
    client.SendRawData(msgout);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
