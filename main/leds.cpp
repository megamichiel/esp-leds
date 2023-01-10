#include <FastLED.h>

#include <Arduino.h>
#include <EEPROM.h>

#include "includes.h"
#include "led_controller.h"
#include "bluetooth_server.h"
#include "wifi_server.h"

using namespace std;

LedController *controller = new LedController();
#if USE_BLUETOOTH
  BluetoothServer server(controller);
#else
  WifiServer server(controller);
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  Serial.begin(115200);

  delay(50);

  EEPROM.begin(512);
  controller->loadConfig();

  // Start server
  server.setup();

  Serial.println();

  controller->init();
}

void loop() {
  unsigned long current_ms = millis();

  server.tick(current_ms);

  if (!controller->update_timed(current_ms) && !server.isActive()) { // If the light is off and there's no active connection, wait longer until further action
    delay(INACTIVE_PACKET_READ_INTERVAL);
    return;
  }

  if ((current_ms = millis() - current_ms) < TICK_DURATION) {
    delay(TICK_DURATION - current_ms);
  } else {
    Serial.printf("Tick too a little long: %lu\n", current_ms);
    delay(1);
  }

  FastLED.show();
}
