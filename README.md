# OSSM Firmware for XToys

## Setting Up
1. Download the [OSSM-BLE.ino](https://github.com/denialtek/xtoys-ossm-firmware/blob/main/OSSM-BLE.ino) file.
2. Open the ino file in the Arduino IDE and upload the firmware to the OSSM ESP32.
3. In [XToys](https://xtoys.app) click the + button, add the OSSM toy, and click the bluetooth connect button.
4. In the setup dialog connect to the OSSM, enable the motor, and adjust the settings as desired.

## Available Settings
Settings can either be adjusted in the ino file or from within XToys. Changes made from XToys are stored in the ESP32 flash memory.

Available settings:
minSpeed:200 = set minimum speed of half-stroke to 200ms (used by XToys client)
maxSpeed:2000 = set maximum speed of half-stroke to 2000ms (used by XToys client)
maxOut:0 = set the position the stepper should stop at when moving in the one direction
maxIn:1000 = set the position the stepper should stop at when moving in the other direction
