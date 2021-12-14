#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLE2904.h"
#include <FastAccelStepper.h>
#include <list>
#include <Preferences.h>

const char* FIRMWARE_VERSION = "v1.0";

#define PUL_PIN 4
#define DIR_PIN 18
#define ENA_PIN 22

#define DEFAULT_MAX_SPEED 200
#define DEFAULT_MIN_SPEED 2000
#define DEFAULT_MAX_POSITION_IN 0
#define DEFAULT_MAX_POSITION_OUT 1000
#define STEPS_PER_MM 20

#define NUM_NONE 0
#define NUM_CHANNEL 1
#define NUM_PERCENT 2
#define NUM_DURATION 3
#define NUM_VALUE 4

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID                "e556ec25-6a2d-436f-a43d-82eab88dcefd"

#define CONTROL_CHARACTERISTIC_UUID "c4bee434-ae8f-4e67-a741-0607141f185b"
// WRITE
// T-Code messages in the format:
// ex. L199I100 = move linear actuator to the 99% position over 100ms
// ex. L10I100 = move linear actuator to the 0% position over 100ms
// DSTOP = stop
// DENABLE = enable motor (non-standard T-Code command)
// DDISABLE = disable motor (non-standard T-Code command)

#define SETTINGS_CHARACTERISTIC_UUID "fe9a02ab-2713-40ef-a677-1716f2c03bad"
// WRITE
// Preferences in the format:
// minSpeed:200 = set minimum speed of half-stroke to 200ms (used by XToys client)
// maxSpeed:2000 = set maximum speed of half-stroke to 2000ms (used by XToys client)
// maxOut:0 = set the position the stepper should stop at when moving in the one direction
// maxIn:1000 = set the position the stepper should stop at when moving in the other direction
// READ
// Returns all preference values in the format:
// minSpeed:200,maxSpeed:2000,maxOut:0,maxIn:1000

// Global Variables - Bluetooth
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *controlCharacteristic;
BLECharacteristic *settingsCharacteristic;
BLEService *infoService;
BLECharacteristic *softwareVersionCharacteristic;

// Global Variables - Stepper
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;
int targetStepperPosition = 0;
bool stepperEnabled = false;
bool stepperMoving = false;

// Preferences
Preferences preferences;
int maxInPosition;
int maxOutPosition;
int maxSpeed;
int minSpeed;

// Other
bool deviceConnected = false;
std::list<std::string> pendingCommands = {};


void updateSettingsCharacteristic();
void processCommand(std::string msg);
void moveTo(int targetPosition, int targetDuration);

// Received request to update a setting
class SettingsCharacteristicCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) {
    std::string msg = characteristic->getValue();

    Serial.print("Received command: ");
    Serial.println(msg.c_str());

    std::size_t pos = msg.find(':');
    std::string settingKey = msg.substr(0, pos);
    std::string settingValue = msg.substr(pos + 1, msg.length());

    if (settingKey == "maxIn") {
      maxInPosition = atoi(settingValue.c_str());
      preferences.putInt("maxIn", maxInPosition);
    }
    if (settingKey == "maxOut") {
      maxOutPosition = atoi(settingValue.c_str());
      preferences.putInt("maxOut", maxOutPosition);
    }
    if (settingKey == "maxSpeed") {
      maxSpeed = atoi(settingValue.c_str());
      preferences.putInt("maxSpeed", maxSpeed);
    }
    if (settingKey == "minSpeed") {
      minSpeed = atoi(settingValue.c_str());
      preferences.putInt("minSpeed", minSpeed);
    }
    Serial.print("Setting pref ");
    Serial.print(settingKey.c_str());
    Serial.print(" to ");
    Serial.println(settingValue.c_str());
    updateSettingsCharacteristic();
  }
};

// Received T-Code command
class ControlCharacteristicCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) {
    std::string msg = characteristic->getValue();

    Serial.print("Received command: ");
    Serial.println(msg.c_str());

