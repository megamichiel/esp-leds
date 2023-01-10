//
// Created by michiel on 1/1/22.
//

#ifndef LEDS_INCLUDES_H
#define LEDS_INCLUDES_H

#define LED_BUILTIN 2

//#define DEBUG

#ifdef DEBUG
  #define debugf(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
  #define debugln(str) Serial.println(str)
#else
  #define debugf(fmt, ...);
  #define debugln(str);
#endif

#define NUM_LEDS 600
const int led_count[] = {300, 300};
//#define NUM_LEDS 240
//const int led_count[] = {120, 120};
const int led_strips = 2;

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define STRIP_1_PIN 12
#define STRIP_2_PIN 13

#define TICK_DURATION 50

// Uncomment this line if you want to use Bluetooth connectivity rather than Wifi
//#define USE_BLUETOOTH

#define WIFI_SSID "Your SSID"
#define WIFI_PASS "Blahblahblah"
#define WIFI_PORT 55420
#define WIFI_MDNS_NAME "central-led"

// Comment the following line if you want to use DHCP.
// For some reason it kept giving IP 255.255.255.255 (found similar issues online)
// But I couldn't fix it, so I used static stuff instead.
//#define WIFI_NO_DHCP

#define WIFI_STATIC_IP IPAddress(192, 168, 1, 11)
#define WIFI_STATIC_GATEWAY IPAddress(192, 168, 1, 1)
#define WIFI_STATIC_MASK IPAddress(255, 255, 255, 0)

#define INACTIVE_DELAY (10 * 1000)
#define INACTIVE_PACKET_READ_INTERVAL 3000
#define POST_CHANGE_SAVE_DELAY 5000
#define KEEP_ALIVE_INTERVAL (5 * 60 * 1000)

#endif //LEDS_INCLUDES_H
