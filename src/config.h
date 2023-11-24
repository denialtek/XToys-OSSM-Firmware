#pragma once

#define API_VERSION  "2.0"
#define FIRMWARE_VERSION  "2.0"

// Pin Definitions
#define SERVO_PULSE       14
#define SERVO_DIR         27
#define SERVO_ENABLE      26
#define SERVO_ENDSTOP     19
#define SERVO_ALARM       26
#define SERVO_SENSORLESS  36
#define PWM               21

// OSSM speed limits
#define HOMING_SPEED          25
#define SPEED_UPPER_LIMIT     200 // strokes per minute
#define SPEED_LOWER_LIMIT     0.5 // strokes per minute

// Only one of Bluetooth or Websockets can be active at once. Compiling and enabling both will break Websockets due to lack of heap space.
#define COMPILE_SERIAL        true
#define COMPILE_BLUETOOTH     true
#define COMPILE_WEBSOCKET     false

// Leave as true. If set to false the intent will be for XToys to enable one or the other modes as part of the firmware flash from the XToys website.
#define AUTO_START_BLUETOOTH_OR_WEBSOCKET true

// Websocket only config (Note: certificate data is in WebsocketManager.cpp)
#define WIFI_SSID "<fill in your network ssid>"
#define WIFI_PSK  "<fill in your network password>"
#define MAX_CLIENTS 1

// Bluetooth only config
#define BLE_NAME                     "OSSM"
#define SERVICE_UUID                 "e5560000-6a2d-436f-a43d-82eab88dcefd"
#define CONTROL_CHARACTERISTIC_UUID  "e5560001-6a2d-436f-a43d-82eab88dcefd"


// Calculation Aid:
#define STEP_PER_REV      2000      // How many steps per revolution of the motor (S1 off, S2 on, S3 on, S4 off)
#define PULLEY_TEETH      20        // How many teeth has the pulley
#define BELT_PITCH        2         // What is the timing belt pitch in mm
#define MAX_RPM           3000.0    // Maximum RPM of motor
#define STEP_PER_MM       STEP_PER_REV / (PULLEY_TEETH * BELT_PITCH)
#define MAX_SPEED         (MAX_RPM / 60.0) * PULLEY_TEETH * BELT_PITCH
#define PHYSICAL_TRAVEL   100.0
#define KEEPOUT_TRAVEL    5.0
#define MAX_DEPTH         int(0.5 + ((PHYSICAL_TRAVEL - (2 * KEEPOUT_TRAVEL))))