#include "BLEManager.h"
#include "config.h"

namespace BLEManager {
  
  bool deviceConnected = false;

  void (*msgReceivedCallback)(String);

  BLEUUID CONTROL_UUID = BLEUUID(CONTROL_CHARACTERISTIC_UUID);

  BLEServer *pServer;
  BLEService *pService;
  BLECharacteristic *controlCharacteristic;
  BLEService *infoService;
  BLECharacteristic *softwareAPIVersionCharacteristic;
  BLECharacteristic *softwareFirmwareVersionCharacteristic;

  void MessageCallbacks::onWrite(BLECharacteristic *characteristic) {
    std::string cppStr = characteristic->getValue();
    String cmd = String(cppStr.c_str());
    // TODO: process settings update
    if (characteristic->getUUID().equals(CONTROL_UUID)) {
      msgReceivedCallback(cmd);
    }
  };

  // Client connected to OSSM over BLE
  void ServerCallbacks::onConnect(BLEServer* pServer) {
    BLEManager::deviceConnected = true;
    Serial.println("BLE Connected");
  };

  void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    BLEManager::deviceConnected = false;
    Serial.println("BLE Disconnected");
    pServer->startAdvertising();
    // TODO: stop
  };

  void sendNotification (String message) {
    controlCharacteristic->setValue(message.c_str());
    controlCharacteristic->notify(true);
  }

  void setup (String bleName, void (*msgReceivedCallback)(String)) {

    BLEManager::msgReceivedCallback = msgReceivedCallback;

    //###################
    // Initiate Bluetooth
    //###################
    Serial.println("Initializing BLE Server...");
    BLEDevice::init(bleName.c_str());
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    infoService = pServer->createService(BLEUUID((uint16_t) 0x180a));
    BLE2904* softwareVersionDescriptor = new BLE2904();
    softwareVersionDescriptor->setFormat(BLE2904::FORMAT_UINT8);
    softwareVersionDescriptor->setNamespace(1);
    softwareVersionDescriptor->setUnit(0x27ad);

    softwareAPIVersionCharacteristic = infoService->createCharacteristic((uint16_t) 0x2a28, BLECharacteristic::PROPERTY_READ);
    softwareAPIVersionCharacteristic->addDescriptor(softwareVersionDescriptor);
    softwareAPIVersionCharacteristic->addDescriptor(new BLE2902());
    softwareAPIVersionCharacteristic->setValue(API_VERSION);

    softwareFirmwareVersionCharacteristic = infoService->createCharacteristic((uint16_t) 0x2a26, BLECharacteristic::PROPERTY_READ);
    softwareFirmwareVersionCharacteristic->addDescriptor(softwareVersionDescriptor);
    softwareFirmwareVersionCharacteristic->addDescriptor(new BLE2902());
    softwareFirmwareVersionCharacteristic->setValue(FIRMWARE_VERSION);
    infoService->start();
    
    pService = pServer->createService(SERVICE_UUID);
    controlCharacteristic = pService->createCharacteristic(
                                          CONTROL_UUID,
                                          BLECharacteristic::PROPERTY_READ |
                                          BLECharacteristic::PROPERTY_WRITE |
                                          BLECharacteristic::PROPERTY_NOTIFY
                                        );
    controlCharacteristic->addDescriptor(new BLE2902());
    controlCharacteristic->setValue("");
    controlCharacteristic->setCallbacks(new MessageCallbacks());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Done");
  };
};