//
// Created by michiel on 1/1/22.
//

#include "util.h"

char *substr(const char *begin, const char *end) {
  char *copy = new char[end - begin + 1];
  memcpy(copy, begin, end - begin);
  copy[end - begin] = 0;
  return copy;
}

bool isSubstr(const char *str, const char *sub) {
  while (*str != 0 && *str == *sub) {
    ++str;
    ++sub;
  }

  return *sub == 0;
}

bool readFully(WiFiUDP &udp, uint8_t *buf, size_t size) {
  // TODO timeout
  size_t off = 0;
  while (off < size) {
    if (!udp.available())
      delay(5);

    off += udp.read(buf, size - off);
  }

  return true;
}

int clampByte(int i) {
  return i < 0 ? 0 : i > 255 ? 255 : i;
}
