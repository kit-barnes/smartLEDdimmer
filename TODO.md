BUGS
----
- hub (smart app upnp service manager) isn't refreshing devices.  
  FIXED 2017/10/14
- device starting up unconfigured flashes "cannot connect" instead of "factory reset"  
  FIXED 2017/10/15
- maxlength in firmware URL form  
  FIXED 2017/10/15
- Don't show Firmware Update when on configuration LAN
  FIXED 2019/4/12 v0.6
- main LED flickering when downloading during update?
- LED state is ON after power-up but not reported to hub
  Fixed 2019/5/12 v0.7 ? (don't recall fixing it but no longer observed)
- Locks up when button pushed!
  Fixed 2019/4/1 v0.6
  Version 0.5 (2017/10) was working fine in two devices until early 2019.
  Samsung must have changed something.  Rushed version 0.6 (not fully tested)
  into one device and then discovered that while the button, the smartThings
  interface, and the configuration pages were working,
  the rest of the web interface was not.
  Fixed 2019/5/12 v0.7

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
  REJECTED - Need WiFi off to set hostname
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
  DONE 2019/5/12 v0.7
- Change device rate curve to be quicker at the bright end.
- Configuration requires button press
  DONE 2019/4/1 v0.6 (kinda - removed from regular UI)
  Now only available when Factory Reset.
- Retry updateHub on fail and updateHub on set from not-hub.
  DONE 2019/5/24 v0.8alpha
- Revisit Hub update logic - Currently when switch is off and hub adjusts level,
  device adjusts level and tuns on switch but hub thinks switch is still off.
