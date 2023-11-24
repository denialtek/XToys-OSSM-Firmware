#include <Arduino.h>
#include <StrokeEngine.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Preferences.h>

#include "config.h"
#if COMPILE_BLUETOOTH
  #include "BLEManager.h"
#endif
#if COMPILE_WEBSOCKET
  #include "WebsocketManager.h"
#endif

// prefs configured during firmware flash from XToys website
Preferences preferences;
boolean prefUseWebsocket;
boolean prefUseBluetooth;

float currentSpeedPercentage = 0; // speed as a value from 0-100

StaticJsonDocument<512> doc;

StrokeEngine Stroker;

static motorProperties servoMotor {  
  .maxSpeed = MAX_SPEED,                
  .maxAcceleration = 100000,      
  .stepsPerMillimeter = STEP_PER_MM,   
  .invertDirection = true,      
  .enableActiveLow = true,      
  .stepPin = SERVO_PULSE,              
  .directionPin = SERVO_DIR,          
  .enablePin = SERVO_ENABLE              
};

static machineGeometry strokingMachine = {
  .physicalTravel = PHYSICAL_TRAVEL,       
  .keepoutBoundary = KEEPOUT_TRAVEL      
};

static endstopProperties endstop = {
  .homeToBack = true,    
  .activeLow = true,          
  .endstopPin = SERVO_ENDSTOP,
  .pinMode = INPUT
};

static sensorlessHomeProperties sensorlessHome = {
  .currentPin = SERVO_SENSORLESS,
  .currentLimit = 1.5
};

void strokeEngineTelemetry(float position, float speed, bool clipping) {
  /* Serial.println("{\"position\":" + String(position * 10, 1) + 
    ",\"speed\":" + String(speed, 1) + 
    ",\"clipping\":" + String(clipping) + "}"); */
}

void homingNotification(bool isHomed) {
  if (isHomed) {
    Serial.println("Found home - Ready to rumble!");
    Stroker.moveToMax(HOMING_SPEED);
  } else {
    Serial.println("Homing failed!");
  }
  DynamicJsonDocument doc(200);
  JsonArray root = doc.to<JsonArray>();
  JsonObject versionInfo = root.createNestedObject();
  versionInfo["action"] = "home";
  versionInfo["success"] = isHomed;
  String jsonString;
  serializeJson(doc, jsonString);

  #if COMPILE_WEBSOCKET
    if (prefUseWebsocket || AUTO_START_BLUETOOTH_OR_WEBSOCKET) {
      WebsocketManager::sendMessage(jsonString);
    }
  #endif
  #if COMPILE_BLUETOOTH
    if (prefUseBluetooth) {
      BLEManager::sendNotification(jsonString);
    }
  #endif
  Serial.println(jsonString);
}

void updateSpeed () {
  // convert from 0-100% to min-max in mm, taking into account current stroke length
  float newSpeed = currentSpeedPercentage * (SPEED_UPPER_LIMIT - SPEED_LOWER_LIMIT) / 100 + SPEED_LOWER_LIMIT;
  float percentOfStroke = Stroker.getStroke() / Stroker.getMaxDepth(); // scale speed faster if stroke is smaller
  newSpeed /= percentOfStroke;
  Stroker.setSpeed(newSpeed, true);
}

