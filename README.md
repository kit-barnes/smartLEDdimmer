smartLEDdimmer
================
A 12v LED dimmer-switch - controlled via push-button switch, WiFi, or SmartThings.

Includes
--------
- Schematic, and Firmware for an ESP8266 Arduino based device
  - WiFi connection to SmartThings Hub on local LAN
  - Local control of ON/OFF and dim level by a pushbutton switch
  - selectable dim/brighten rate
  - OTA firmware update enabled
- SmartThings code for device handler and uPnP Service Manager smart app
- SketchUp files for 3D printed case and switch mount

Development and Build Environment
---------------------------------
Original environment was Arduino IDE with ESP8266 (https://github.com/esp8266/Arduino).
Now using Git and PlatformIO IDE (https://platformio.org/platformio-ide)
with the Espressif 8266 platform.

Still just using the SmartThings Groovy IDE (https://graph.api.smartthings.com/)
and NotePad++ with Groovy definitions
(http://notepad-plus.sourceforge.net/commun/userDefinedLang/userDefineLang_Groovy.xml)
and Git for the SmartThings code.

Initial Firmware Load
---------------------
The firmware is an Arduino sketch and can be installed
by following instructions on the Adafruit website at  
http://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide
(or by using PlatformIO's Upload facility).

I do not solder a header onto the Huzzah to load the firmware.
Rather I plug one into the FTDI cable, 'plug' the resulting short pins
into the Huzzah, and hold it there with slight angular pressure
while loading the firmware.  
Note: While loading, 12V supply is disconnected - Huzzah is powered by FTDI cable.

Local Operation
---------------
Tap the pushbutton switch to toggle the LED strip on and off.  
Press and hold the pushbutton to continuously cycle the strip
up and down through all brightness levels.
Alternate presses reverse the cycle direction - If you release the switch
while the strip is brightening, the next press will start dimming it.

The two switches on the Adafruit Huzzah are not used except for initial
firmware load.

Status LEDs
-----------
The Adafruit Huzza ESP8266 breakout module has 2 LEDs (one red and one blue)
which are used to indicate status. The red one will be lit during startup and
whenever the button is pressed.

- During firmware update - Red LED flashing once every half-second
- If firmware update fails - Red LED flashing twice every half-second
- During Factory Reset - Blue LED lit (for about 30 seconds)
- Following Factory Reset (unconfigured) - Blue LED flashing once every 2 seconds
- Configured but not connected to WiFi - Blue LED flashing twice every 2 seconds
- Connected but not communicating with hub - Red LED flashing once every 4 seconds

WiFi Configuration
------------------
The dimmer needs to be configured to connect to your Wi-Fi LAN.

- Factory Reset the device by pressing the push button switch for a few seconds
  as you apply power. The blue LED should light and remain lit for about 30 seconds
  then start flashing once every two seconds.
- Connect your computer or phone to the dimmer's configuration LAN:
  WiFi SSID **wcbDimmerXXXXXX**
  (**XXXXXX** will be the last 6 hexadecimal digits of the dimmer's MAC address)
- Browse to http://192.168.4.1 (The dimmer's address on wcbDimmerXXXXXX)
- Enter your WiFi SSID and password (if required) of your WiFi network
  and the hostname for this dimmer (defaults to wcbDimmerXXXXXX)
  then click **CONFIGURE**.  Dimmer will restart and attempt to connect to your WiFi.
- If it connects, the red LED should start to flash once every 4 seconds.
  If instead, the blue LED starts flashing twice every 2 seconds,
  the dimmer was unable to connect to your WiFi.  Reconnect to **wcbDimmerXXXXXX**
  and browse to http://192.168.4.1 to see the error message and try again.
- Once the dimmer is connected to your WiFi, you can discover its IP address
  with a LAN discovery tool.

WiFi Operation
--------------


SmartThings Setup
-----------------
- On the web, sign into the SmartThings developer IDE and create device handler and service manager.
  You can copy and paste the code into the IDE's editors, then publish them both "for me".
- In the SmartThings mobile app, click **Add a SmartApp** and select **WCB Dimmer Manager**
  from the **My Apps** category.
- Wait for the manager to discover your device(s). If discovery takes more than a few minutes
  make sure your dimmer is connected to the same LAN as your hub).
- Select your device(s), tap Done and Save. Your device(s) will be added into your **My Home**
  where you can rename and select an appropriate icon for them.
- It is NOT possible to remove individual devices - you must remove the service manager using the
  SmartThings mobile app (which will remove them all) then reinstall any you wish to keep.
  To remove the service manager, you must first remove all the dimmer devices
  from any smartapps and cards or panels in which they may used.