void startServer() {
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/reset", handleReset);
  server.begin();
  PRINTLN("HTTP server started");
}

/*   
 * Config page @ http://192.168.4.1 in a web browser
 */
void handleRoot() {
  String response = "<!doctype html><html><head>";
  response += "<title>SN IoT Properties</title>";
  response += "</head><body>";

  //
  // password & endpoint_auth not displayed to the user
  //
  response += "<h1>SN IoT Properties</h1><form method=\"POST\" action=\"/config\"><table>";
  response += "<tr><th>ssid</th><td><input type=\"text\" name=\"ssid\" value=\"" + String(ssid) + "\"/></td></tr>";
  response += "<tr><th>password</th><td><input type=\"text\" name=\"password\" value=\"\"/></td></tr>";
  response += "<tr><th>endpoint</th><td><input type=\"text\" name=\"endpoint\" value=\"" + String(endpoint) + "\" size=\"128\"/></td></tr>";
  response += "<tr><th>endpoint_auth</th><td><input type=\"text\" name=\"endpoint_auth\" value=\"\" size=\"64\"/></td></tr>";
  response += "<tr><th>endpoint_fingerprint</th><td><input type=\"text\" name=\"endpoint_fingerprint\" value=\"" + String(endpoint_fingerprint) + "\" size=\"64\"/></td></tr>";
  response += "</table>";
  response += "<input type=\"submit\" value=\"Configure\"/>";
  response += "</form>";

  response += "</body></html>";

  server.send(200, "text/html", response);
}

void handleReset() {
  String result = "{\"status\":\"OK\"}";
  server.send(200,"application/json",result);
  PRINTLN("Resetting...");
  delay(1000);
  ESP.restart();
}

void handleConfig() {
  String result = "{\"status\":\"OK\"";
  
  for (int i = 0; i < server.args(); i++) {
    result += ",\"" + server.argName(i) + "\":\"" + server.arg(i) + "\"";
    if(server.argName(i)=="ssid") {
      PRINT("Found ssid: ");
      PRINTLN(server.arg(i));
      server.arg(i).toCharArray(ssid,server.arg(i).length()+1);
    }
    if(server.argName(i)=="password") {
      PRINT("Found password: ");
      PRINTLN(server.arg(i));
      server.arg(i).toCharArray(password,server.arg(i).length()+1);
    }
    if(server.argName(i)=="endpoint") {
      PRINT("Found endpoint: ");
      PRINTLN(server.arg(i));
      server.arg(i).toCharArray(endpoint,server.arg(i).length()+1);
    }
    if(server.argName(i)=="endpoint_auth") {
      PRINT("Found endpoint_auth: ");
      PRINTLN(server.arg(i));
      server.arg(i).toCharArray(endpoint_auth,server.arg(i).length()+1);
    }
    if(server.argName(i)=="endpoint_fingerprint") {
      PRINT("Found endpoint_fingerprint: ");
      PRINTLN(server.arg(i));
      server.arg(i).toCharArray(endpoint_fingerprint,server.arg(i).length()+1);
    }
  } 

  result += "}";

  // store configured values to EEPROM
  saveConfig();
  
  server.send(200,"application/json",result);
}
