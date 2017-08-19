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

/*
 * If serial is turned off, the PRINT() macros need to be a noop
 * Gottcha here is if you do something like: 
 * PRINT("foo"); i++;
 * in which case i++ will not be compiled/executed
*/
#ifdef NOSERIAL
#define PRINT(x) //(x)
#define PRINTLN(x) //(x)
#else
#define PRINT(x) Serial.print(x)
#define PRINTLN(x) Serial.println(x)
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

// struct definition needs to be here to make Client.ino compile right
struct HttpResponse {
  int status;
  String body;
};

const char* netnamePrefix = "iot-";

// config variables (stored in EEPROM)
char ssid[32];
char password[32];
char endpoint[128];
char endpoint_auth[64];
char endpoint_fingerprint[64];

int report_interval = 0; // seconds
int sample_interval = 100; // ms

int send_data = 0;

char *netname;

ESP8266WebServer server(80);

void setup() {
	delay(1000);
#ifndef NOSERIAL  
	Serial.begin(115200);
#endif

  // recover config from EEPROM
  loadConfig();

  // set pin modes
  pinMode(LED,OUTPUT);
  pinMode(SENSOR,INPUT);

  // build device/AP name
  String s = netnamePrefix + WiFi.macAddress();
  s.replace(":","");
  netname = (char*)malloc(s.length()+1);
  s.toCharArray(netname,s.length()+1);
  PRINT("netname: ");
  PRINTLN(netname);

  // connect to configured AP
  WiFi.begin(ssid, password);

  int retry = 0;
  PRINTLN("Connecting Wifi");
  digitalWrite(LED,ON);
  while(WiFi.status() != WL_CONNECTED && retry < 30) {
    PRINT(".");
    retry++;
    delay(1000);
  }
  digitalWrite(LED,OFF);
  
  if(WiFi.status() != WL_CONNECTED) {
    // if we can't connect to configured ssid, launch the AP for configuration
    doAP();
  } else {
    // connected to ssid, boot normally
    PRINTLN("");
    PRINTLN("WiFi connected");
    PRINTLN("IP address: ");
    PRINTLN(WiFi.localIP());

    // launch web server for additional configuration
    startServer();
  
    // other boot-up stuff goes here (thinger.io...etc.)
    registerDevice();

    // if we're in normal mode, init sensors
    sensorInit();
  }

  // advertise Bonjour/Zeroconf name
  if(MDNS.begin(netname)) {
    PRINT("MDNS responder started: ");
    PRINT(netname);
    PRINTLN(".local");
  }
}

void loop() {
	server.handleClient();

  if(send_data==1) {
    sendSensorData();  
  }
}


