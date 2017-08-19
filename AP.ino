void doAP() {
  PRINT("Configuring access point...");

  WiFi.softAP(netname);

  IPAddress myIP = WiFi.softAPIP();
  PRINT("AP IP address: ");
  PRINTLN(myIP);

  // fire up web server
  startServer();
}


