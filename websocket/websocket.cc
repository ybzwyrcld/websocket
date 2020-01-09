// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 09:56
// Description:  No.

#include "websocket.h"

#include <assert.h>
#include <arpa/inet.h>

#include <map>
#include <vector>

#include "base64.h"
#include "sha1.h"


namespace libwebsocket {

namespace {

union ProtocolHead {
  struct Bits {
    uint16_t opcode:4;
    uint16_t reserve:3;
    uint16_t fin:1;
    uint16_t payloadlen:7;
    uint16_t mask:1;
  }bit;
  uint8_t value[2];
};

union Uint16Converter {
  uint16_t value;
  uint8_t array[2];
};
union Uint64Converter {
  uint64_t value;
  uint8_t array[8];
};

int StringSplit(const std::string &str, const std::string &div,
                std::vector<std::string> *out) {
  if (out == nullptr) return -1;
  out->clear();
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = str.find(div);
  while (std::string::npos != pos2) {
    out->push_back(str.substr(pos1, pos2 - pos1));
    pos1 = pos2 + div.size();
    pos2 = str.find(div, pos1);
  }
  if (pos1 != str.length()) out->push_back(str.substr(pos1));
  return 0;
}

}  // namespace

int WebSocket::HandShake(const std::string &request, std::string *respond) {
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
  real_key =
      base64_encode(reinterpret_cast<const uint8_t *>(message_digest), 20);
  real_key += "\r\n";
  *respond += real_key.c_str();
  *respond += "Upgrade: websocket\r\n\r\n";
  return 0;
}

int WebSocket::ServerFormDataGenerate(const std::vector<char> &msg,
                                      std::vector<char> *out) {
  if (out == nullptr) return -1;
  out->clear();
  ProtocolHead head;
  head.bit.fin = fin_;
  head.bit.reserve = reserve_;
  head.bit.opcode = opcode_;
  out->push_back(head.value[0]);
  head.bit.mask = 0;  // Never need a mask.
  // Payload length.
  if (msg.size() < 126) {
    head.bit.payloadlen = msg.size();
    out->push_back(head.value[1]);
  } else if (msg.size() == 126) {
    head.bit.payloadlen = 126;
    out->push_back(head.value[1]);
    Uint16Converter converter;
    converter.value = msg.size();
    for (int i = 0; i < 2; ++i) {
      out->push_back(converter.array[1-i]);
    }
  } else {
    head.bit.payloadlen = 127;
    out->push_back(head.value[1]);
    Uint64Converter converter;
    converter.value = msg.size();
    for (int i = 0; i < 8; ++i) {
      out->push_back(converter.array[7-i]);
    }
  }
  // Masking key.
  if (head.bit.mask) {
    for (int i = 0; i < 4; ++i) {
      out->push_back(0x0);
    }
  }
  // Payload data.
  out->insert(out->end(), msg.begin(), msg.end());
  return 0;
}

int WebSocket::ClientFormDataParse(const std::vector<char> &msg,
                                   std::vector<char> *out) {
  if (out == nullptr) return -1;
  out->clear();
  ProtocolHead head;
  head.value[0] = static_cast<uint8_t>(msg[0]);
  head.value[1] = static_cast<uint8_t>(msg[1]);
  fin_ = head.bit.fin;
  reserve_ = head.bit.reserve;
  opcode_ = head.bit.opcode;
  mask_ = head.bit.mask;
  payload_length_ = head.bit.payloadlen;
  // Payload length.
  int pos = 6;
  if (payload_length_ == 126) {
    Uint16Converter converter;
    for (int i = 0; i < 2; ++i) {
      converter.array[1 - i] = msg[2 + i];
    }
    payload_length_ = converter.value;
    pos = 8;
  } else if (payload_length_ > 126) {
    Uint64Converter converter;
    for (int i = 0; i < 8; ++i) {
      converter.array[7 - i] = msg[2 + i];
    }
    payload_length_ = converter.value;
    pos = 14;
  }
  // Payload content.
  if (mask_ == 1) {
    for (int i = 0; i < payload_length_; ++i) {
      out->push_back(msg[pos + i] ^ msg[pos - 4 + i % 4]);
    }
  } else {
    out->assign(msg.begin() + pos, msg.end());
  }
  return 0;
}

}  // namespace libwebsocket
