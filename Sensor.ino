long onCount = 0;
long count = 0;
int sensor_initialized = 0;

Ticker sendTimer;
Ticker readTimer;

void sensorInit() {
  // attach timer callbacks
  sendTimer.attach(report_interval, flagForSend);
  readTimer.attach_ms(sample_interval, handleSensor);
  sensor_initialized = 1;
}

/*
 * timer interrupt handler: read sensor values
 */
void handleSensor() {
  if(sensor_initialized==0) {
    return;  
  }
  int value = digitalRead(SENSOR);
  digitalWrite(LED,(value==HIGH ? ON : OFF));

  if(value==1) {
    onCount++;
  } 
  count++;
  
}

/* 
 * timer interrupt handler: just toggles the send flag 
 */
void flagForSend() {
  send_data = 1;  
}

/* 
 * bundles up on/off counts into % and sends to server 
 */
void sendSensorData() {
  float pct = (float)onCount/((float)count);
  pct*=100;
  debug(millis());
  debug(": ");
  debug(pct);
  debug("% ");
  debug(onCount);
  debug("/");
  debugln(count);

  sendValueToServer(pct);

  send_data = 0;
  onCount=0;
  count=0;
}
