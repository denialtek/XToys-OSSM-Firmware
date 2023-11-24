#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLE2904.h>

#include "config.h"

namespace BLEManager {
  
  extern bool deviceConnected;

  // Callback function to call any time a message is received
  extern void (*msgReceivedCallback)(String);

  extern BLEUUID CONTROL_UUID;

  extern BLEServer *pServer;
  extern BLEService *pService;
  extern BLECharacteristic *controlCharacteristic;
  extern BLECharacteristic *settingsCharacteristic;
  extern BLEService *infoService;
  extern BLECharacteristic *softwareVersionCharacteristic;

  class MessageCallbacks : public BLECharacteristicCallbacks {
    // Received request to update a setting
    void onWrite(BLECharacteristic *characteristic);
  };

  // Client connected to OSSM over BLE
  class ServerCallbacks: public BLEServerCallbacks {

    void onConnect(BLEServer* pServer);

    void onDisconnect(BLEServer* pServer);
  };

  void sendNotification (String message);

  void setup (String bleName, void (*msgReceivedCallback)(String));
};