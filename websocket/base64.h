#ifndef WEBSOCKET_BASE64_H_
#define WEBSOCKET_BASE64_H_

#include <string>


namespace libwebsocket {

std::string base64_encode(unsigned char const*, unsigned int len);
std::string base64_decode(std::string const& s);

}  // namespace libwebsocket

#endif  // WEBSOCKET_BASE64_H_
