/**
 *  UPnP 12vLED Dimmer
 *  firmware for ESP8266 based homebuilt device
 *
 *  Copyright 2017 William C Barnes
 */
 // Initial configuration at 192.168.4.1:80 on open WLAN - SSID: wcbDimmerXXXXXX

#include <ESP8266WiFi.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <FS.h>

const char* version = "0.8";

#define PIN_BUTTON    14  // local control button - short press: on/off, long press adjusts dimming
#define PIN_LEVEL     13  // PWM output to LEDs - 0 (always low) for off, 1023 (always high) full on

#define PIN_RED       0   // Adafruit Huzzah red LED - on during setup and when button is pressed
#define PIN_BLUE      2   // ESP8266 module blue LED - flashing when configuration needed

ESP8266WebServer server(80);

String devname = String();    // SmartThings name, configuration SSID, and default hostname
String error = String();    // reported on serial interface and on configuration web page
const char* configFile = "/Dimmer.cfg";
//configuration parameters
// one per line in configuration file, no labels, just the strings
String ssid = String();        // line 1 - WLAN from which the hub is reachable
String password = String();    // line 2 - WLAN password (empty line if none)
String host = String();     // line 4 - hostname for this device

const char *statusWiFi[] = {
    "Idle",
    "SSID not found",
    "Scan completed",
    "Connected",
    "Incorrct Password",
    "Connection lost",
    "Disconnected"
};

// Device State
enum {FACTORY_RESET, NOT_ONLINE, WAITING_HUB, NORMAL, UPDATING, UPDATE_FAILED};
int devState = FACTORY_RESET;
IPAddress hubIP;                // valid iff
int hubPort = 0;                //    hubPort != 0
bool updateNeeded = false;
bool longPress = false;         // button is long-pressed (local control asserted)
// Dimmer State
bool ledOn = false;
int ledLevel = 100;     // 1 to 100 - exposed
int ledRate = 0;        // dim/brighten rate - 0=immediate
int targetPWM = 0;      // 0 to 1023 - internal

void sendDimmerRoot() {
  Serial.print(F("in sendDimmerRoot...  strlen(s) = "));
  char s[1200];
  sprintf_P( s, PSTR("\
    <!DOCTYPE HTML><html><head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <style>\
      body {font-family:\"Lato\",sans-serif;}\
      div.tabs {overflow:hidden;background-color:#f1f1f1;}\
      div.tabs button {background-color:inherit;float:left;padding: 14px 16px;font-size: 17px;}\
      div.tabs button:hover{background-color:#ddd;}\
      div.tabs button.active {background-color: #ccc;}\
    </style><script src='/script.js'></script>\
    </head><body onload='startup()'>\
    <p><b>WiFi 12volt LED Dimmer - version %s\
    <br>MAC address = %s</b>\
    <noscript><p>This page requires JavaScript which your browser is not providing.</noscript>\
    <div class='tabs'>\
      <button class='tab' onclick=\"tabHit(event, 'ctrl')\" id='defaultCard'>State</button>\
      <button class='tab' onclick=\"tabHit(event, 'cnfg')\">Config</button>\
      <button class='tab' onclick=\"tabHit(event, 'help')\">Help</button>\
    </div><div id='card'>/div></body></html>"),
    version, WiFi.macAddress().c_str());
  Serial.println(strlen(s));
  server.send(200, "text/html", s);
  Serial.println("  leaving sendDimmerRoot");
}

