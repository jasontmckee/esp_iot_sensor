void startServer() {
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/reset", handleReset);
  server.on("/resetwifi", handleResetWiFi);
  server.on("/restart", handleRestart);
  server.on("/identify", handleIdentify);
  server.begin();
  debugln("HTTP server started");
}

/*   
 * Config page @ http://192.168.4.1 in a web browser
 */
void handleRoot() {
  String response = "<!doctype html><html><head>";
  response += "<title>IoT Properties</title>";
  response += "</head><body>";

  //
  // password & endpoint_auth not displayed to the user
  //
  response += "<h1>IoT Properties</h1><form method=\"POST\" action=\"/config\">";
  response += "<p>Version: " + String(VERSION) + "</p>";
  response += "<table>";
  response += "<tr><th>endpoint</th><td><input type=\"text\" name=\"endpoint\" value=\"" + String(endpoint) + "\" size=\"128\"/></td></tr>";
  response += "<tr><th>endpoint_auth</th><td><input type=\"text\" name=\"endpoint_auth\" value=\"\" size=\"64\"/></td></tr>";
  response += "<tr><th>endpoint_fingerprint</th><td><input type=\"text\" name=\"endpoint_fingerprint\" value=\"" + String(endpoint_fingerprint) + "\" size=\"64\"/></td></tr>";
  response += "</table>";
  response += "<input type=\"submit\" value=\"Configure\"/>";
  response += "</form>";

  response += "<p>";
  response += "<a href=\"/identify\">Identify</a><hr/>";
  response += "<a href=\"/restart\">Restart</a><br/>";
  response += "<a href=\"/reset\">Reset</a><br/>";
  response += "</p>";
  response += "<p>";
  response += "<a href=\"/resetwifi\">Reset WiFi</a><hr/>";
  response += "</p>";

  response += "</body></html>";

  server.send(200, "text/html", response);
}

void handleReset() {
  String result = "{\"status\":\"OK\"}";
  server.send(200,"application/json",result);
  debugln("Resetting...");
  delay(1000);
  ESP.reset();
}

void handleRestart() {
  String result = "{\"status\":\"OK\"}";
  server.send(200,"application/json",result);
  debugln("Restarting...");
  delay(1000);
  ESP.restart();
}

void handleResetWiFi() {
  String result = "{\"status\":\"OK\"}";
  server.send(200,"application/json",result);
  debugln("Resetting...");
  delay(1000);
  wifiManager.resetSettings();
  delay(1000);
  ESP.reset();
}

void handleIdentify() {
  // suspend sensor handling by flagging sensor as un-initialized
  // save previous state
  int sensor_state = sensor_initialized;
  sensor_initialized = 0;
  
  String result = "{\"status\":\"OK\"}";
  server.send(200,"application/json",result);

  debug("Identifying");
  for(int i=0; i< 15; i++) {
    toggleLed();
    delay(500);
    toggleLed();
    delay(500);
  }
  debugln("Done!");

  // restore previous state
  sensor_initialized = sensor_state;
}


void handleConfig() {
  String result = "{\"status\":\"OK\"";
  
  for (int i = 0; i < server.args(); i++) {
    result += ",\"" + server.argName(i) + "\":\"" + server.arg(i) + "\"";
    if(server.argName(i)=="endpoint") {
      debug("Found endpoint: ");
      debugln(server.arg(i));
      server.arg(i).toCharArray(endpoint,server.arg(i).length()+1);
    }
    if(server.argName(i)=="endpoint_auth") {
      debug("Found endpoint_auth: ");
      debugln(server.arg(i));
      server.arg(i).toCharArray(endpoint_auth,server.arg(i).length()+1);
    }
    if(server.argName(i)=="endpoint_fingerprint") {
      debug("Found endpoint_fingerprint: ");
      debugln(server.arg(i));
      server.arg(i).toCharArray(endpoint_fingerprint,server.arg(i).length()+1);
    }
  } 

  result += "}";

  // store configured values to EEPROM
  saveConfig();
  
  server.send(200,"application/json",result);
}
