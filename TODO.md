BUGS
----
- hub (smart app upnp service manager) isn't refreshing devices.  
  FIXED 2017/10/14
- device starting up unconfigured flashes "cannot connect" instead of "factory reset"  
  FIXED 2017/10/15
- maxlength in firmware URL form  
  FIXED 2017/10/15

PROPOSED
--------
- change device operation to match Leviton (and my earlier ESP1 ST_Anywhere) devices.  
ie: When level is set, set switch accordingly. (switch = level? on : off)  
DONE 2017/10/15
- Don't fill in password field in device configuration form  
DONE 2017/10/15
- Don't disconnect WiFi at every startup
- improve device web UI.  status?  control?  help?  link to GitHub repo?
- Change uuid base back to ESP8266SSDP standard (so ESP8266SSDP.cpp need not be modified)