void sendJS() {
  Serial.println(F("in sendJS"));
  server.send(200, "application/javascript", PSTR("\
    function startup() {\n\
      document.getElementById('defaultCard').click();\n\
    }\n\
	  var refreshTmr;\n\
    function tabHit(evt, cardNm) {\n\
      var i, tab;\n\
      tab = document.getElementsByClassName('tab');\n\
      for (i = 0; i < tab.length; i++) {\n\
        tab[i].className = tab[i].className.replace(' active', \"\");\n\
      }\n\
      evt.currentTarget.className += ' active';\n\
      var xhttp = new XMLHttpRequest();\n\
      xhttp.onreadystatechange = function() {\n\
        if (this.readyState == 4 && this.status == 200) {\n\
          document.getElementById('card').innerHTML = this.responseText;\n\
          if (refreshTmr) { if (cardNm!='ctrl') {\n\
			      clearInterval(refreshTmr); refreshTmr = null; }\n\
          } else if (cardNm=='ctrl') {\n\
            refreshTmr = setInterval(setLED, 5000);  setLED(); }\n\
        }\n\
      };\n\
      xhttp.open('GET', 'card?cardNm='+cardNm, true);\n\
      xhttp.send();\n\
    }\n\
    function setLED(param='') {\n\
      var xhttp = new XMLHttpRequest();\n\
      xhttp.onreadystatechange = function() {\n\
        if (this.readyState == 4 && this.status == 200) {\n\
          // All 3 headers SHOULD always be there, but...\n\
          var x = this.getResponseHeader('Switch');\n\
          if (x) document.getElementById('switch').checked = (x == 'on');\n\
          x = this.getResponseHeader('Level');\n\
          if (x) {\n\
            var l = parseInt(x);\n\
            document.getElementById('slider').value = l;\n\
            document.getElementById('lvl').value = l;\n\
          }\n\
          x = this.getResponseHeader('Rate');\n\
          if (x) document.getElementById('rate').value = parseInt(x);\n\
        }\n\
      };\n\
      xhttp.open('GET', 'set'+param, true);\n\
      xhttp.send();\n\
    }\n\
  "));
  Serial.println("  leaving sendJS");
}

void sendConfigurationForm() {
  Serial.print(F("in sendConfigurationForm...  strlen(s) = "));
  char s[2048];
  sprintf_P(s, PSTR("<!DOCTYPE HTML><html><body><b>WiFi 12volt LED Dimmer - version %s\n\
    <br>MAC address = %s</b><p><i>%s</i><p>Please supply values and click CONFIGURE\n\
    <p><form method=GET action='config'>\n\
    <p>WiFi SSID - <input type='text' name='ssid' value='%s' size=32 maxlength=32>\n\
    <br>(required - 32 characters or less)\n\
    <p>WiFi Password - <input type='text' name='password' size=63 maxlength=63>\n\
    <br>(if required for the SSID - 63 characters or less)\n\
    <p>Hostname  - <input type='text' name='host' value='%s' size=32 maxlength=32>\n\
    <br>(optional - 32 characters or less)\n\
    <p><input type='submit' name='submit1' value='CONFIGURE'></form>\n\
    </body></html>"), version, WiFi.macAddress().c_str(), error.c_str(),
                      ssid.c_str(), (host.length()?host:devname).c_str());
  Serial.println(strlen(s));
  server.send(200, "text/html", s);
}

void handleConfiguration() {
  Serial.println(F("in handleConfiguration"));
  if (server.hasArg("ssid")&&server.hasArg("password")&&server.hasArg("host")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    host = server.arg("host");
    Serial.printf("ssid (%d char) = %s\npassword = %s\nhost = %s\n",
        ssid.length(), ssid.c_str(), password.c_str(), host.c_str());
    int x = ssid.length();
    if (x && x<=32 && password.length()<64 && host.length()<=32) {
      Serial.print(F("Opening configuration file for write..."));
      File f = SPIFFS.open(configFile, "w");
      Serial.println(f? F("OK"): F("Failed!"));
      if (f) {
        f.println(ssid);
        f.println(password);
        f.println(host);
        f.close();
        Serial.println(F("file written - Restarting"));
        server.sendHeader("Connection","close");
        server.send(200, "text/html", "<!DOCTYPE HTML><html><body><h3>RESTARTING</h3></body></html>");
        delay(200);
        server.stop();
        WiFi.softAPdisconnect(true);
        ESP.restart();
        Serial.println(F("I'm still here?"));
        return;
      }
      error = String(F("Failed to open file for write! - What the? - Try Factory Reset!"));
      Serial.println(error);
    } else error = String(F("SSID missing"));
  } else error = String(F("Missing parameter"));
  Serial.println(error);
  Serial.println(F("calling sendConfigurationForm"));
  sendConfigurationForm();
}

