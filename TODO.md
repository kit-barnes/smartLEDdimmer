BUGS
----
- hub (smart app upnp service manager) isn't refreshing devices.  
  FIXED 2017/10/14
- device starting up unconfigured flashes "cannot connect" instead of "factory reset"  
  FIXED 2017/10/15
- maxlength in firmware URL form  
  FIXED 2017/10/15
- Don't show Firmware Update when on configuration LAN
- main LED flickering when downloading during update?
- LED state is ON after power-up but not reported to hub
- Occasional multi-second delay on short tap after long period of inactivity
- Label rate slider in SmartThings app
- updateHub() WAS working but now gets stuck sometime after sending message?
  ? Broken by Samsung update early (March?) 2019.

PROPOSED
--------
- change device operation to match Leviton (and my earlier ESP1 ST_Anywhere) devices.  
  ie: When level is set, set switch accordingly. (switch = level? on : off)  
  and when switch is set on, if level is 0, set level to 100.  
  DONE 2017/10/15
- Don't fill in password field in device configuration form  
  DONE 2017/10/15
- Don't disconnect WiFi at every startup  
  Consider if, after power outage, WiFi takes longer to start than dimmer.
- improve device web UI
  - status
    DONE 2017/10/25
  - control
    DONE 2017/10/25
  - help?  link to GitHub repo?
  - hub link status
- SET request must be in proper format - else return NOT FOUND.
  More assurance on hub (wcb handler) recognition.
- Change uuid base back to ESP8266SSDP standard (so ESP8266SSDP.cpp need not be modified)
- Change device rate curve to be quicker at the bright end.
- Configuration requires button press
- Update ReadMe to finish SmartThings configuration and document OTA firmware update