    // check for messages that might need to be immediately handled
    if (msg == "DSTOP") { // stop request
      Serial.println("STOP");
      stepper->stopMove();
      pendingCommands.clear();
      stepperMoving = false;
      return;
    } else if (msg == "DENABLE") { // enable stepper motor
      Serial.println("ENABLE");
      stepper->enableOutputs();
      stepper->forceStopAndNewPosition(0);
      pendingCommands.clear();
      stepperMoving = false;
      stepperEnabled = true;
      return;
    } else if (msg == "DDISABLE") { // disable stepper motor
      Serial.println("DISABLE");
      stepper->disableOutputs();
      pendingCommands.clear();
      stepperMoving = false;
      stepperEnabled = false;
      return;
    }
    if (msg.front() == 'D') { // device status message, process immediately (technically the code isn't handling any T-Code 'D' messages currently
      processCommand(msg);
      return;
    }
    if (msg.back() == 'C') { // movement command includes a clear existing commands flag, clear queue and process new movement command immediately
      pendingCommands.clear();
      processCommand(msg);
      return;
    }
    // probably a normal movement command, store it to be run after other movement commands are finished
    if (!stepperMoving) {
      processCommand(msg);
    } else if (pendingCommands.size() < 100) {
      pendingCommands.push_back(msg);
      Serial.print("# of pending commands: ");
      Serial.println(pendingCommands.size());
    } else {
      Serial.print("Too many commands in queue. Dropping: ");
      Serial.println(msg.c_str());
    }
  }
};

// Read actions and numeric values from T-Code command
void processCommand(std::string msg) {
  
  char command = NULL;
  int channel = 0;
  int targetAmount = 0;
  int targetDuration = 0;
  int numBeingRead = NUM_NONE;

  for (char c : msg) {
    switch (c) {
      case 'l':
      case 'L':
        command = 'L';
        numBeingRead = NUM_CHANNEL;
        break;
      case 'i':
      case 'I':
        numBeingRead = NUM_DURATION;
        break;
      case 'D':
      case 'd':
        command = 'D';
        numBeingRead = NUM_CHANNEL;
        break;
      case 'v':
      case 'V':
        numBeingRead = NUM_VALUE;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        int num = c - '0'; // convert from char to numeric int
        switch (numBeingRead) {
          case NUM_CHANNEL:
            channel = num;
            numBeingRead = NUM_PERCENT;
            break;
          case NUM_PERCENT:
            targetAmount = targetAmount * 10 + num;
            break;
          case NUM_DURATION:
            targetDuration = targetDuration * 10 + num;
            break;
        }
        break;
    }
  }

  // if amount was less than 5 digits increase to 5 digits
  // t-code message is a value to the right of the decimal place so we need a consistent length to work with in moveTo command
  // ex L99 means 0.99000 and L10010 means 0.10010
  if (command == 'L' && channel == 1) {
    if (targetAmount != 0) {
      while (targetAmount < 10000) {
        targetAmount *= 10;
      }
    }
    moveTo(targetAmount, targetDuration);
  } else if (command == 'D') { // not handling currently
  } else {
    Serial.print("Invalid command: ");
    Serial.println(msg.c_str());
  }
}

// Move stepper to requested position
// targetPosition = value between 0 and 99999 from T-Code command
// targetDuration = how quickly to move to targetPosition (in ms)
void moveTo(int targetPosition, int targetDuration) {

  // if (pendingCommands.size() > 5) {
  //  targetDuration *= 0.7; // if falling behind on pattern make slower strokes faster to catch up
  // }

  int currentStepperPosition = stepper->getCurrentPosition();

  // convert from (0-10000) to (inPos-outPos) range
  targetStepperPosition = -targetPosition * (maxInPosition - maxOutPosition) / 10000 + maxOutPosition * STEPS_PER_MM;

  // steps/s
  int targetStepperSpeed = abs(targetStepperPosition - currentStepperPosition) / (targetDuration / 1000.0);
  
  stepper->setSpeedInHz(targetStepperSpeed);
  stepper->moveTo(targetStepperPosition);
  stepperMoving = true;
  
  logMotion(targetStepperPosition, targetDuration);
}

// Update value readable from Settings characteristic
void updateSettingsCharacteristic() {
  String settingsInfo = String("maxIn:") + maxInPosition + ",maxOut:" + maxOutPosition + ",maxSpeed:" + maxSpeed + ",minSpeed:" + minSpeed;
  settingsCharacteristic->setValue(settingsInfo.c_str());
}