void handleUpdateFirmware() {
  Serial.println(F("in handleUpdateFirmware"));
  WiFiClient client;
  if (digitalRead(PIN_BUTTON) == LOW) {   // button is pressed - do update
    String url = server.arg("url");   // error/input check? XXXXXXXXXXXXXXXXXXXXXXX
    devState = UPDATING; // don't expect this to do anything XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    digitalWrite(PIN_RED, LOW);  // so turn on steady red LED during update instead
    server.send(200, "text/html", "<!DOCTYPE HTML><html><body><h3>UPDATING</h3></body></html>");
    Serial.print("Updating...");
    delay(200);
    ESPhttpUpdate.rebootOnUpdate(true);
    ESPhttpUpdate.update(client, url.c_str());
    // successful update will reset device and not get here
    devState = UPDATE_FAILED;
    Serial.println("FAILED");
    Serial.println(ESPhttpUpdate.getLastErrorString().c_str());
  } else server.send(200, "text/html",
	"<!DOCTYPE HTML><html><body><h3>Forbidden:  Dimmer button not pressed</h3></body></html>");
}

void handleCardRequest() {
  char s[2048];
  Serial.print(F("in handleCardRequest cardNm = "));
  String cardNm = server.arg("cardNm");
  Serial.print(cardNm);
  if (cardNm == "ctrl") {
    sprintf_P(s, PSTR("\
      <p><label>On: <input type='checkbox' id='switch'\n\
        onclick=\"setLED('?switch=' + (document.getElementById('switch').checked? 'on': 'off'))\"></label>\n\
      <p><label>level: <input type='range' id='slider'\n\
        onchange=\"setLED('?level=' + document.getElementById('slider').value)\"></label>\n\
        <output id='lvl' for='slider'></output>\n\
      <p><label>rate: <input type='number' id='rate' min='0' max='10'\n\
        oninput=\"setLED('?rate=' + document.getElementById('rate').value)\"></label>\n\
    "));
  } else if (cardNm == "cnfg") {
    sprintf_P(s, PSTR("<p>Please supply values and click CONFIGURE\n\
      <p><form method=GET action='config'>\n\
      <p>WiFi SSID - <input type='text' name='ssid' value='%s' size=32 maxlength=32>\n\
      <br>(required - 32 characters or less)\n\
      <p>WiFi Password - <input type='text' name='password' size=63 maxlength=63>\n\
      <br>(if required for the SSID - 63 characters or less)\n\
      <p>Hostname  - <input type='text' name='host' value='%s' size=32 maxlength=32>\n\
      <br>(optional - 32 characters or less)\n\
      <p><input type='submit' name='submit1' value='CONFIGURE'></form>\n\
      <p><p>OR enter the location of firmware below\n\
      <br>and click UPDATE <b>while pressing dimmer button</b>.\n\
      <p><form method=GET action='updatefirmware'>\n\
      <p>URL - <input type='text' name='url' size=72 maxlength=256>\n\
      <br>(eg: http://server:port/path/file.bin )\n\
      <p><input type='submit' name='submit2' value='UPDATE'></form>\n\
      </body></html>"), ssid.c_str(), (host.length()?host:devname).c_str());
  } else if (cardNm == "help") {
    sprintf(s,"Hello World help");
  } else {
    server.send(400, "text/html", "<!DOCTYPE HTML><html><body><h3>Bad Request!</h3></body></html>");
    Serial.println(F("   Bad Request"));
    return;
  }
  Serial.print(F("   strlen = "));
  Serial.println(strlen(s));
  server.send(200, "text/html", s);
}

bool parseHubAddr(const String ha, IPAddress ip, int& port){
  // Set ip and port from ha - returns true on success
  const char *s = ha.c_str();
  int x;
  for (int i = 0; i <= 4; i++,s++) {
    x = 0;
    while (*s>='0'&&*s<='9') x = 10*x + *s++ - '0';
    if (i<4 && x>255) return false;
    else ip[i] = (byte) x;
    if (i<3 && *s!='.') return false;
    if (i==3 && *s!=':') {
      if (!*s) {
        Serial.println(F("  parseHubAddr OK - assumes port 80"));
        port = 80;
        return true;
      } else return false;
    }
  }
  if (*--s || !x || x>65535) return false;
  port = x;
  Serial.println(F("  parseHubAddr OK"));
  return true;
}
 
void setTargetPWM() { targetPWM = (ledOn && ledLevel>0) ? map(ledLevel,1,100,1,1023) : 0; }

