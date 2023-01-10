
#ifndef LEDS_WIFI_SERVER_H
#define LEDS_WIFI_SERVER_H

#include "Arduino.h"
#include "WiFi.h"
#include "WiFiUdp.h"

#include "led_server.h"

class WifiServer : public LedServer {
private:
  WiFiUDP server;
  bool online = false;

  unsigned char readBuffer[1024], writeBuffer[1024];
  unsigned long last_read_time = 0, activity_time = 0, keep_alive_time = 0;

  bool has_connection = false;

public:
  explicit WifiServer(LedController *controller);

  void setup() override;

  void handleWifiEvent(arduino_event_id_t event);

  bool isOnline() const override;

  void tick(unsigned long current_ms) override;

  bool isActive() override;
};

#endif //LEDS_WIFI_SERVER_H
