// Load runtime parameters from EEPROM
void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, endpoint);
  EEPROM.get(0+sizeof(endpoint), endpoint_auth);
  EEPROM.get(0+sizeof(endpoint)+sizeof(endpoint_auth), endpoint_fingerprint);
  char ok[2+1];
  EEPROM.get(0+sizeof(endpoint)+sizeof(endpoint_auth)+sizeof(endpoint_fingerprint), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    endpoint[0] = 0;
    endpoint_auth[0] = 0;
    debugln("No params recovered");
  } else {
    debugln("Recovered parameters:");
    debugln(endpoint);
    debugln(strlen(endpoint_auth)>0?"********":"<no auth>");
    debugln(endpoint_fingerprint);
  }
}

// Store runtime parameters to EEPROM
void saveConfig() {
  EEPROM.begin(512);
  EEPROM.put(0, endpoint);
  EEPROM.put(0+sizeof(endpoint), endpoint_auth);
  EEPROM.put(0+sizeof(endpoint)+sizeof(endpoint_auth), endpoint_fingerprint);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(endpoint)+sizeof(endpoint_auth)+sizeof(endpoint_fingerprint), ok);
  EEPROM.commit();
  EEPROM.end();
  debugln("Saved parameters");
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  debugln("Should save config");
  shouldSaveConfig = true;
}
