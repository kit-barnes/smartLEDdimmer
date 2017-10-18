/**
 *  SmartThings Device Driver for WCB homebuilt UPnP 12vLED Dimmer
 *
 *  Copyright 2017 William C Barnes
 *
 */
metadata {
	definition (name: "WCB 12vLED Dimmer", namespace: "kit-barnes", author: "William C Barnes") {
	
		capability "Actuator"
		capability "Sensor"
		capability "Light"
		capability "Switch"
		capability "Refresh"
		capability "Switch Level"
		
		attribute "rate", "number"
		
		command "setRate", ["number"]
	}

	simulator {
		// TODO: define status and reply messages here
	}

	tiles(scale: 2) {
		multiAttributeTile(name:"switchLevelTile", type: "lighting", width: 6, height: 4, canChangeIcon: true){
			tileAttribute ("device.switch", key: "PRIMARY_CONTROL") {
				attributeState "off", label: '${name}', action: "switch.on", icon: "st.switches.switch.off", backgroundColor: "#ffffff", nextState:"turningOn"
				attributeState "on", label: '${name}', action: "switch.off", icon: "st.switches.switch.on", backgroundColor: "#00A0DC", nextState:"turningOff"
				attributeState "turningOn", label:'${name}', action:"switch.off", icon:"st.switches.switch.on", backgroundColor:"#00A0DC", nextState:"turningOff"
				attributeState "turningOff", label:'${name}', action:"switch.on", icon:"st.switches.switch.off", backgroundColor:"#ffffff", nextState:"turningOn"
			}
   			tileAttribute ("device.level", key: "SLIDER_CONTROL", range:"(0..100)") {
				attributeState "levelState", action:"switch level.setLevel", label:'${currentValue}%', unit:"%"
			}
		}
		controlTile("rateTile", "device.rate", "slider", height: 2, width: 6, range:"(0..10)") {
			state "rateState", action:"setRate", label:'Rate: ${currentValue}'
		}
		main(["switchLevelTile"])
		details(["switchLevelTile", "rateTile"])
	}
}


// parse HTTP message from device into events
def parse(String description) {
	log.debug "Parsing '${description}'"
	def msg = parseLanMessage(description)
	def headers = msg.headers
	def result = [];
	if (headers.Switch) {
    	result << createEvent(name: "switch", value: headers.Switch);
        log.debug "  switch = ${headers.Switch}"
        }
	if (headers.Level) {
    	result << createEvent(name: "level", value: headers.Level);
        log.debug "  level = ${headers.Level}"
        }
	if (headers.Rate) {
    	result << createEvent(name: "rate", value: headers.Rate);
        log.debug "  rate = ${headers.Rate}"
        }
	return result;
}

def sync(ip, port) {
	log.debug "in Sync()"
	def existingIp = getDataValue("ip")
	def existingPort = getDataValue("port")
	if (ip && ip != existingIp) {
		updateDataValue("ip", ip)
	}
	if (port && port != existingPort) {
		updateDataValue("port", port)
	}
}

def refresh() {
	log.debug "in refresh()"
	return new physicalgraph.device.HubAction(
		method: "GET",
		path: "/set?hubAddr=${getHubAddress()}",
		headers: [
			HOST: "${getHostAddress()}"
		]	)
}

def on() {
	log.debug "in on()"
	return new physicalgraph.device.HubAction(
		method: "GET",
		path: "/set?switch=on&hubAddr=${getHubAddress()}",
		headers: [
			HOST: "${getHostAddress()}"
		]
//,		query: ["switch":"on"]
	)
}

def off() {
	log.debug "in off()"
	return new physicalgraph.device.HubAction(
		method: "GET",
		path: "/set?switch=off&hubAddr=${getHubAddress()}",
		headers: [
			HOST: "${getHostAddress()}"
		]
//,		query: ["switch":"off"]
	)
}

def setLevel(level) {
	log.debug "in setLevel()"
	return new physicalgraph.device.HubAction(
		method: "GET",
		path: "/set?level=${level}&hubAddr=${getHubAddress()}",
		headers: [
			HOST: "${getHostAddress()}"
		]
//,		query: ["level":"${level}"]
	)
}

def setRate(rate) {
	log.debug "in setRate()"
	return new physicalgraph.device.HubAction(
		method: "GET",
		path: "/set?rate=${rate}&hubAddr=${getHubAddress()}",
		headers: [
			HOST: "${getHostAddress()}"
		]
	)
}

// gets the address of the Hub
private getHubAddress() {
    return device.hub.getDataValue("localIP") + ":" + device.hub.getDataValue("localSrvPortTCP")
}

// gets the address of the device
private getHostAddress() {
    def ip = getDataValue("ip")
    def port = getDataValue("port")

    if (!ip || !port) {
//        def parts = device.deviceNetworkId.split(":")
//        if (parts.length == 2) {
//            ip = parts[0]
//            port = parts[1]
//        } else {
            log.warn "Can't figure out ip and port for device: ${device.id}"
//        }
    }

    log.debug "Using IP: $ip and port: $port for device: ${device.id}"
    return convertHexToIP(ip) + ":" + convertHexToInt(port)
}

private Integer convertHexToInt(hex) {
    return Integer.parseInt(hex,16)
}

private String convertHexToIP(hex) {
    return [convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".")
}


/*        state.alertMessage = "some message"
        runIn(2, "sendAlert")

private sendAlert() {
   sendEvent(
      descriptionText: state.alertMessage,
	  eventType: "ALERT",
	  name: "childDeviceCreation",
	  value: "failed",
	  displayed: true,
   )
}
*/
