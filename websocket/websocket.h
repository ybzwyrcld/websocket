// Copyright(c) 2019, 2020
// Yuming Meng <mengyuming@hotmail.com>.
// All rights reserved.
//
// Author:  Yuming Meng
// Date:  2020-01-07 09:56
// Description:  No.

#ifndef WEBSOCKET_WEBSOCKET_H_
#define WEBSOCKET_WEBSOCKET_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace libwebsocket {

enum OPCodeType {
  kOPPacket = 0x0,
  kOPText,
  kOPBinary,
  kOPClose = 0x8,
  kOPPing,
  kOPPong,
};

class WebSocket {
 public:
  WebSocket() {}
  ~WebSocket() {}
  bool IsHandShake(const std::string &request);
  int HandShake(const std::string &reuest, std::string *respond);
  int FormDataGenerate(const std::vector<char> &msg, std::vector<char> *out);
  int FormDataParse(const std::vector<char> &msg, std::vector<char> *out);

  uint8_t fin(void) const { return fin_; }
  void set_fin(const uint8_t &fin) { fin_ = fin; }

  uint8_t reserve(void) const { return reserve_; }
  void set_reserve(const uint8_t &reserve) { reserve_ = reserve; }

  uint8_t opcode(void) const { return opcode_; }
  void set_opcode(const uint8_t &opcode) { opcode_ = opcode; }

  uint8_t mask(void) const { return mask_; }
  void set_mask(const uint8_t &mask) { mask_ = mask; }

  uint64_t payload_length(void) const { return payload_length_; }
  void set_payload_length(const uint64_t &payload_length) {
    payload_length_ = payload_length;
  }

 private:
  uint8_t fin_ = 0;
  uint8_t reserve_ = 0;
  uint8_t opcode_ = 0;
  uint8_t mask_ = 0;
  uint64_t payload_length_ = 0;
  std::vector<char> payload_content_;
};

}  // namespace libwebsocket

#endif  // WEBSOCKET_WEBSOCKET_H_
