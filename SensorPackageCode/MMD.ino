#include <SPI.h>
#include <WiFi.h>
#include <stdio.h>
#include <stdint.h>

//1 digital for SPO2
//1 digital for blood pressure
//1 for ECG
//1 for pulse
//1 for thermistor
const int SPO2_PIN = 4;
const int BP_PIN = 1;
const int ECG_PIN = 2;
const int PULSE_PIN = 3;
const int THERM_PIN = 0;

const boolean DEBUG_MODE = true;

//http://mssystems.emscom.net/helpdesk/knowledgebase.php?article=51

const unsigned long MAX_UNSIGNED_UL = 0xFFFFFFFF;

unsigned long start_time, last_time, last_send_time;
unsigned ms_index, s_index, packet_cnt;
unsigned last_reading;

unsigned ekg_ms_readings[1000];
unsigned pulse_ms_readings[1000];
unsigned ms_readings[1000];
double s_avgs[10];
double last_s_avg;
double tens_avg;
boolean full_ms_buf, full_s_buf;

int _status = WL_IDLE_STATUS;
char ssid[] = "EECS";
uint8_t buffer[90];
WiFiServer server(80);
IPAddress ip(10, 3, 13, 158);
boolean cxn;

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
  analogReference(EXTERNAL);   //use 0.45 V AREF

  WiFi.config(ip);
  unsigned attempts = 0;
  cxn = false;
  while (_status != WL_CONNECTED && attempts < 3) {
    debugln("Attempting to connect to EECS");

    _status = WiFi.begin(ssid);
    attempts++;
    delay(5000);
  }
  if (_status != WL_CONNECTED) {
    debugln("Connection to network failed.");
    cxn = false;
  } else {
    cxn = true;
    debugln("Connection to network successful.");
  }
  server.begin();
  if (DEBUG_MODE) Serial.println(WiFi.localIP());

  ms_index = s_index = packet_cnt = 0;
  tens_avg = 0;
  last_time = start_time = micros();
  last_send_time = 0;
  full_ms_buf = full_s_buf = false;

  int i;
  for (i=0; i<90; i++)
    buffer[i] = 0;
  for (i=0; i<1000; i++) {
    ekg_ms_readings[i] = 0;
    pulse_ms_readings[i] = 0;
    ms_readings[i] = 0;
  }
  for (i=0; i<10; i++)
    s_avgs[i] = 0.0;
}

void loop() {
  unsigned long time = micros();
  boolean thresh = false;
  if (last_time > time) { //Overflow, handle it
    if (time + (MAX_UNSIGNED_UL - last_time) > 1000) {
      thresh = true;
    }
  }
  else if ((time - last_time) > 1000) {
    thresh = true;
  }
  if (!thresh || !cxn) return;

  last_time = time;
  last_reading = analogRead(THERM_PIN);
  ekg_ms_readings[ms_index] = analogRead(ECG_PIN);
  //pulse_ms_readings[ms_index] = analogRead(PULSE_PIN);

  double old_reading = ms_readings[ms_index];
  ms_readings[ms_index] = last_reading;

  if (full_ms_buf)
    s_avgs[s_index] = ((s_avgs[s_index] * 1000.0 - old_reading) + ms_readings[ms_index]) / 1000.0;
  else
    s_avgs[s_index] = (s_avgs[s_index] * (ms_index) + ms_readings[ms_index]) / (ms_index + 1);


  WiFiClient client = server.available();

  if (client) {
    if (client.connected()) {
      client.flush();
      int num;
      num = sprintf((char*)buffer, "HTTP/1.1 200 OK\r\n");
      client.write(buffer, num);
      
      debug((char*)buffer);
      num = sprintf((char*)buffer, "Content-Type: application/json\r\n");
      client.write(buffer, num);
      
      debug((char*)buffer);
      num = sprintf((char*)buffer, "Connection: close\r\n\r\n");
      
      client.write(buffer, num);
      debug((char*)buffer);
      num = sprintf((char*)buffer, "{\"l\":%u,\"s\":%u,\"t\":%u,\"e\":\"",
      ms_readings[ms_index], (unsigned) s_avgs[s_index], (unsigned) tens_avg);
      client.write(buffer, num);
      debug((char*)buffer);
      
      if (full_ms_buf) {
        unsigned i;
        num = 0;
        for (i = 0; i < 1000; i += 4) {
          num += sprintf((char*) buffer + num, "%u,", ekg_ms_readings[(ms_index+i)%1000]);
          if (num >= 85) {
            client.write(buffer, num);
            debug((char*)buffer);
            num = 0;
          }
        }
        //buffer[num-1] = ' ';
        client.write(buffer, num-1); // Don't print the last comma
        debug((char*)buffer);
        num = sprintf((char*)buffer, "\"}\r\n\r\n");
        client.write(buffer, num);
        debug((char*)buffer);
      } 
      else {
        num = sprintf((char*)buffer, "0\"}\r\n\r\n");
        client.write(buffer, num);
        debug((char*)buffer);
      }

    }
    client.stop();
  }


  ms_index++;
  if (ms_index >= 1000) {
    if (full_s_buf)
      tens_avg = (tens_avg * 10.0 - last_s_avg + s_avgs[s_index]) / 10.0;
    else
      tens_avg = (tens_avg * (s_index) + s_avgs[s_index]) / ((double) s_index + 1);
//    debugln(ms_readings[ms_index-1]);
//    debug("Latest 1s avg ");
//    debugln(s_avgs[s_index]);
//    debug("Latest 10s avg ");
//    debugln(tens_avg);
    full_ms_buf = true;
    ms_index = 0;
    double old_avg = s_avgs[s_index];

    s_index++;
    if (s_index >= 10) {
//      debug("Latest 10s avg ");
//      debugln(tens_avg);
      s_index = 0; 
      full_s_buf = true;
    }
    last_s_avg = s_avgs[s_index];
    s_avgs[s_index] = old_avg;
  }
}

void debugln(char* msg) {
  if (DEBUG_MODE) Serial.println(msg);
}

void debug(char* msg) {
  if (DEBUG_MODE) Serial.print(msg);
}

