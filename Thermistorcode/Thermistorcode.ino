#include <SPI.h>
#include <WiFi.h>

const int analogPin = 0;
const unsigned long MAX_UNSIGNED_UL = 0xFFFFFFFF;
const unsigned PACKET_INTERVAL = 50000;

unsigned long start_time, last_time, last_send_time;
unsigned ms_index, s_index, packet_cnt;
unsigned last_reading;

unsigned ms_readings[1000];
double s_avgs[10];
double last_s_avg;
double tens_avg;
boolean full_ms_buf, full_s_buf;

int _status = WL_IDLE_STATUS;
char ssid[] = "EECS";
WiFiServer server(80);
IPAddress ip(10, 3, 13, 158);

boolean client_has_xctd;

void setup() {
  Serial.begin(9600);
   pinMode(A0, INPUT);
   analogReference(EXTERNAL);   //use 0.45 V AREF
  
  WiFi.config(ip);
  while (_status != WL_CONNECTED) {
    Serial.println("Attempting to connect to EECS");
    
    _status = WiFi.begin(ssid);
    delay(5000);
  }
  Serial.println("Connection to network successful.");
  
  server.begin();
  Serial.print(WiFi.localIP());
  Serial.println();
  
  ms_index = s_index = packet_cnt = 0;
  tens_avg = 0;
  last_time = start_time = micros();
  last_send_time = 0;
  full_ms_buf = full_s_buf = false;
  client_has_xctd = false;
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
  if (!thresh) return;
  
  last_time = time;
  last_reading = analogRead(analogPin);
  
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
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"packet\":{\"last_reading\":");
      client.print(ms_readings[ms_index]);
      client.print(",\"s_avg\":");
      client.print(s_avgs[s_index]);
      client.print(",\"tens_avg\":");
      client.print(tens_avg);
      client.print(",\"packet\":");
      client.print(packet_cnt++);
      client.println("}}");
      client.flush();
    }
    client.stop();
  }
  
  ms_index++;
  if (ms_index >= 1000) {
    if (full_s_buf)
      tens_avg = (tens_avg * 10.0 - last_s_avg + s_avgs[s_index]) / 10.0;
    else
      tens_avg = (tens_avg * (s_index) + s_avgs[s_index]) / ((double) s_index + 1);
    //Serial.println(ms_readings[ms_index-1]);
    Serial.print("Latest 1s avg ");
    Serial.println(s_avgs[s_index]);
    Serial.print("Latest 10s avg ");
    Serial.println(tens_avg);
    full_ms_buf = true;
    ms_index = 0;
    double old_avg = s_avgs[s_index];
    
    s_index++;
    if (s_index >= 10) {
      //Serial.print("Latest 10s avg ");
      //Serial.println(tens_avg);
      s_index = 0; 
      full_s_buf = true;
    }
    last_s_avg = s_avgs[s_index];
    s_avgs[s_index] = old_avg;
  }
}

