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

// WebSocket协议头.
union WebSocketProtocolHead {
  struct Bits {
    // 用于表示消息接收类型, 如果接收到未知的opcode, 接收端必须关闭连接.
    uint16_t opcode:4;
    // 用于扩展定义的, 如果没有扩展约定的情况则必须为0.
    uint16_t reserve:3;
    // 用于描述消息是否结束, 如果为1则该消息为消息尾部, 如果为零则还有后续数据包.
    uint16_t fin:1;
    // 实际消息内容长度.
    // 如果其值在0-125，则是payload的真实长度.
    // 如果值是126，则后面2个字节形成的16位无符号整型数的值是payload的真实长度.
    // 如果值是127，则后面8个字节形成的64位无符号整型数的值是payload的真实长度.
    uint16_t payload_len:7;
    // 用于标识PayloadData是否经过掩码处理,
    // 客户端发出的数据帧需要进行掩码处理，所以此位是1, 数据需要解码.
    uint16_t mask:1;
  }bit;
  uint8_t u8val[2];
  uint16_t u16val;
};

// Webdocket数据帧中OPCODE定义.
enum OPCodeType {
  kOPCodePacket = 0x0,  // 附加数据帧
  kOPCodeText,  // 文本数据帧.
  kOPCodeBinary,  // 二进制数据帧.
  kOPCodeClose = 0x8,  // 连接关闭.
  kOPCodePing,  // ping.
  kOPCodePong,  // pong.
};

struct WebSocketMsg {
  // WebSocket协议头.
  WebSocketProtocolHead msg_head;
  // WebSocket协议内容.
  std::vector<char> payload_content;
};

inline bool IsHandShake(const std::string &request) {
  return ((request.find("GET / HTTP/1.1") != std::string::npos) &&
          (request.find("Connection: Upgrade") != std::string::npos ||
           request.find("Connection:Upgrade") != std::string::npos) &&
          (request.find("Upgrade: websocket") != std::string::npos ||
           request.find("Upgrade:websocket") != std::string::npos) &&
          (request.find("Sec-WebSocket-Key:") != std::string::npos));
}

int HandShake(std::string const& reuest, std::string* respond);
int WebSocketFramePackaging(WebSocketMsg const& msg, std::vector<char>* out);
int WebSocketFrameParse(std::vector<char> const& msg, WebSocketMsg* out);

}  // namespace libwebsocket

#endif  // WEBSOCKET_WEBSOCKET_H_
