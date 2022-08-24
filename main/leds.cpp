#include <cmath>
#include <list>

#include <FastLED.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include "WiFiUdp.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "formula.h"
#include "includes.h"
#include "util.h"

using namespace std;

WiFiUDP server;
bool serverOnline = false;

void onWiFiEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("Wifi connected");

      digitalWrite(LED_BUILTIN, 0);

      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      String ip = WiFi.localIP().toString();

      if (strcmp(ip.c_str(), "255.255.255.255") == 0) {
        Serial.println("Received 255.255.255.255 as IP, retrying in 10 seconds");
        digitalWrite(LED_BUILTIN, 1);
        delay(10000);
        WiFi.begin(SSID, PASS);
        break;
      }

      Serial.printf("Wifi IP: %s\n", ip.c_str());

      if (MDNS.begin(MDNS_NAME))
        Serial.println("MDNS responder started");

      server.begin(PORT);
      serverOnline = true;
      server.setTimeout(10000);
      Serial.println("UDP server started");
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      digitalWrite(LED_BUILTIN, 1);
      serverOnline = false;
      server.stop();
      Serial.println("Lost wifi connection, attempting to reconnect");
      WiFi.begin(SSID, PASS);
      break;

    default:
      break;
  }
}

struct FormulaData {
  FormulaType type = int_formula;
  Form *form = nullptr;
  bool isVariable = false, isTimed = false;

  String toString() const {
    return form->toString(type);
  }

  int eval(int x, int t) const {
    return type == int_formula ? form->eval(x, t) : (int) form->eval((double) x, (double) t);
  }
};

CRGB leds[NUM_LEDS];

FormulaData formulas[3];
bool timedFormulas = false, variableFormulas = false;

uint8_t bright;
uint16_t fade;
int tick;

unsigned char readBuffer[1024], writeBuffer[1024];
bool changed = false, has_connection = false;
unsigned long last_changed = 0, last_read_time = 0, activity_time = 0;

void replaceFormula(FormulaData &data, const char *str) {
  Form *form = parseFormula(str);
  if (form != nullptr) {
    delete data.form;

    data.form = form;
    data.isVariable = form->isVariable();
    data.isTimed = form->isTimed();
  }

  timedFormulas = variableFormulas = false;
  for (const FormulaData &formula : formulas) {
    timedFormulas |= formula.isTimed;
    variableFormulas |= formula.isVariable;
  }
}

void saveConfig() {
  debugln("Saving config");

  EEPROM.writeBytes(0, "CentralLED", 10);
  int addr = 10;
  for (FormulaData &data : formulas) {
    String s = data.form->toString(data.type);
    EEPROM.write(addr++, data.type);
    EEPROM.writeString(addr, s);
    addr += (int) s.length() + 1;
  }
  EEPROM.write(addr++, bright);
  EEPROM.write(addr++, fade >> 8);
  EEPROM.write(addr, fade & 0xFF);

  EEPROM.commit();
}

void loadConfig() {
  debugln("Loading config");

  char header[10];
  EEPROM.readBytes(0, header, 10);
  if (memcmp(header, "CentralLED", 10) == 0) {
    int addr = 10;

    for (FormulaData &data : formulas) {
      data.type = (FormulaType) EEPROM.read(addr++);
      String s = EEPROM.readString(addr);
      addr += (int) s.length() + 1;
      replaceFormula(data, s.c_str());
    }
    bright = EEPROM.read(addr++);
    fade = EEPROM.read(addr++) << 8;
    fade |= EEPROM.read(addr);
  } else {
    replaceFormula(formulas[0], "x+t");
    replaceFormula(formulas[1], "255");
    replaceFormula(formulas[2], "255");
    bright = 4;
    fade = 1;
  }
  debugln(formulas[0].toString());
  debugln(formulas[1].toString());
  debugln(formulas[2].toString());

  debugf("Bright: %i\n", bright);
}

