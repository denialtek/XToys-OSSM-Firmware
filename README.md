Firmware for controlling the [OSSM (Open Source Sex Machine)](https://github.com/KinkyMakers/OSSM-hardware) via the [XToys.app](https://xtoys.app) website.

The firmware enables the OSSM to be controlled via serial, Bluetooth or websocket commands.

Uses a [slightly modified version](https://github.com/denialtek/StrokeEngine) of the [StrokeEngine](https://github.com/theelims/StrokeEngine) library to interact with the OSSM.

# Automatic Setup

The firmware can be installed directly from the XToys website via these steps:
1. Connect your OSSM via USB cable.
2. Load the OSSM block in XToys and click the grey connect button.
3. Select the download icon beside the v2.0 label and follow the install steps.

# Manual Setup

1. On your OSSM motor set the DIP switches to:  
![S1=Off, S2=On, S3=On, S4=Off, S6=Off](ossm-dip.png)  
**Note:** If the dildo is mounted on the left side of the OSSM (the side with the USB port) then flip S6 to ON instead
2. Connect your OSSM via USB cable.
3. Download and extract the repository.
4. Install [Visual Studio Code](https://code.visualstudio.com) and the [PlatformIO](https://platformio.org/platformio-ide) extension.
5. In Visual Studio go to File > Open and select the XToys-OSSM firmware folder.
6. Open the config.h file (OSSM > src > config.h).

If you want to use Bluetooth fill out these fields:  
#define COMPILE_BLUETOOTH true  
#define COMPILE_WEBSOCKET false  
#define BLE_NAME "OSSM"

If you want to use Websockets fill out these fields:  
#define COMPILE_BLUETOOTH false  
#define COMPILE_WEBSOCKET true  
#define WIFI_SSID "your network ssid"  
#define WIFI_PSK "your network password"

If you use non-standard pins on the ESP32 this can also be adjusted here.

6. Click the PlatformIO icon in the left side bar and then click the Upload button (Project Tasks > esp32dev > Upload).

**NOTE: The firmware will not work if both COMPILE_BLUETOOTH and COMPILE_WEBSOCKET are set to true. Only use one or the other.**

# Command Structure

XToys sends JSON messages over Bluetooth, serial or websocket depending on what connection method the user chose in XToys.

The JSON is always an array of objects where each object contains an "action" key as well as additional values as needed.

ex.  

    [
        { "action": "startStreaming" },
        { "action": "move", "position": 100, "time": 200 },
        { "action": "move", "position": 0, "time": 0 }
    ]

# Supported Commands

| XToys Event                                               | Action Name        | Sample JSON                                                                                                                                                                                    |
|-----------------------------------------------------------|--------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| On Connect                                                | connected          | {<br>"action": "connected"<br>}                                                                                                                                                                |
| On homing selected                                        | home               | {<br>"action": "home",<br>"type": "auto" // or "manual"<br>}                                                                                                                                   |
| On Speed mode selected                                    | setPattern         | {<br>"action": "setPattern",<br>"pattern": 0 // XToys only ever sends pattern 0<br>}                                                                                                           |
| On speed set to 0                                         | stop               | {<br>"action": "stop"<br>}                                                                                                                                                                     |
| On speed set to >0                                        | setSpeed           | {<br>"action": "setSpeed",<br>"speed": 50 // 0-100<br>}                                                                                                                                        |
| On upper stroke length changed                            | setDepth           | {<br>"action": "setDepth",<br>"depth": 50 // 0-100<br>}                                                                                                                                        |
| On lower stroke length changed                            | setStroke          | {<br>"action": "setStroke",<br>"stroke": 50 // 0-100<br>}                                                                                                                                      |
| On Position mode selected                                 | startStreaming     | {<br>"action": "startStreaming"<br>}                                                                                                                                                           |
| On position changed (manually or via pattern)             | move               | {<br>"action": "move",<br>"position: 50, // 0-100<br>"time": 500, // ms<br>"replace": true // whether to wipe the existing pending position commands (ex. when user has changed patterns)<br>} |
| On manual disconnect or homing cancelled                  | disable            | {<br>"action": "disable"<br>}                                                                                                                                                                  |
| On firmware flash complete and Bluetooth being configured | configureBluetooth | {<br>	"action": "configureBluetooth",<br>	"name": "OSSM" // desired Bluetooth name<br>}                                                                                                          |
| On firmware flash complete and Websocket being configured | configureWebsocket | {<br>	"action": "configureWebsocket",<br>	"ssid": "MyNetwork",<br>	"password": "mysecretpassword"<br>}                                                                                            |
| On connect and version being checked                      | version            | {<br>	"action": "version"<br>}                                                                                                                                                                  |


Actions that are in the OSSM firmware code but are not sent via XToys currently:
- "getPatternList"
- "setup"
- "retract"
- "extend"
- "setSensation"
