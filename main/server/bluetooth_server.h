
#ifndef LEDS_BLUETOOTH_SERVER_H
#define LEDS_BLUETOOTH_SERVER_H

#include "led_server.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>


#define SERVICE_UUID        "5bf556f6-321a-4584-b473-5fda5bbfbc15"
#define CHARACTERISTIC_UUID "15246ab5-d6b0-4539-9cd1-88676c9e7e7a"

#define BLE_MTU 20

class BluetoothServer : public LedServer, public BLEServerCallbacks, public BLECharacteristicCallbacks {
private:
  bool online = false;

  unsigned char readBuffer[1024], writeBuffer[1024], notifyBuffer[BLE_MTU];
  unsigned long last_read_time = 0, activity_time = 0;

  unsigned int readBufferIndex = 0, writeBufferIndex = 0, writeBufferLength = 0;

  bool has_connection = false;

  BLEServer* pServer = nullptr;
  BLECharacteristic* pCharacteristic = nullptr;
  bool deviceConnected = false;
  bool oldDeviceConnected = false;
  bool justDisconnected = false;
  unsigned long disconnectTime = 0;
  uint32_t value = 0;

  void updateValue(bool askForMore);

public:
  explicit BluetoothServer(LedController *controller);

  void setup() override;

  void onConnect(BLEServer* pServer) override;
  void onDisconnect(BLEServer* pServer) override;
  void onWrite(BLECharacteristic* pCharacteristic) override;

  bool isOnline() const override;

  void tick(unsigned long current_ms) override;

  bool isActive() override;
};

#endif //LEDS_BLUETOOTH_SERVER_H