void handleSet() {	// execute command from hub or web interface
  Serial.println(F("in handleSet"));
  String reqSwitch = server.arg("switch");
  String reqLevel = server.arg("level");
  String reqRate = server.arg("rate");
  Serial.print(F(" reqLevel=")); Serial.print(reqLevel);
  Serial.print(F(" reqSwitch=")); Serial.print(reqSwitch);
  Serial.print(F(" reqRate=")); Serial.println(reqRate);
  bool changed = false;
  if (reqRate.length()) {
    int rate = atoi(reqRate.c_str());
    if (rate != ledRate) {
      ledRate = rate;
      changed = true;
    }
  }
  if (!longPress) {
    int level = ledLevel;
    bool on = ledOn;
    if (reqLevel.length()) {
      level = atoi(reqLevel.c_str());
      on = level? true: false;
    }
    if (reqSwitch.length()) {
      on = (reqSwitch == "on");
      if (on && !level) level = 100;
    }
    if (level != ledLevel || on != ledOn) {
      ledLevel = level;
      ledOn = on;
      changed = true;
    }
  }
  IPAddress ip;
  int port;
  if (server.hasArg("hubAddr") && parseHubAddr(server.arg("hubAddr"),ip,port)) {
    Serial.print(F("  hubAddr="));
    Serial.println(server.arg("hubAddr"));
    devState = NORMAL;
    hubIP = (server.client()).remoteIP();
    hubPort = port;
    updateNeeded = false;         // taken care of by response below
    Serial.print(F("  hub="));
    Serial.print(hubIP);
    Serial.print(F(":"));
    Serial.println(hubPort);
  } else {    // set request from other than hub
    if (changed) updateNeeded = true;
  }
  if (changed) {
    setTargetPWM();
    Serial.print(F(" ledOn=")); Serial.print(ledOn);
    Serial.print(F(" ledLevel=")); Serial.print(ledLevel);
    Serial.print(F(" ledRate=")); Serial.print(ledRate);
    Serial.print(F(" targetPWM=")); Serial.println(targetPWM);
} else {Serial.print(F(" no change!"));}
  // response
  server.sendHeader("Connection","close");
  server.sendHeader("Switch",ledOn?"on":"off");
  server.sendHeader("Level",String(ledLevel).c_str());
  server.sendHeader("Rate",String(ledRate).c_str());
  server.send(200, "text/plain", "");
  server.client().stop();
}

void handleNotFound() {
  Serial.println(F("handleNotFound:"));
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}

void updateHub() {
  WiFiClient c;
  Serial.print(F("in updateHub  hub="));
  Serial.print(hubIP); Serial.print(F(":")); Serial.println(hubPort);
  Serial.print(F(" ledOn=")); Serial.print(ledOn);
  Serial.print(F(" ledLevel=")); Serial.print(ledLevel);
  Serial.print(F(" ledRate=")); Serial.println(ledRate);
	if (hubPort != 0) {
		if (c.connect(hubIP,hubPort)) {
			// send status
			c.println(F("NOTIFY / HTTP/1.1"));
			c.print(F("HOST: ")); c.print(hubIP); c.print(F(":")); c.println(hubPort);
			c.print(F("Switch: ")); c.println(ledOn?F("on"):F("off"));
			c.print(F("Level: ")); c.println(ledLevel);
			c.print(F("Rate: ")); c.println(ledRate);
			c.println(F("CONTENT-LENGTH: 0"));
			c.println();
			c.println();
			Serial.println(F("  Hub updated - Responds:"));
			//wait for response
			unsigned long startMS = millis();
      while (c.available() == 0) {
        if (millis() > startMS + 1000) {
          devState = WAITING_HUB;
          Serial.println(F("  no response from Hub"));
          c.stop();
          return;
        }
        delay(1);
      }
      while (c.available()) {
        char ch = static_cast<char>(c.read());
        Serial.print(ch);
      }
      Serial.println();
			c.stop();
		} else {
			devState = WAITING_HUB;
			Serial.println(F("  failed: couldn't connect"));
		}
	} else {
    Serial.print(F(" failed: devStste=")); Serial.println(devState);
  }
}