void processCommand(DynamicJsonDocument doc) {
  JsonArray commands = doc.as<JsonArray>();
  for(JsonObject command : commands) {
    String action = command["action"];
    Serial.println("Action:");
    Serial.println(action);
    
    if (action.equals("move")) {
      int position = command["position"];
      int time = command["time"];
      boolean replace = command["replace"] == true;
      Stroker.appendToStreaming(position, time, replace);
    
    } else if (action.equals("startStreaming")) {
      Stroker.startStreaming();

    // homing
    } else if (action.equals("home")) {
      Stroker.disable();
      String type = command["type"];
      if (type.equals("auto")) {
        Stroker.enableAndHome(&endstop, homingNotification);
      } else if (type.equals("manual")) {
        Stroker.thisIsHome(HOMING_SPEED);
        homingNotification(true);
      } else if (type.equals("sensorless")) {
        Stroker.enableAndSensorlessHome(&sensorlessHome, homingNotification, HOMING_SPEED);
      }
    
    } else if (action.equals("configureWebsocket")) {
      
      String ssid =  command["ssid"];
      String password = command["password"];

      preferences.putString("ssid", ssid);
      preferences.putString("password", password);
      preferences.putBool("useWebsocket", true);
      preferences.putBool("useBluetooth", false);
      preferences.end();

      delay(1000);
      ESP.restart();

    } else if (action.equals("configureBluetooth")) {
      
      String bleName =  command["name"];

      preferences.putString("bleName", bleName);
      preferences.putBool("useWebsocket", false);
      preferences.putBool("useBluetooth", true);
      preferences.end();

      delay(1000);
      ESP.restart();

    } else if (action.equals("stop")) {
      Stroker.stopMotion();
      
    } else if (action.equals("setPattern")) {
      int pattern = command["pattern"]; // XToys currently only sets pattern 0 (which it expects to be a basic steady in/out stroke)
      Stroker.setPattern(pattern, true);
      Stroker.startPattern();

    } else if (action.equals("setSpeed")) {
      currentSpeedPercentage = command["speed"];
      updateSpeed();
    
    } else if (action.equals("setStroke")) {
      float stroke = command["stroke"];
      Stroker.setStroke(stroke / 100.0 * Stroker.getMaxDepth(), true);
      updateSpeed();
      
    } else if (action.equals("setDepth")) {
      float depth = command["depth"];
      Stroker.setDepth(depth / 100.0 * Stroker.getMaxDepth(), true);

    // not currently sent by XToys
    } else if (action.equals("setSensation")) {
      float sensation = command["sensation"];
      Stroker.setSensation(sensation, true);

    // not currently sent by XToys
    } else if (action.equals("setup")) {
      float speed = command["speed"] != NULL ? command["speed"] : 10.0;
      Stroker.setupDepth(speed, true);

    // not currently sent by XToys
    } else if (action.equals("retract")) {
      float speed = command["speed"] != NULL ? command["speed"] : 10.0;
      Stroker.moveToMin(speed);

    // not currently sent by XToys
    } else if (action.equals("extend")) {
      float speed = command["speed"] != NULL ? command["speed"] : 10.0;
      Stroker.moveToMax(speed);

    } else if (action.equals("connected")) {
      Stroker.disable();

    } else if (action.equals("disable")) {
      Stroker.disable();

    // print version JSON to any active connections
    } else if (action.equals("version")) {
      DynamicJsonDocument doc(200);
      JsonArray root = doc.to<JsonArray>();
      JsonObject versionInfo = root.createNestedObject();
      versionInfo["action"] = "version";
      versionInfo["api"] = API_VERSION;
      versionInfo["firmware"] = FIRMWARE_VERSION;
      String jsonString;
      serializeJson(doc, jsonString);

      #if COMPILE_WEBSOCKET
        if (prefUseWebsocket || AUTO_START_BLUETOOTH_OR_WEBSOCKET) {
          WebsocketManager::sendMessage(jsonString);
        }
      #endif
      Serial.println(jsonString);

    } else if (action.equals("getPatternList")) {
      // not implemented
    }
  }
};

void onToyMessage (String msg) {
  deserializeJson(doc, msg);
  processCommand(doc);
};

void setup() {
  Serial.begin(115200);

  // load prefs previously set via XToys website
  preferences.begin("ossm", false);
  prefUseWebsocket = preferences.getBool("useWebsocket", AUTO_START_BLUETOOTH_OR_WEBSOCKET);
  prefUseBluetooth = preferences.getBool("useBluetooth", AUTO_START_BLUETOOTH_OR_WEBSOCKET);

  Serial.println("Websocket Enabled: " + String(prefUseWebsocket));
  Serial.println("Bluetooth Enabled: " + String(prefUseBluetooth));

  #if COMPILE_WEBSOCKET
    if (prefUseWebsocket || AUTO_START_BLUETOOTH_OR_WEBSOCKET) {
      
      String ssid = preferences.getString("ssid", WIFI_SSID);
      String password = preferences.getString("password", WIFI_PSK);

      // Connect to WiFi
      Serial.println("Setting up WiFi");
      WiFi.begin(ssid.c_str(), password.c_str());
      while (WiFi.status() != WL_CONNECTED) {
          Serial.print(".");
          delay(500);
      }
      Serial.println("Connected.");
      Serial.print("IP=");
      Serial.println(WiFi.localIP());

      // Start websocket server
      WebsocketManager::setup(&onToyMessage);
    }
  #endif

  #if COMPILE_BLUETOOTH
    if (prefUseBluetooth) {
      String bleName = preferences.getString("bleName", BLE_NAME);
      BLEManager::setup(bleName, &onToyMessage);
      Serial.print("BLE=");
      Serial.println(bleName);
    }
  #endif

  // Set PWM output with 8bit resolution and 5kHz
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PWM, 0);
  ledcWrite(0, 0);

  // Setup Stroke Engine
  Stroker.begin(&strokingMachine, &servoMotor);
  Stroker.registerTelemetryCallback(strokeEngineTelemetry);

  // TEMP
  /* Stroker.thisIsHome();
  Stroker.startStreaming();
  Stroker.appendToStreaming(0, 2000);
  Stroker.appendToStreaming(50, 1000);
  Stroker.appendToStreaming(0, 1000);
  Stroker.appendToStreaming(100, 1000);
  Stroker.appendToStreaming(0, 1000); */
};

void loop() {

  #if COMPILE_WEBSOCKET
    if (AUTO_START_BLUETOOTH_OR_WEBSOCKET || prefUseWebsocket) {
      WebsocketManager::loop();
    }
  #endif

  #if COMPILE_SERIAL
    // read new commands from serial
    while (Serial.available()) {
      deserializeJson(doc, Serial);
      processCommand(doc);
    }
  #endif
};