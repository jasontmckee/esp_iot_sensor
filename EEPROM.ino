// Load runtime parameters from EEPROM
void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  EEPROM.get(0+sizeof(ssid)+sizeof(password), endpoint);
  EEPROM.get(0+sizeof(ssid)+sizeof(password)+sizeof(endpoint), endpoint_auth);
  EEPROM.get(0+sizeof(ssid)+sizeof(password)+sizeof(endpoint)+sizeof(endpoint_auth), endpoint_fingerprint);
  char ok[2+1];
  EEPROM.get(0+sizeof(ssid)+sizeof(password)+sizeof(endpoint)+sizeof(endpoint_auth)+sizeof(endpoint_fingerprint), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
    endpoint[0] = 0;
    endpoint_auth[0] = 0;
    PRINTLN("No params recovered");
  } else {
    PRINTLN("Recovered parameters:");
    PRINTLN(ssid);
    PRINTLN(strlen(password)>0?"********":"<no password>");
    PRINTLN(endpoint);
    PRINTLN(strlen(endpoint_auth)>0?"********":"<no auth>");
    PRINTLN(endpoint_fingerprint);
  }
}

// Store runtime parameters to EEPROM
void saveConfig() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  EEPROM.put(0+sizeof(ssid)+sizeof(password), endpoint);
  EEPROM.put(0+sizeof(ssid)+sizeof(password)+sizeof(endpoint), endpoint_auth);
  EEPROM.put(0+sizeof(ssid)+sizeof(password)+sizeof(endpoint)+sizeof(endpoint_auth), endpoint_fingerprint);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(ssid)+sizeof(password)+sizeof(endpoint)+sizeof(endpoint_auth)+sizeof(endpoint_fingerprint), ok);
  EEPROM.commit();
  EEPROM.end();

  PRINTLN("Saved parameters");
}