void setup() {	
  analogWrite(PIN_LEVEL, 0);   // set output off
  devname = String("wcbDimmer")+String(ESP.getChipId(),16);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();    // need WiFi disconnected to set host name
  Serial.begin(115200);
  Serial.println();
  Serial.printf("//\n//  WiFi 12volt LED Dimmer - Version %s\n//", version);
  Serial.println();
  pinMode(PIN_BLUE, OUTPUT); digitalWrite(PIN_BLUE, LOW);	// blue LED on during format
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  if (digitalRead(PIN_BUTTON)==LOW) {
    Serial.println(F("Button pressed - Factory Reset"));
    SPIFFS.format();
  }
  pinMode(PIN_RED, OUTPUT); digitalWrite(PIN_RED, LOW);	// red LED on during setup
  digitalWrite(PIN_BLUE, HIGH);	// blue LED off during setup
  Serial.print(F("Starting File System..."));
  Serial.println(SPIFFS.begin()? F("OK"): F("Failed!"));
  error = String("");   // no error
  Serial.print(F("Opening configuration file for read..."));
  File f = SPIFFS.open(configFile, "r");
  Serial.println(f? F("OK"): F("Failed!"));
  if (f) {
    ssid = String(f.readStringUntil('\n')); ssid.trim();
    password = String(f.readStringUntil('\n')); password.trim();
    host = String(f.readStringUntil('\n')); host.trim();
    f.close();
    // no need to check these values - they came from the file we wrote. (hopefully)
    Serial.printf("ssid (%d char) = %s\npassword = %s\nhost = %s\n",
        ssid.length(), ssid.c_str(), password.c_str(), host.c_str());
    WiFi.hostname(host);
    // TODO add reconnection code?  XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    Serial.println(F("connecting"));
    WiFi.mode(WIFI_STA);
    if (password == "") WiFi.begin(ssid.c_str());         // not tested! (for success)
    else WiFi.begin(ssid.c_str(),password.c_str());
    int status = WiFi.waitForConnectResult();
    if (status == WL_CONNECTED) {
      Serial.print(F("Connected - IP address: "));
      Serial.println(WiFi.localIP());
      Serial.println(F("Successful initialization"));
      devState = WAITING_HUB;
    } else {
      error = String(statusWiFi[status]);
      devState = NOT_ONLINE;
    }
  } else {
    Serial.println(F("Failed to open config file"));
    error = String(F("Please enter WLAN credentials"));
    ssid = String();
    password = String();
    host = String();
    devState = FACTORY_RESET;
  }
  if (error.length()) {
    Serial.println(F("Configuration required"));
    Serial.println(error);
    WiFi.disconnect(true);
    // Create a WiFi access point and provide a web server on it.
    Serial.println(F("Starting configuration access point"));
    WiFi.mode(WIFI_AP);
    WiFi.softAP(devname.c_str());
    Serial.print(F("configuration server at "));
    Serial.println( WiFi.softAPIP() );
    // configure and start http server
    server.on("/", HTTP_GET, sendConfigurationForm);
    server.on("/config", HTTP_GET, handleConfiguration);
    server.onNotFound(handleNotFound);
    Serial.println(F("Starting server"));
    server.begin();
  } else {  // no error - ready for SmartThings hub
    // configure and start http server
    server.on("/", HTTP_GET, sendDimmerRoot);
    server.on("/script.js", HTTP_GET, sendJS);
    server.on("/card", HTTP_GET, handleCardRequest);
    server.on("/config", HTTP_GET, handleConfiguration);
    server.on("/updatefirmware", HTTP_GET, handleUpdateFirmware);
    server.on("/description.xml", HTTP_GET, [](){
      WiFiClient c = server.client();
      Serial.print(F("Description request - Remote:"));
      Serial.print( c.remoteIP() );
      Serial.print(F(":"));
      Serial.print( c.remotePort() );
      Serial.print(F("  Local:"));
      Serial.print( c.localIP() );
      Serial.print(F(":"));
      Serial.println( c.localPort() );
      SSDP.schema(c);
    });
    server.on("/set", HTTP_GET, handleSet);
    server.onNotFound(handleNotFound);
    Serial.println(F("Starting server"));
    server.begin();
    Serial.println(F("Starting SSDP"));
    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setDeviceType("urn:schemas-upnp-org:device:DimmableLight:1");
    SSDP.setName(devname);
    SSDP.setSerialNumber(WiFi.macAddress());
    //SSDP.setURL("index.html");
    SSDP.setModelName("uPnP 12Volt LED Dimmer");
    //SSDP.setModelURL("index.html");
    SSDP.setManufacturer("WCBarnes Homebuilt");
    //SSDP.setManufacturerURL("index.html");
    SSDP.begin();
  }
  Serial.println(F("exiting setup()"));
}

