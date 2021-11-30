# OSSM Firmware for XToys
Firmware for use with the OSSM (Open Source Sex Machine) by KinkyMakers.
[https://github.com/KinkyMakers/OSSM-hardware](https://github.com/KinkyMakers/OSSM-hardware)

## Setting Up
1. Download the [OSSM-BLE.ino](https://github.com/denialtek/xtoys-ossm-firmware/blob/main/OSSM-BLE.ino) file.
2. Open the ino file in the Arduino IDE and upload the firmware to the OSSM ESP32.
3. In [XToys](https://xtoys.app) click the + button, add the OSSM toy, and click the bluetooth connect button.
4. In the setup dialog connect to the OSSM, enable the motor, and adjust the settings as desired.

## Bluetooth Info
This firmware uses the following Bluetooth info:  
Device Identifier: OSSM  
Service: e556ec25-6a2d-436f-a43d-82eab88dcefd  
Control Characteristic: c4bee434-ae8f-4e67-a741-0607141f185b (used for sending T-Code commands)  
Settings Characteristic: fe9a02ab-2713-40ef-a677-1716f2c03bad (used for adjusting OSSM settings)  

## Control Commands
Supported T-Code commands to send to Control characteristic:  
DSTOP - Stop OSSM immediately (leaves motor enabled)  
DENABLE - Enables motor (non-standard T-Code command)  
DDISABLE - Disable motor (non-standard T-Code command)  
L199I100 - Linear actuator move commands with a position + speed  
L199I100C - Appending 'C' to a command causes the OSSM to clear any pending movement commands (non-standard T-Code command)

## Settings Commands
Supported settings commands to send to Settings characteristic:  
minSpeed:200 - set minimum speed of half-stroke to 200ms (used by XToys client)  
maxSpeed:2000 - set maximum speed of half-stroke to 2000ms (used by XToys client)  
maxOut:0 - set the position the stepper should stop at when moving in the one direction  
maxIn:1000 - set the position the stepper should stop at when moving in the other direction

Changes made via these settings commands are stored in the ESP32 flash memory. Alternatively the default settings can be adjusted in the ino file. 

Reading from the Settings characteristic will return a comma separated list of the current settings.
