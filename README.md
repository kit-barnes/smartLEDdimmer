uPnP12vLEDdimmer
================
SmartThings Hub connected device controlling a monochrome 12 volt LED strip.

Includes
--------
- Schematic, and Firmware for an ESP8266 Arduino based device
  - WiFi connection to SmartThings Hub on local LAN
  - Local control of ON/OFF and dim level by a pushbutton switch
  - selectable dim/brighten rate
  - OTA firmware update enabled
- SmartThings code for device handler and uPnP Service Manager smart app
- SketchUp files for 3D printed case and switch mount

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
The Adafruit Huzza ESP8266 breakout module has 2 LEDS (one red and one blue)
which are used to indicate device status. The red one will be lit during startup and
whenever the button is pressed.

- During firmware update - Red LED flashing once every half-second
- If firmware update fails - Red LED flashing twice every half-second
- During Factory Reset - Blue LED lit (for about 30 seconds)
- Following Factory Reset (unconfigured) - Blue LED flashing once every 2 seconds
- Configured but not connected to WiFi - Blue LED flashing twice every 2 seconds
- Connected but not communicating with hub - Red LED flashing once every 4 seconds



Initial Firmware Load
---------------------
The firmware is an Arduino sketch and can be installed
by following instructions on the Adafruit website at  
http://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide

I do not solder a header onto the Huzzah for the load.
Rather I plug one into the FTDI cable, plug the resulting short pins
into the Huzzah, and hold it there with slight angular pressure
while I load the firmware.  
Note: While loading, 12V supply is disconnected - Huzzah is powered by FTDI cable.


WiFi Configuration
------------------
The dimmer needs to be configured to connect to your Wi-Fi LAN.

- Factory Reset the device by pressing the push button for a few seconds
  as you apply power. The blue LED should light and remain lit for about 30 seconds
  then start flashing once every two seconds.
- Connect your computer or phone to the dimmer's configuration LAN: WiFi SSID **wcbDimmerXXXXXX**
  (**XXXXXX** will be the last 6 hexadecimal digits of the dimmer's MAC address)
- Browse to http://192.168.4.1 (The dimmer's address on wcbDimmerXXXXXX)
- Enter your WiFi SSID and password (if required) and hostname (defaults to wcbDimmerXXXXXX)
  then click **CONFIGURE**.  Dimmer will restart and attempt to connect to your WiFi.

SmartThings Setup
-----------------
- On the web, sign into the SmartThings developer IDE and create device handler and service manager.
- In the SmartThings mobile app, click **Add a SmartApp** and select **WCB uPnP Service Manager**
  from the **My Apps** category.
- (to be continued after improving the manager code)