// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 10:25
// Description:  No.

#include <stdio.h>
#include <unistd.h>

#include <chrono>  // NOLINT.
#include <thread>  // NOLINT.

#include "websocket/client.h"


using libwebsocket::WebSocketClient;


namespace {

constexpr char str[] = "hello websocket";

}  // namespace

int main(void) {
  WebSocketClient client;
  client.Init();
  client.SetRemoteAccessPoint("127.0.0.1", 8081);
  client.OnReceived([] (WebSocketClient::Socket const& fd,
      char const* buffer, int const& size) -> void {
    printf("Recv[%d]: %s\n", size, buffer);
  });
  if ((client.ConnectRemote() == 0) &&
      client.Run()) {
    std::vector<char> msg;
    msg.assign(str, str + sizeof(str) -1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while (client.service_is_running()) {
      client.SendData(msg);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    client.Stop();
  }
  return 0;
}
