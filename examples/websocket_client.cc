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


using libwebsocket::Client;

int main(void) {
  Client client;
  client.Init("127.0.0.1", 8081);
  client.OnReceived([](const int &fd, const char *buffer, const int &size) {
      printf("Recv[%d]: %s\n", size, buffer);
      });
  if (client.Run() != true) return -1;
  std::string msg = "hello websocket";
  while (1) {
    client.SendData(msg);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
