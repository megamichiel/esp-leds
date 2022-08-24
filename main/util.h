//
// Created by michiel on 1/1/22.
//

#ifndef LEDS_UTIL_H
#define LEDS_UTIL_H

#include "WiFiUdp.h"


char *substr(const char *begin, const char *end);

bool isSubstr(const char *str, const char *sub);

bool readFully(WiFiUDP &client, uint8_t *buf, size_t size);

/* Program Logic */

int clampByte(int i);


#endif //LEDS_UTIL_H