void handleButton() {
  enum { DEBOUNCE=5,      // minimal milliseconds the button must be closed
        LONG_PRESS=250    // milliseconds for a long press
        };
  static int saveRate = 0;                // storage for ledRate during long press
  static unsigned long startMillis = 0;   // if 0 then local control button is not pressed
  static boolean brighten = false;        // long press brightens else dims.
                           // toggled at end of long press or reaching limits (0-99)

  if (digitalRead(PIN_BUTTON)==HIGH) {    //button is released
    if (startMillis) {                    // it was just released
      digitalWrite(PIN_RED,HIGH);
      if (longPress) {                                 //long press ends
        longPress = false;
        brighten = !brighten;
        ledRate = saveRate;
        updateHub();   // send level to hub
      } else if (millis()-startMillis >= DEBOUNCE) {   // short press ends
        // toggle switch on-off
        ledOn = !ledOn;
        setTargetPWM();
        updateHub();   // send level to hub
      }
    }
    startMillis = 0;
  } else {    // button is pressed
    if (!startMillis) {   // press starting now
      startMillis = millis();
      digitalWrite(PIN_RED,LOW);
    } else {    // press continues
      if (millis()-startMillis >= LONG_PRESS) {
        if (!longPress) {
          longPress = true;
          ledOn = true;
          saveRate = ledRate;
          ledRate = 0;
        }
        startMillis = millis();
        // adjust level
        // implement an exponential curve for level adjustment
        int delta = ledLevel/6 + 1;  // should result in ~16 steps for full scale
        ledLevel += brighten? delta : -delta;
        if (ledLevel < 1) { ledLevel = 1; brighten = true; }
        else if (ledLevel > 100) { ledLevel = 100; brighten = false; }
        setTargetPWM();
      }
    }
  }
}

void adjustPWM() {
  static int currentPWM = 0;
  static unsigned long lastMillis = 0;
  int diff;
  if ((diff = targetPWM - currentPWM) != 0) {
    if (!ledRate) {
      currentPWM = targetPWM;
    }
    else if (millis()>=lastMillis + 20) {
      lastMillis = millis();
      int a = 1; a = a<<(ledRate-1);
      int delta = currentPWM/a + 1;   // exponential curve again
      if ( delta >= (diff>0? diff: -diff) ) {
        delta = diff;
        lastMillis = 0;
      } else if (diff<0) delta = -delta;
      currentPWM += delta;
    }
    analogWrite(PIN_LEVEL, currentPWM);
  }
}

void setStatusLEDs() {
	static int lastSixteenths = 0;
	enum {
	FLASH1EVERY32SEC = 0x1FF,   // or rather every 32*1024/1000 seconds to be precise
    FLASH1EVERY4SEC = 0x3F,
    FLASH1EVERY2SEC = 0x1F,
    FLASH2EVERY2SEC = 0x1D,
    FLASH4EVERY2SEC = 0x1B,
    FLASH1EVERYHALFSEC = 0x7,
    FLASH2EVERYHALFSEC = 0x5
	};
	int sixteenths = (int)(millis()>>6);
	if (sixteenths!=lastSixteenths && digitalRead(PIN_BUTTON)==HIGH) {
		lastSixteenths = sixteenths;
		int red = HIGH;
		int blue = HIGH;
		switch (devState) {
		  case NORMAL:
			break;
    case WAITING_HUB:
			if (!(sixteenths&FLASH1EVERY4SEC)) red = LOW;
			break;
		case NOT_ONLINE:
			if (!(sixteenths&FLASH2EVERY2SEC)) blue = LOW;
			break;
		case FACTORY_RESET:
			if (!(sixteenths&FLASH1EVERY2SEC)) blue = LOW;
      break;
    case UPDATING:
			if (!(sixteenths&FLASH1EVERYHALFSEC)) red = LOW;
      break;
    case UPDATE_FAILED:
			if (!(sixteenths&FLASH2EVERYHALFSEC)) red = LOW;
      break;
		}
		digitalWrite(PIN_RED, red);
		digitalWrite(PIN_BLUE, blue);
	}
}

void loop() {
  server.handleClient();
  handleButton();
  adjustPWM();
  setStatusLEDs();
}
