
#ifndef LEDS_LED_SERVER_H
#define LEDS_LED_SERVER_H

#include "led_controller.h"

class LedServer {
protected:
  LedController *controller;

public:
  explicit LedServer(LedController *controller) {
    this->controller = controller;
  }

  virtual void setup() {}

  virtual bool isOnline() const {
    return false;
  }

  uint8_t handlePacket(const uint8_t *&packet, unsigned long current_ms);

  void writePacket(uint8_t *packet, unsigned int &index, bool withName = true);

  virtual void tick(unsigned long current_ms) {}

  virtual bool isActive() {
    return false;
  }
};

#endif //LEDS_LED_SERVER_H
