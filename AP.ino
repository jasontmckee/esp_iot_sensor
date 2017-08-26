void doAP() {
  debug("Configuring access point...");

  // turn on AP mode
  WiFi.mode(WIFI_AP);

  WiFi.softAP(netname);

  IPAddress myIP = WiFi.softAPIP();
  delay(1000);
  debug("AP IP address: ");
  debugln(myIP);

  // fire up web server
  startServer();
}


