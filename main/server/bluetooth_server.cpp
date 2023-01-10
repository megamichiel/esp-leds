
#include <Arduino.h>

#include "includes.h"

#include "bluetooth_server.h"


BluetoothServer::BluetoothServer(LedController *controller) : LedServer(controller), readBuffer(), writeBuffer() {}

void BluetoothServer::setup() {
  Serial.println("Setting up BLE");

  // Create the BLE Device
  BLEDevice::init(controller->getDeviceName().c_str());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
          CHARACTERISTIC_UUID,
          BLECharacteristic::PROPERTY_READ   |
          BLECharacteristic::PROPERTY_WRITE  |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristic->setCallbacks(this);

  pService->addCharacteristic(pCharacteristic);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  auto desc = new BLE2902();
  desc->setValue("State");
  pCharacteristic->addDescriptor(desc);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
//  pAdvertising->setScanResponse(false);
//  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  pAdvertising->start();

  digitalWrite(LED_BUILTIN, 0);

  Serial.println("Waiting for a connection...");
}

void BluetoothServer::onConnect(BLEServer *server) {
  deviceConnected = true;
}

void BluetoothServer::onDisconnect(BLEServer *server) {
  deviceConnected = false;
}

void BluetoothServer::updateValue(bool askForMore) {
  // Update info characteristic
  unsigned int packetLen = 0;

  writePacket(writeBuffer, packetLen, false); // Don't write name

  pCharacteristic->setValue(writeBuffer, packetLen);
  pCharacteristic->notify();
}

void BluetoothServer::onWrite(BLECharacteristic* characteristic) {
  const uint8_t *packet = characteristic->getData();

  // Packet can be split up
  uint8_t dataIndicator = *(packet++);
  bool endOfData = false;

  switch (dataIndicator) {
    case 0: // Single packet, so start and end of data
      endOfData = true;
      // fall-through

    case 1: // Start of data, but more is to come
      readBufferIndex = 0;
      // fall-through

    case 2: // More data is to come
      memcpy(readBuffer + readBufferIndex, packet, characteristic->getLength() - 1);
      readBufferIndex += characteristic->getLength() - 1;
      break;

    case 3: // End of data
      memcpy(readBuffer + readBufferIndex, packet, characteristic->getLength() - 1);
      endOfData = true;
      break;

    default:
      // Undefined
      return;
  }

  if (endOfData) {
    packet = readBuffer;
    readBufferIndex = 0;

    uint8_t flags = handlePacket(packet, millis());

    if (flags == 0) { // If flags == 0, we're retrieving values
      switch (*(packet++)) {
        case 0: // Start of read
          // We need to send packet in chunks of size BLE_MTU
          writeBufferLength = 0;
          writePacket(writeBuffer, writeBufferLength, false); // Don't write name

          writeBufferIndex = 0;
          notifyBuffer[0] = 1; // Indicate that there's more data
          // Fall-through

        case 1: { // More to read
          unsigned int length = BLE_MTU - 1;

          if (writeBufferIndex + length >= writeBufferLength) {
            length = writeBufferLength - writeBufferIndex;
            notifyBuffer[0] = 0; // Indicate end of data
          }
          memcpy(notifyBuffer + 1, writeBuffer + writeBufferIndex, length); // Copy packet info to notify buffer
          pCharacteristic->setValue(notifyBuffer, length + 1);
          pCharacteristic->notify();

          writeBufferIndex += length;

          break;
        }
      }
    }
  }
}

bool BluetoothServer::isOnline() const {
  return online;
}

void BluetoothServer::tick(unsigned long current_ms) {
  // disconnecting
  if (justDisconnected && (current_ms - disconnectTime) > 500) {
    justDisconnected = false;

    // This is not useless
    pServer->startAdvertising(); // restart advertising
    oldDeviceConnected = deviceConnected;
  }

  if (!deviceConnected && oldDeviceConnected) {
    if (!justDisconnected) {
      debugln("Device disconnected");

      justDisconnected = true;
      disconnectTime = current_ms;

      has_connection = false;
    }
  }

  if (deviceConnected && !oldDeviceConnected) {
    // New device has connected
    debugln("Device connected");

    oldDeviceConnected = deviceConnected;
    has_connection = true;
  }
}

bool BluetoothServer::isActive() {
  return has_connection;
}
