#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

/*
 * Connects to instance configured in EEPROM, registers & checks in 
 * to get runtime configuration values (sample & report intervals)
 */
void registerDevice() {
  String ep = String(endpoint);
  String host = ep.substring(ep.indexOf("://")+3,ep.indexOf("/",ep.indexOf("://")+3));
  String resource = ep.substring(ep.indexOf("/",ep.indexOf("://")+3));

  // build POST body
  String body = "{\"mac_addr\": \"" + WiFi.macAddress() + "\",\"model\":\"" + ARDUINO_BOARD + "\",\"name\":\"" + netname + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";

  HttpResponse response = post(host,resource,body);

  if(response.status==200 || response.status==201) {
    // parse body as JSON

    // way more than we need, but being safe in case the response gets larger...
    StaticJsonBuffer<300> JSONBuffer;
    JsonObject &object = JSONBuffer.parseObject(response.body.c_str());
    if (!object.success()) {
      PRINTLN("JSON parsing failed");
      return;
    }
    
    // set global var for data reporting frequency
    JsonObject& result = object["result"];

    report_interval = result["report_interval"];
    sample_interval = result["sample_interval"];
    
    PRINTLN(String("reporting interval set to ") + report_interval + " seconds");
    PRINTLN(String("sampling interval set to ") + sample_interval + " ms");
  } else {
    PRINTLN(String("ERROR: register returned ") + response.status);
  }
}

/*
 * Send data to SN instance.  Currently hits Scripted REST API placeholder, needs to switch to Metric Base.
 */
void sendValueToServer(float value) {
  String ep = String(endpoint);
  String host = ep.substring(ep.indexOf("://")+3,ep.indexOf("/",ep.indexOf("://")+3));
  String resource = ep.substring(ep.indexOf("/",ep.indexOf("://")+3));

  String body = "{\"reading\":" + String(value) + "}";

  post(host,resource + "/" + WiFi.macAddress() + "/reading", body);
}

/*
 * Helper function to abstract out HTTP POSTing
*/
HttpResponse post(String host, String resource, String body) {
  HttpResponse response = HttpResponse();
  
  WiFiClientSecure client;
  PRINT("connecting to ");
  PRINTLN(host);
  if (!client.connect(host.c_str(), 443)) {
    PRINTLN("connection failed");
    response.status = -1;
    return response;
  }

  if (client.verify(endpoint_fingerprint, host.c_str())) {
    PRINTLN("certificate matches");
  } else {
    PRINTLN("ERROR: certificate doesn't match");
  }

  PRINT("requesting resource: ");
  PRINTLN(resource);

  client.print(String("POST ") + resource + " HTTP/1.0\r\n" +
               "Host: " + host + "\r\n" +
               "Content-Type: application/json\r\n" + 
               "Authorization: " + endpoint_auth + "\r\n" +
               "Content-Length: " + body.length() + "\r\n" +
               "Connection: close\r\n\r\n" + body);

  PRINTLN("request sent");
  if(client.connected()) {
    String line = client.readStringUntil('\n');
    String status = line.substring(line.indexOf(" ")+1,line.lastIndexOf(" "));
    // set status in response
    response.status = status.toInt();
    PRINT("status: ");
    PRINTLN(status);    
  }
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //PRINT("header: "); PRINTLN(line);
    if (line == "\r") {
      PRINTLN("headers received");
      break;
    }
  }
  // read body
  //PRINTLN(client.available());
  response.body = "";
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    PRINTLN(line);
    response.body += line;
  }

  PRINTLN("closing connection");  

  return response;
}
