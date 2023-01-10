
#include <Arduino.h>

#include <WiFi.h>
#include <ESPmDNS.h>

#include "includes.h"
#include "util.h"

#include "wifi_server.h"

WifiServer *instance = nullptr;

void onWifiEvent(arduino_event_id_t event) {
  if (instance != nullptr) {
    instance->handleWifiEvent(event);
  }
}

WifiServer::WifiServer(LedController *controller) : LedServer(controller), readBuffer(), writeBuffer() {}

void WifiServer::setup() {
  instance = this;

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onWifiEvent);

#ifdef WIFI_NO_DHCP
  WiFi.config(WIFI_STATIC_IP, WIFI_STATIC_GATEWAY, WIFI_STATIC_MASK);  // arduino-esp32 #2537
#endif
  WiFi.setHostname("central-led");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void WifiServer::handleWifiEvent(arduino_event_id_t event) {
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
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        break;
      }

      Serial.printf("Wifi IP: %s\n", ip.c_str());

      if (MDNS.begin(WIFI_MDNS_NAME))
        Serial.println("MDNS responder started");

      server.begin(WIFI_PORT);
      online = true;
      server.setTimeout(10000);
      Serial.println("UDP server started");
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      digitalWrite(LED_BUILTIN, 1);
      online = false;
      server.stop();
      Serial.println("Lost wifi connection, attempting to reconnect");
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      break;

    default:
      break;
  }
}

bool WifiServer::isOnline() const {
  return online;
}

void WifiServer::tick(unsigned long current_ms) {
  int packetSize;
  // If there's no active connection, only read a packet every second
  if (online && (has_connection || current_ms - last_read_time >= INACTIVE_PACKET_READ_INTERVAL)) {
    last_read_time = current_ms;

    if ((packetSize = server.parsePacket()) != 0) {
      unsigned int readBytes = 0, packetLen;

      do {
        readFully(server, readBuffer, 2);
        readFully(server, readBuffer, packetLen = readBuffer[0] << 8 | readBuffer[1]);

        readBytes += 2 + packetLen;

        const uint8_t *packet = readBuffer;

        switch (*(packet++)) {
          default:
            continue;

          case 0: // Ping
            has_connection = true;
            activity_time = current_ms;

            server.beginPacket();
            server.write((const uint8_t *) "\x00\x01\x00", 3); // Pong!
            server.endPacket();
            break;

          case 1: {
            handlePacket(packet, current_ms);

            has_connection = true;
            activity_time = current_ms;

            server.beginPacket();
            server.write((const uint8_t *) "\x00\x01\x01", 3);
            server.endPacket();

            break;
          }
          case 2:
            packetLen = 2;
            writeBuffer[packetLen++] = 2;
            writePacket(writeBuffer, packetLen);

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

        if (activity_time == current_ms) {
          keep_alive_time = activity_time;
        }
      } while (readBytes < packetSize);
    }
  }

  // Track if the device hasn't received any data in the last 10 seconds
  if (has_connection && current_ms - activity_time > INACTIVE_DELAY)
    has_connection = false;

  // Send something random to keep the wlan connection alive every 5 minutes
  if (online && !has_connection && current_ms - keep_alive_time > KEEP_ALIVE_INTERVAL) {
    keep_alive_time = current_ms;

    WiFiClient client;
    if (client.connect("192.168.1.1", 80)) {
      client.println("GET / HTTP/1.1\n");

      client.stop();
    }
  }
}

bool WifiServer::isActive() {
  return has_connection;
}
