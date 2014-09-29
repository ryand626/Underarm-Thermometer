const int analogPin = 0;
const unsigned long MAX_UNSIGNED_UL = 0xFFFFFFFF;
const unsigned PACKET_INTERVAL = 50000;

unsigned long start_time, last_time, last_send_time;
unsigned ms_index, s_index, packet_cnt;
unsigned last_reading;

double ms_readings[1000];
double s_avgs[10];
double tens_avg;
boolean full_ms_buf, full_s_buf;

void setup() {
  Serial.begin(9600);
  
  //Network connection here
  
  ms_index = s_index = packet_cnt = 0;
  last_time = start_time = micros();
  last_send_time = 0;
  full_ms_buf = full_s_buf = false;
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
  last_reading = (double) analogRead(analogPin);
  
  ms_readings[ms_index++] = last_reading;
  
  //s_avgs[s_index] = (s_index ? s_avgs[s_index-1] : 0) * (s_index-1) + last_reading) / s_index;
  
  s_avgs[s_index] = recalc_avg(ms_readings, ms_index, 1000, full_ms_buf);
  tens_avg = recalc_avg(s_avgs, s_index, 10, full_s_buf);
  
  // ----------
  // Wifi stuff here
  thresh = false;
  if (last_send_time > time && (time + (MAX_UNSIGNED_UL - last_send_time) > PACKET_INTERVAL )) {
    thresh = true;
  } else if ((last_send_time - time) > PACKET_INTERVAL) {
    thresh = true;
  }
  if (thresh) {
    //Send wifi UDP packet of string:
    // {last_reading}:{s_avgs[s_index]}:{tens_avg}:{packet_cnt}
  }
  // ----------
  
  if (ms_index >= 1000) {
    Serial.print("Latest 1s avg ");
    Serial.println(s_avgs[s_index]);
    full_ms_buf = true;
    ms_index = 0;
    s_index++;
    if (s_index >= 100) {
      Serial.print("Latest 10s avg ");
      Serial.println(tens_avg);
      s_index = 0; 
      full_s_buf = true;
    }
  }
}

/*
 * Recalculate the average for some array. Will recalculate an average of
 * arr_size values if full is true, and arr_index values if it is not (meaning
 * arr_index is the NEXT index after the index of the last reading, 0 if newly full)
 */
double recalc_avg(double* arr, unsigned arr_index, unsigned arr_size, boolean full) {
  double avg = 0;
  if (!full) {
    for (unsigned i = 0; i < arr_index; i++)
      avg += arr[i];
    avg /= arr_index;
  } else {
    boolean sentinel = true;
    for (unsigned i = arr_index + 1; sentinel; i++) {
      i = i % arr_size;
      avg += arr[i];
    }
    avg /= arr_size;
  }
  
  return avg;
}
