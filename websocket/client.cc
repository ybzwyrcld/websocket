// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-09 10:40
// Description:  No.

#include "client.h"

#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif

#include <chrono>

#include "socket_util.h"

#include <memory>
#include <thread>  // NOLINT.
#include <vector>

#include "websocket.h"
#include "base64.h"
#include "sha1.h"


namespace libwebsocket {

namespace {

constexpr int kMaxBufferLength = 4096;

// 生成一条随机字符串.
std::string GetRandomString(int const& length) {
  std::string result;
  for (int i = 0; i < length; i++) {
    srand(time(NULL));
    int flag = rand() % 3;
    switch (flag) {
      case 0:
        result.push_back(static_cast<char>('A' + rand() % 26));
        break;
      case 1:
        result.push_back(static_cast<char>('a' + rand() % 26));
        break;
      case 2:
        result.push_back(static_cast<char>('0' + rand() % 10));
        break;
      default:
        result.push_back('x');
        break;
    }
  }
  return result;
}

}  // namespace

// 设置默认回调函数.
void WebSocketClient::Init(void) {
  callback_ = [] (Socket const&, char const*, int const&) { return; };
  deep_callback_ = [] (Socket const&, char const*, int const&) { return; };
  is_connected_.store(false);
  service_is_running_.store(false);
}

// 与远程服务器建立TCP连接, 并设置socket为非阻塞模式.
int WebSocketClient::ConnectRemote(void) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(server_port_));
#if defined(__linux__)
  addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
  socket_= socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == -1) {
    printf("%s[%d]: Create socket failed!!!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  struct timeval timeout = {0, 500000};
  setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#elif defined(_WIN32)
  WSADATA ws_data;
  if (WSAStartup(MAKEWORD(2,2), &ws_data) != 0) {
    return -1;
  }
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == INVALID_SOCKET) {
    printf("%s[%d]: Create socket failed!!!\n", __FUNCTION__, __LINE__);
    WSACleanup();
    return -1;
  }
  addr.sin_addr.S_un.S_addr = inet_addr(server_ip_.c_str());
#endif
  if (Connect(socket_, reinterpret_cast<struct sockaddr *>(&addr),
              sizeof(addr)) == -1) {
    printf("%s[%d]: Connect to remote server failed!!!\n",
           __FUNCTION__, __LINE__);
    Close(socket_);
#if defined(_WIN32)
    WSACleanup();
#endif
    return -1;
  }
  // 设置非阻塞模式.
#if defined(__linux__)
  int flags = fcntl(socket_, F_GETFL, 0);
  fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
#elif defined(_WIN32)
  unsigned long ul = 1;
  if (ioctlsocket(socket_, FIONBIO, (unsigned long *)&ul) == SOCKET_ERROR) {
    printf("%s[%d]: Set socket nonblock failed!!!\n", __FUNCTION__, __LINE__);
    return -1;
  }
#endif
  is_connected_.store(true);
  return 0;
}

// 开启与客户端进行握手, 握手成功后转到服务线程处理数据.
bool WebSocketClient::Run(void) {
  if (!is_connected_) return false;
  int ret;
  std::string key = GetRandomString(20);
  std::vector<char> keytemp(key.begin(), key.end());
  Base64Encode(keytemp, &key);
  // Get authentication key.
  std::string authen_key = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA1 sha;
  uint32_t message_digest[5];
  sha.Reset();
  sha << authen_key.c_str();
  sha.Result(message_digest);
  for (int i = 0; i < 5; i++) message_digest[i] = htonl(message_digest[i]);
  keytemp.assign(reinterpret_cast<char*>(message_digest),
                 reinterpret_cast<char*>(message_digest)+20);
  Base64Encode(keytemp, &authen_key);
  // Generate request data.
  std::string request =
      "GET / HTTP/1.1\r\n"
      "Connection: Upgrade\r\n"
      "Host: " + server_ip_ + ":" + std::to_string(server_port_) + "\r\n"
      "Origin: null\r\n"
      "Sec-WebSocket-Extensions: x-webkit-deflate-frame\r\n"
      "Sec-WebSocket-Key: " + key + "\r\n"
      "Sec-WebSocket-Version: 13\r\n"
      "Upgrade: websocket\r\n";
  if (Send(socket_, request.c_str(), request.size(), 0) <= 0) {
    printf("%s[%d]: Send request data failed !!!\n", __FUNCTION__, __LINE__);
    Close(socket_);
    return false;
  }
  // Waitting for respond.
  int timeout = 3;
  std::unique_ptr<char[]> buffer(new char[kMaxBufferLength],
                                 std::default_delete<char[]>());

  while (timeout--) {
    ret = Recv(socket_, buffer.get(), kMaxBufferLength, 0);
    if (ret > 0) {
      std::string respond = buffer.get();
      if ((respond.find("HTTP/1.1 101") == std::string::npos) ||
          (respond.find(authen_key) == std::string::npos)) {
        printf("%s[%d]: Authentication failed !!!\n", __FUNCTION__, __LINE__);
        Close(socket_);
        return false;
      }
      break;
    } else if (ret == 0) {
      printf("%s[%d]: Remote socket close !!!\n", __FUNCTION__, __LINE__);
      Close(socket_);
      return false;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  if (timeout < 0) {
    Close(socket_);
    return false;
  }
  service_thread_ = std::thread(&WebSocketClient::ThreadHandler, this);
  service_thread_.detach();
  return true;
}

// 停止服务线程, 关闭并清空已连接的套接字.
void WebSocketClient::Stop(void) {
  if (service_is_running_) {
    service_is_running_.store(false);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  if (socket_ > 0) {
    Close(socket_);
  }
  is_connected_.store(false);
}

// 发送原始数据.
int WebSocketClient::SendRawData(char const* buffer, int const& size) {
  return Send(socket_, buffer, size, 0);
}

// 将原始数据封装后再进行发送.
int WebSocketClient::SendData(char const* buffer, int const& size) {
  WebSocket websocket;
  websocket.set_fin(1);
  websocket.set_mask(1);
  websocket.set_opcode(0x1);
  std::vector<char> msgin, msgout;
  msgin.assign(buffer, buffer + size);
  websocket.FormDataGenerate(msgin, &msgout);
  return SendRawData(msgout);
}

// 接收服务端发过来的数据, 调用回调函数进行外部处理.
void WebSocketClient::ThreadHandler(void) {
  service_is_running_.store(true);
  int ret;
  std::unique_ptr<char[]> buffer(new char[kMaxBufferLength],
                                   std::default_delete<char[]>());
  WebSocket websocket;
  std::vector<char> recv_data, payload_content;
  while (service_is_running_) {
    ret = Recv(socket_, buffer.get(), kMaxBufferLength, 0);
    if (ret > 0) {
      deep_callback_(socket_, buffer.get(), ret);
      recv_data.assign(buffer.get(), buffer.get() + ret);
      // 解析出实际消息内容.
      if (websocket.FormDataParse(recv_data, &payload_content) > 0) {
        callback_(socket_, payload_content.data(),
                  payload_content.size());
      }
    } else if (ret <= 0) {
      if (ret < 0) {  // 排除正常错误返回码.
#if defined(__linux__)
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
#elif defined(_WIN32)
        auto wsa_errno = WSAGetLastError();
        if (wsa_errno == WSAEINTR || wsa_errno == WSAEWOULDBLOCK) continue;
#endif
      }
      printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
      Close(socket_);
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  if (socket_ > 0) {
    Close(socket_);
    socket_ = -1;
  }
  service_is_running_.store(false);
  Stop();
}

}  // namespace libwebsocket
