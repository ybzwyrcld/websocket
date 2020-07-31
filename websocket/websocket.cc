// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 09:56
// Description:  No.

#include "websocket.h"

#if defined(__linux__)
#include <arpa/inet.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include <assert.h>
#include <time.h>

#include <map>
#include <vector>

#include "base64.h"
#include "sha1.h"

namespace libwebsocket {

namespace {

constexpr char kMaskKey[] = {'f', 'u', 'c', 'k'};

union U16Converter {
  uint16_t value;
  uint8_t array[2];
};

union U64Converter {
  uint64_t value;
  uint8_t array[8];
};

int StringSplit(std::string const& str, std::string const& div,
                std::vector<std::string>* out) {
  if (out == nullptr) return -1;
  out->clear();
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = str.find(div);
  while (std::string::npos != pos2) {
    out->push_back(str.substr(pos1, pos2-pos1));
    pos1 = pos2 + div.size();
    pos2 = str.find(div, pos1);
  }
  if (pos1 != str.length()) out->push_back(str.substr(pos1));
  return 0;
}

int GetRandomMaskKey(std::vector<char>* out) {
  if (out == nullptr) return -1;
  srand(time(nullptr));
  union {
    int i32val;
    char ch_array[4];
  } mask_key {rand()};
  out->assign(mask_key.ch_array, mask_key.ch_array+4);
  return 0;
}

}  // namespace


int HandShake(const std::string &request, std::string *respond) {
  std::vector<std::string> lines;
  std::vector<std::string> header;
  std::map<std::string, std::string> header_map;
  StringSplit(request, "\r\n", &lines);
  for (auto str : lines) {
    if (str.find(": ") != std::string::npos) {
      StringSplit(str, ": ", &header);
    } else if (str.find(":") != std::string::npos) {
      StringSplit(str, ":", &header);
    } else {
      continue;
    }
    header_map[header.at(0)] = header.at(1);
  }
  // Generate respond data.
  *respond = "HTTP/1.1 101 Switching Protocols\r\n";
  *respond += "Connection: upgrade\r\n";
  *respond += "Sec-WebSocket-Accept: ";
  std::string real_key = header_map["Sec-WebSocket-Key"];
  real_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA1 sha;
  uint32_t message_digest[5];
  sha.Reset();
  sha << real_key.c_str();
  sha.Result(message_digest);
  for (int i = 0; i < 5; i++) {
    message_digest[i] = htonl(message_digest[i]);
  }
  std::vector<char> keytemp(reinterpret_cast<char*>(message_digest),
                            reinterpret_cast<char*>(message_digest)+20);
  Base64Encode(keytemp, &real_key);
  real_key += "\r\n";
  *respond += real_key.c_str();
  *respond += "Upgrade: websocket\r\n\r\n";
  return 0;
}

int WebSocketFramePackaging(const WebSocketMsg& msg,
                            std::vector<char> *out) {
  if (out == nullptr) return -1;
  out->clear();
  uint64_t length = msg.payload_content.size();
  auto head = msg.msg_head;
  out->push_back(head.u8val[0]);
  // Payload length.
  if (length < 126) {
    head.bit.payload_len = length;
    out->push_back(head.u8val[1]);
  } else if (length == 126) {
    head.bit.payload_len = 126;
    out->push_back(head.u8val[1]);
    U16Converter converter;
    converter.value = length;
    for (int i = 0; i < 2; ++i) {
      out->push_back(converter.array[1-i]);
    }
  } else {
    head.bit.payload_len = 127;
    out->push_back(head.u8val[1]);
    U64Converter converter;
    converter.value = length;
    for (int i = 0; i < 8; ++i) {
      out->push_back(converter.array[7-i]);
    }
  }
  // Payload data.
  if (head.bit.mask) {  // Has mask key.
    std::vector<char> mask_key;
    if (GetRandomMaskKey(&mask_key) != 0) {
      mask_key.assign(kMaskKey, kMaskKey+4);
    }
    for (auto const& ch : mask_key) out->push_back(ch);
    for (uint64_t i = 0; i < length; ++i) {
      out->push_back(msg.payload_content[i]^mask_key[i%4]);
    }
  } else {
    for (auto const& ch : msg.payload_content) out->push_back(ch);
  }
  return 0;
}

int WebSocketFrameParse(std::vector<char> const& msg,
                        WebSocketMsg* out) {
  if (out == nullptr) return -1;
  auto& head = out->msg_head;
  auto& content = out->payload_content;
  head.u8val[0] = static_cast<uint8_t>(msg[0]);
  head.u8val[1] = static_cast<uint8_t>(msg[1]);
  // Payload length.
  uint64_t payload_length = head.bit.payload_len;
  int pos = 6;
  if (head.bit.payload_len == 126) {
    U16Converter converter;
    for (int i = 0; i < 2; ++i) {
      converter.array[1-i] = msg[2+i];
    }
    payload_length = converter.value;
    pos = 8;
  } else if (head.bit.payload_len > 126) {
    U64Converter converter;
    for (int i = 0; i < 8; ++i) {
      converter.array[7-i] = msg[2+i];
    }
    payload_length = converter.value;
    pos = 14;
  }
  // Payload content.
  if (head.bit.mask == 1) {
    content.clear();
    for (uint64_t i = 0; i < payload_length; ++i) {
      content.push_back(msg[pos+i]^msg[pos-4+i%4]);
    }
  } else {
    content.assign(msg.begin()+pos-4, msg.end());
  }
  return 0;
}

}  // namespace libwebsocket