// Log stepper motion event
void logMotion(int targetPosition, int targetDuration) {
  Serial.print("Moving to position ");
  Serial.print(targetPosition);
  Serial.print(" over ");
  Serial.print(targetDuration);
  Serial.println("ms");
  Serial.println("------");
}

// Client connected to OSSM over BLE
class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE Connected");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE Disconnected");
    pServer->startAdvertising();
    stepper->stopMove();
    stepperMoving = false;
  }
};


void setup()
{
  Serial.begin(115200);
  Serial.println("----------");

  //###################
  // Initiate Preferences
  //###################
  Serial.println("Loading Settings...");
  preferences.begin("OSSM", false);
  maxInPosition = preferences.getInt("maxIn", DEFAULT_MAX_POSITION_IN);
  maxOutPosition = preferences.getInt("maxOut", DEFAULT_MAX_POSITION_OUT);
  maxSpeed = preferences.getInt("maxSpeed", DEFAULT_MAX_SPEED);
  minSpeed = preferences.getInt("minSpeed", DEFAULT_MIN_SPEED);
  Serial.printf("Max In Position: %d\nMax Out Position: %d\nMax Speed: %d\nMin Speed: %d\nDone\n", maxInPosition, maxOutPosition, maxSpeed, minSpeed);

  //###################
  // Initiate Stepper
  //###################
  Serial.println("Initializing Stepper...");
  engine.init();
  stepper = engine.stepperConnectToPin(PUL_PIN);
  if (stepper) {
    stepper->setDirectionPin(DIR_PIN);
    stepper->setEnablePin(ENA_PIN);
    stepper->setAutoEnable(false); // if set true, Fastaccelstepper disables the motor if it's not moving which means it won't push back
    stepper->setAcceleration(10000 * STEPS_PER_MM); // shrug. I dunno. Setting to a huge number because setting a proper acceleration kept resulting in steps being lost for me.
    stepper->forceStopAndNewPosition(0);
    stepper->disableOutputs(); // disable stepper by default until an enable command is received over BLE
  }
  Serial.println("Done");

  //###################
  // Initiate Bluetooth
  //###################
  Serial.println("Initializing BLE Server...");
  BLEDevice::init("OSSM");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  infoService = pServer->createService(BLEUUID((uint16_t) 0x180a));
  BLE2904* softwareVersionDescriptor = new BLE2904();
  softwareVersionDescriptor->setFormat(BLE2904::FORMAT_UINT8);
  softwareVersionDescriptor->setNamespace(1);
  softwareVersionDescriptor->setUnit(0x27ad);

  softwareVersionCharacteristic = infoService->createCharacteristic((uint16_t) 0x2a28, BLECharacteristic::PROPERTY_READ);
  softwareVersionCharacteristic->addDescriptor(softwareVersionDescriptor);
  softwareVersionCharacteristic->addDescriptor(new BLE2902());
  softwareVersionCharacteristic->setValue(FIRMWARE_VERSION);
  infoService->start();
  
  pService = pServer->createService(SERVICE_UUID);
  controlCharacteristic = pService->createCharacteristic(
                                         CONTROL_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  controlCharacteristic->addDescriptor(new BLE2902());
  controlCharacteristic->setValue("");
  controlCharacteristic->setCallbacks(new ControlCharacteristicCallback());
  
  settingsCharacteristic = pService->createCharacteristic(
                                         SETTINGS_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  settingsCharacteristic->addDescriptor(new BLE2902());
  settingsCharacteristic->setValue("");
  settingsCharacteristic->setCallbacks(new SettingsCharacteristicCallback());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  updateSettingsCharacteristic();
  Serial.println("Done");
  
  Serial.println("OSSM is now ready");
}

// Wait for stepper to get to target position and then start move to next position if queue has any additional commands
void loop()
{
  if (stepper->getCurrentPosition() == targetStepperPosition) {
    if (pendingCommands.size() > 0) {
      std::string command = pendingCommands.front();
      pendingCommands.pop_front();
      processCommand(command);
    } else {
      stepperMoving = false;
    }
  }
  
  delay(20);
}
