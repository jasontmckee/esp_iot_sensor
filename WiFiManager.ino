void configModeCallback(WiFiManager *myWiFiManager) {
  debugln("Entered config mode");
  debugln(WiFi.softAPIP());

  debugln(myWiFiManager->getConfigPortalSSID());
}

bool connectToAP(String ssid, String pass) {
  debug(String("attempting to connect to ") + ssid + " ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(),pass.c_str());
  int default_connect_delay_count = 0;
  do {
    toggleLed();
    default_connect_delay_count++;
    delay(500);
  } while (WiFi.status() != WL_CONNECTED && default_connect_delay_count < 20);

    debugln("");
  if(WiFi.isConnected()) {
    debugln("success");
    digitalWrite(LED,OFF);
    return true; 
  } 

  debugln("failed");    
  WiFi.disconnect();
  delay(2000);

  return false;
}

