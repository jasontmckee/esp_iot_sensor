# esp_iot_sensor

Simple ESP8266-based IoT sensor.  Collects readings, reports back to upstream server on a configured interval.  

Tested on ESP-01 and NodeMCU v1.0 devices

## Details

On boot, reads network SSID/pass from EEPROM and attempts to connect.  If unable to connect, switch to AP mode with iot-<mac_addr> SSID and runs web server with config page @ http://192.168.4.1.  If successfully connected to wifi, device connects to endpoint (also in EEPROM), registers and pulls runtime config data (sample and report interval).  It then samples digital sensor data and reports the time spent on as a percentage.

Assumes a sensor device like a motion sensor that stays latched on while motion is detected.  To use a sound level or other sensor that toggles on/off in < 1ms timeframes, the sensor read code needs to be adjusted to poll instead of read on a timer.
