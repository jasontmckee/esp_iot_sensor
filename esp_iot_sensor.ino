/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include "HttpClient.h"
#include "Version.h"
#include "WiFiDefaults.h"

#ifdef ARDUINO_ESP8266_NODEMCU
#define LED LED_BUILTIN
#define SENSOR D2
#endif

#ifdef ARDUINO_ESP8266_ESP01
#define LED LED_BUILTIN
#define SENSOR 2
// reusing TX serial pin with LED_BUILTIN, so no serial output
#define NOSERIAL
#endif

// at some point ESP01 became GENERIC
#ifdef ARDUINO_ESP8266_GENERIC
//#define LED LED_BUILTIN
#define LED 1
#define SENSOR 2
// reusing TX serial pin with LED_BUILTIN, so no serial output
#define NOSERIAL
#endif

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
#define LED LED_BUILTIN
#define SENSOR D1
#endif

/*
 * If serial is turned off, the debug() macros need to be a noop
 * Gottcha here is if you do something like: 
 * debug("foo"); i++;
 * in which case i++ will not be compiled/executed
*/
#ifdef NOSERIAL
#define debug(x) //(x)
#define debugln(x) //(x)
#else
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#endif

/*
 * Next gottcha is on many (most?) ESP8266's the behavior of LED_BUILTIN is
 * reversed, HIGH==off, LOW==on
 * defining ON/OFF lets us keep the same behavior with built-in or LEDs 
 * connected to data pins
*/
#if LED == LED_BUILTIN
#define ON LOW
#define OFF HIGH
#else
#define ON HIGH
#define OFF LOW
#endif

const char* netnamePrefix = "iot-";

// config variables (stored in EEPROM)
char endpoint[128] = "";
char endpoint_auth[64] = "";
char endpoint_fingerprint[64] = "";

// calling actual saveConfig from wifimanager makes things unstable
bool shouldSaveConfig = false;

// configuration from server
int report_interval = 0; // seconds
int sample_interval = 100; // ms

int send_data = 0;

char *netname;

ESP8266WebServer server(80);

WiFiManager wifiManager;
  
Ticker ledTimer;
int ledState = ON;

void setup() {
	delay(1000);
#ifndef NOSERIAL  
	Serial.begin(115200);
#endif

  debugln("");
  debug("Version: ");
  debugln(VERSION);

  // recover config from EEPROM
  loadConfig();

  // WiFiManager versions of config variables
  WiFiManagerParameter custom_endpoint("endpoint", "endpoint", endpoint, 128);
  WiFiManagerParameter custom_endpoint_auth("endpoint_auth", "endpoint authorization", endpoint_auth, 64);
  WiFiManagerParameter custom_endpoint_fingerprint("endpoint_fingerprint", "endpoint SHA fingerprint", endpoint_fingerprint, 64);

  // set pin modes
  pinMode(LED,OUTPUT);
  pinMode(SENSOR,INPUT);

  // build device/AP name
  String s = netnamePrefix + WiFi.macAddress();
  s.replace(":","");
  netname = (char*)malloc(s.length()+1);
  s.toCharArray(netname,s.length()+1);
  debug("netname: ");
  debugln(netname);

  // save previous ssid/pass used by wifimanager
  String lastSSID = WiFi.SSID();
  String lastPassword = WiFi.psk();

  if(connectToAP(DEFAULT_SSID,DEFAULT_PASSWORD)) {
    debugln("connected to default ssid");
  } else if(connectToAP(lastSSID,lastPassword)) {
    debugln("connected to last ssid");
  } else {
    debugln("launching WiFiManager");
    wifiManager.addParameter(&custom_endpoint);
    wifiManager.addParameter(&custom_endpoint_auth);
    wifiManager.addParameter(&custom_endpoint_fingerprint);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
  
    ledTimer.attach(1,toggleLed);
    if(!wifiManager.autoConnect(netname)) {
      debugln("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  }
  
  // success! continue startup
  ledTimer.detach();
  ledTimer.attach(0.1,toggleLed);
  delay(3000);
  ledTimer.detach();
  digitalWrite(LED,OFF);

  if(shouldSaveConfig) {
    debugln("");
    debugln("persisting WiFiManager config params:");
    if(strcmp(custom_endpoint.getValue(),"")!=0) {
      strcpy(endpoint,custom_endpoint.getValue());
      debugln(String("endpoint: ") + endpoint);
    }
    if(strcmp(custom_endpoint_auth.getValue(),"")!=0) {
      strcpy(endpoint_auth,custom_endpoint_auth.getValue());
      debugln(String("endpoint_auth: ") + endpoint_auth);
    }
    if(strcmp(custom_endpoint_fingerprint.getValue(),"")!=0) {
      strcpy(endpoint_fingerprint,custom_endpoint_fingerprint.getValue());
      debugln(String("endpoint_fingerprint: ") + endpoint_fingerprint);
    }
    saveConfig();
  }
  
  // launch web server for additional configuration
  startServer();

  // other boot-up stuff goes here (thinger.io...etc.)
  registerDevice();

  // if we're in normal mode, init sensors
  sensorInit();

  // advertise mDNS name
  delay(2000);
  if(MDNS.begin(netname,WiFi.localIP(),120)) {
    debug("mDNS responder started: ");
    debug(netname);
    debugln(".local");

    MDNS.addService("http", "tcp", 80);

    MDNS.update();
  } else {
    debugln("Error starting mDNS");  
  }
  
}

void loop() {
	server.handleClient();

  if(send_data==1) {
    sendSensorData();  
  }
}

void toggleLed() {
    digitalWrite(LED,ledState);
    //debug(".");
    ledState = (ledState==ON) ? OFF : ON;  
}
