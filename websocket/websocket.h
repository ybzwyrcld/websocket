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

class WebSocket {
 public:
  WebSocket() {}
  ~WebSocket() {}
  int HandShake(const std::string &reuest, std::string *respond);
  int FormDataGenerate(const std::vector<char> &msg, std::vector<char> *out);
  int FormDataParse(const std::vector<char> &msg, std::vector<char> *out);
  void set_fin(const uint8_t &fin) { fin_ = fin; }
  void set_reserve(const uint8_t &reserve) { reserve_ = reserve; }
  void set_opcode(const uint8_t &opcode) { opcode_ = opcode; }
  void set_mask(const uint8_t &mask) { mask_ = mask; }
  void set_payload_length(const uint64_t &payload_length) {
    payload_length_ = payload_length;
  }
  uint8_t fin(void) const { return fin_; }
  uint8_t reserve(void) const { return reserve_; }
  uint8_t opcode(void) const { return opcode_; }
  uint8_t mask(void) const { return mask_; }
  uint64_t payload_length(void) const { return payload_length_; }

 private:
  uint8_t fin_ = 0;
  uint8_t reserve_ = 0;
  uint8_t opcode_ = 0;
  uint8_t mask_ = 0;
  uint64_t payload_length_ = 0;
};

}  // namespace libwebsocket

#endif  // WEBSOCKET_WEBSOCKET_H_
