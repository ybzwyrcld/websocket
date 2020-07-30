// MIT License
//
// Copyright (c) 2020 Yuming Meng
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// @File    :  base64.cc
// @Version :  1.0
// @Time    :  2020/07/30 13:45:06
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None


#include "base64.h"

#include <stdint.h>
#include <string.h>


namespace libwebsocket {

namespace {

constexpr char kBase64CodingTable[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline int GetIndexByChar(char const& ch) {
  for (int i = 0; i < 64; ++i) {
    if (ch == kBase64CodingTable[i]) {
      return i;
    }
  }
  return -1;
}

inline char GetCharByIndex(int const& index) {
  return kBase64CodingTable[index%64];
}

}  // namespace


int Base64Encode(std::vector<char> const& src, std::string* out) {
  if (src.empty() || out == nullptr) return -1;
  int const& len = src.size();
  char temp[3] = {0};
  int i = 0;
  int fill_count = 0;
  if (len%3 != 0) fill_count = 3-len%3;
  out->clear();
  while (i < len) {
    memcpy(temp, src.data()+i, len-i >= 3 ? 3 : len-i);
    out->push_back(GetCharByIndex((temp[0]&0xFC)>>2));
    out->push_back(GetCharByIndex(((temp[0]&0x3)<<4)|((temp[1]&0xF0)>>4)));
    if (temp[1] == 0) break;
    out->push_back(GetCharByIndex(((temp[1]&0xF)<<2)|((temp[2]&0xC0)>>6)));
    if (temp[2] == 0) break;
    out->push_back(GetCharByIndex(temp[2]&0x3F));
    i += 3;
    memset(temp, 0x0, 3);
  }
  for (int i = 0; i < fill_count; ++i) out->push_back('=');
  return 0;
}

int Base64Decode(std::string const& src, std::vector<char>* out) {
  if (src.empty() || src.size()%4 != 0|| out == nullptr) return -1;
  char temp[4] = {0};
  int i = 0;
  out->clear();
  int const& length = src.size();
  while (i < length) {
    memcpy(temp, src.data()+i, 4);
    out->push_back(((GetIndexByChar(temp[0])&0x3F)<<2)|
                   ((GetIndexByChar(temp[1])&0x3F)>>4));
    if (temp[2] == '=') break;
    out->push_back(((GetIndexByChar(temp[1])&0xF)<<4)|
                   ((GetIndexByChar(temp[2])&0x3F)>>2));
    if (temp[3] == '=') break;
    out->push_back(((GetIndexByChar(temp[2])&0x3)<<6)|
                   ((GetIndexByChar(temp[3])&0x3F)));
    i += 4;
  }
  return 0;
}

}  // namespace libwebsocket
