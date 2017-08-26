#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

/*
 * Connects to instance configured in EEPROM, registers & checks in 
 * to get runtime configuration values (sample & report intervals)
 */
void registerDevice() {
  // build POST body
  String body = "{\"mac_addr\": \"" + WiFi.macAddress() + "\",\"model\":\"" + ARDUINO_BOARD + "\",\"name\":\"" + netname + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";

  HttpResponse response = post(String(endpoint),body);

  if(response.status==200 || response.status==201) {
    // parse body as JSON

    // way more than we need, but being safe in case the response gets larger...
    StaticJsonBuffer<300> JSONBuffer;
    JsonObject &object = JSONBuffer.parseObject(response.body.c_str());
    if (!object.success()) {
      debugln("JSON parsing failed");
      return;
    }
    
    // set global var for data reporting frequency
    JsonObject& result = object["result"];

    report_interval = result["report_interval"];
    sample_interval = result["sample_interval"];
    
    debugln(String("reporting interval set to ") + report_interval + " seconds");
    debugln(String("sampling interval set to ") + sample_interval + " ms");
  } else {
    debugln(String("ERROR: register returned ") + response.status);
  }
}

/*
 * Send data to SN instance.  Currently hits Scripted REST API placeholder, needs to switch to Metric Base.
 */
void sendValueToServer(float value) {
  String body = "{\"reading\":" + String(value) + "}";

  post(String(endpoint) + "/" + WiFi.macAddress() + "/reading", body);
}

/*
 * Helper function to abstract out HTTP POSTing
*/
HttpResponse post(String url, String body) {
  HttpResponse response = HttpResponse();

  // NOTE that we're not currently handling non-https connections

  String host = getHost(url);
  int port = getPort(url);
  String resource = getResource(url);
  
  WiFiClientSecure client;
  debug("connecting to ");
  debug(host);
  debug(":");
  debug(port);
  debugln(resource);
  
  if (!client.connect(host.c_str(), port)) {
    debugln("connection failed");
    response.status = -1;
    return response;
  }

  if (client.verify(endpoint_fingerprint, host.c_str())) {
    debugln("certificate matches");
  } else {
    debugln("ERROR: certificate doesn't match");
  }

  debug("requesting resource: ");
  debugln(resource);

  client.print(String("POST ") + resource + " HTTP/1.0\r\n" +
               "Host: " + host + "\r\n" +
               "Content-Type: application/json\r\n" + 
               "Authorization: " + endpoint_auth + "\r\n" +
               "Content-Length: " + body.length() + "\r\n" +
               "Connection: close\r\n\r\n" + body);

  debugln("request sent");
  if(client.connected()) {
    String line = client.readStringUntil('\n');
    String status = line.substring(line.indexOf(" ")+1,line.lastIndexOf(" "));
    // set status in response
    response.status = status.toInt();
    debug("status: ");
    debugln(status);
  }
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //debug("header: "); debugln(line);
    if (line == "\r") {
      debugln("headers received");
      break;
    }
  }
  // read body
  //debugln(client.available());
  response.body = "";
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    debugln(line);
    response.body += line;
  }

  debugln("closing connection");  

  return response;
}

/*
 * Helper functions to break apart URLs
*/
String getProtocol(String url) {
  return url.substring(0,url.indexOf(":"));
}
String getHost(String url) {
  String host = url.substring(url.indexOf("://")+3,url.indexOf("/",url.indexOf("://")+3));
  // if port is specified, it will be in the host string above
  if(host.indexOf(":")>-1) {
    host = host.substring(0,host.indexOf(":"));
  }
  return host;  
}

int getPort(String url) {
  int port = 80;
  if(getProtocol(url) == "https") {
    port = 443;  
  }
  String host = url.substring(url.indexOf("://")+3,url.indexOf("/",url.indexOf("://")+3));
  if(host.indexOf(":")>-1) {
    String portNum = host.substring(host.indexOf(":")+1);
    port = portNum.toInt();
  }
  return port;  
}

String getResource(String url) {
  return url.substring(url.indexOf("/",url.indexOf("://")+3));
}