void updateLeds() {
  CRGB c{}, p = CRGB(0, 0, 0);

  static uint8_t h, s, v;
  if (!variableFormulas) {
    h = formulas[0].eval(0, tick) & 0xFF; // hue % 256
    s = clampByte(formulas[1].eval(0, tick));
    v = clampByte(formulas[2].eval(0, tick));
  }

  debugf("hsv = %i %i %i\n", h, s, v);

  for (int x = 0, y = 0, z, strip, offset; x - fade + 1 < NUM_LEDS; x += fade, y = fade - 1, p = c) {
    if (variableFormulas) {
      h = formulas[0].eval(x, tick) & 0xFF; // hue % 256
      s = clampByte(formulas[1].eval(x, tick));
      v = clampByte(formulas[2].eval(x, tick));
    }

    c = CHSV(h, s, v);

    while (y >= 0 && (z = x - y) < NUM_LEDS) {
      // Do some re-calculation for multiple strips
      offset = 0;
      strip = 0;
      while (z - offset >= led_count[strip])
        offset += led_count[strip++];

      if ((strip & 1) == 0)
        z = offset + (led_count[strip] - (z - offset));

      leds[z] = CRGB(
              (y * p.r + (fade - y) * c.r) / fade,
              (y * p.g + (fade - y) * c.g) / fade,
              (y * p.b + (fade - y) * c.b) / fade
      );
      --y;
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  Serial.begin(115200);

  delay(50);

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onWiFiEvent);

#ifdef STATIC_IP
  WiFi.config(STATIC_IP, STATIC_GATEWAY, STATIC_MASK);  // arduino-esp32 #2537
#endif
  WiFi.setHostname("central-led");
  WiFi.begin(SSID, PASS);

  EEPROM.begin(512);

  Serial.println();

  loadConfig();
  tick = 0;

  for (auto &led : leds)
    led = 0;

  FastLED.addLeds<LED_TYPE, STRIP_1_PIN, COLOR_ORDER>(leds, led_count[0]).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, STRIP_2_PIN, COLOR_ORDER>(leds + led_count[0], led_count[1]).setCorrection(TypicalLEDStrip);

  updateLeds();
  FastLED.setBrightness(bright);
  FastLED.show();
}

void loop() {
  unsigned long current_ms = millis();
  int packetSize;

  // If there's no active connection, only read a packet every second
  if (serverOnline && (has_connection || current_ms - last_read_time >= 3000)) {
    last_read_time = current_ms;

    if ((packetSize = server.parsePacket()) != 0) {
      unsigned int readBytes = 0, packetLen;

      do {
        readFully(server, readBuffer, 2);
        readFully(server, readBuffer, packetLen = readBuffer[0] << 8 | readBuffer[1]);

        readBytes += 2 + packetLen;

        uint8_t *packet = readBuffer;

        switch (*(packet++)) {
          case 0: // Ping
            has_connection = true;
            activity_time = current_ms;

            server.beginPacket();
            server.write((const uint8_t *) "\x00\x01\x00", 3); // Pong!
            server.endPacket();
            break;

          case 1: {
            uint8_t flags = *(packet++);

            debugf("Updating leds, flags: %x\n", flags);

            if ((flags & 1)) {
              bright = *(packet++);

              FastLED.setBrightness(bright);

              if (bright == 0)
                FastLED.show();
            }
            if (flags & 2) {
              fade = *(packet++) << 8;
              fade |= *(packet++);
              if (fade == 0)
                fade = 1;
            }

            for (int i = 0; i < 3; ++i) {
              if (flags & (4 << i)) {
                formulas[i].type = *(packet++) ? double_formula : int_formula;
                replaceFormula(formulas[i], (const char *) packet);
                packet += strlen((const char *) packet) + 1;
              }
            }

            changed = true;
            has_connection = true;
            last_changed = activity_time = current_ms;
            server.beginPacket();
            server.write((const uint8_t *) "\x00\x01\x01", 3);
            server.endPacket();

            updateLeds();

            break;
          }
          case 2:
            packetLen = 2;
            writeBuffer[packetLen++] = 2;
            writeBuffer[packetLen++] = NUM_LEDS >> 8;
            writeBuffer[packetLen++] = NUM_LEDS & 0xFF;
            writeBuffer[packetLen++] = bright;
            writeBuffer[packetLen++] = fade >> 8;
            writeBuffer[packetLen++] = fade & 0xFF;
            for (FormulaData &formula: formulas) {
              String str = formula.toString();
              writeBuffer[packetLen++] = (uint8_t) formula.type;
              writeBuffer[packetLen++] = str.length();
              memcpy(writeBuffer + packetLen, str.c_str(), str.length());
              packetLen += str.length();
            }

            has_connection = true;
            activity_time = current_ms;

            server.beginPacket();
            packetLen -= 2;
            writeBuffer[0] = packetLen >> 8;
            writeBuffer[1] = packetLen & 0xFF;
            server.write(writeBuffer, packetLen + 2);
            server.endPacket();

            break;
        }
      } while (readBytes < packetSize);
    }
  }

  // 5 seconds after something was last changed, save, instead of saving on every change
  if (changed && current_ms - last_changed > 5000) {
    changed = false;
    saveConfig();
  }

  // Track if the device hasn't received any data in the last 10 seconds
  if (has_connection && current_ms - activity_time > 10000)
    has_connection = false;

  if (bright > 0) {
    if (timedFormulas)
      updateLeds();

    tick = (tick + 1) & 0x7FFFFFFF; // Keeping it positive
  } else if (!has_connection) { // If the light is off and there's an active connection, wait longer until further action
    delay(3000);
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
