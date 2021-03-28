#include <Arduino.h>
#include <FirebaseESP32.h>
#include <SoftwareSerial.h>
#include <WiFi.h>

#define FIREBASE_HOST "test-fyp-5c283-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "IjH4uhAUxUVnrGNgPxFaF5nPSIpXrOcLAqwXGACc"

#define WIFI_SSID "test"
#define WIFI_PASSWORD "12345678"
/*
dPin  Label   Function
 2    D2      Wifi status LED
 4    D4      Bluetooth status LED
13    D13     HC05 State
16    RX2     HC05rx
17    TX2     HC05tx
18    D18     r0rx
19    D19     r0tx
21    D21     
22    D22     r1rx
23    D23     r1tx
25    D25     
26    D26     r2rx
27    D27     r2tx
32    D32     r3rx
33    D33     r3tx
*/

#define N             1
#define MAX_DEFINED_N 4
#define WIFILED       2
#define BTLED         4
#define BTSTATE       13

String stringRack[N];

#if N >= 1
SoftwareSerial serialRack0(18, 19);
#endif
#if N >= 2
SoftwareSerial serialRack1(22, 23);
#endif
#if N >= 3
SoftwareSerial serialRack2(26, 27);
#endif
#if N >= 4
SoftwareSerial serialRack3(32, 33);
#endif

#if N > MAX_DEFINED_N || N == 0
#error Bad N value
#endif

SoftwareSerial BTSerial(16, 17);
FirebaseData database;
TaskHandle_t mainLoop_H;
TaskHandle_t statusLoop_H;

void mainLoop(void*);
void statusLoop(void*);
void startWifi();
void startFirebase();
void updateFirebase();
void getSensorReadings();
void updateRacks(String);
SoftwareSerial& serialRack(int);

void setup() {
  // connect to secondary devices
  Serial.begin(9600);
  for (int i = 0; i < N; i++) {
    serialRack(i).begin(9600);
  }
  pinMode(WIFILED, OUTPUT);
  pinMode(BTLED, OUTPUT);
  pinMode(BTSTATE, INPUT);
  BTSerial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // connect to wifi.
  startWifi();

  // connect to firebase
  startFirebase();
  delay(500);
  xTaskCreatePinnedToCore(
      statusLoop,     /* Task function. */
      "statusLoop_H", /* name of task. */
      10000,          /* Stack size of task */
      NULL,           /* parameter of the task */
      1,              /* priority of the task */
      &statusLoop_H,  /* Task handle to keep track of created task */
      1);             /* pin task to core 1 */
  delay(1500);
  xTaskCreatePinnedToCore(
      mainLoop,       /* Task function. */
      "mainLoop",     /* name of task. */
      10000,          /* Stack size of task */
      NULL,           /* parameter of the task */
      1,              /* priority of the task */
      &mainLoop_H,    /* Task handle to keep track of created task */
      0);             /* pin task to core 0 */
}

void mainLoop(void* pvParameters){
  while (1) {
    getSensorReadings();
    updateFirebase();
    delay(1000);
    if (BTSerial.available()) {
      updateRacks(BTSerial.readStringUntil('\n'));
    }
  }
}

void statusLoop(void* pvParameters) {
  while (1) {
    delay(500);
    if(WiFi.status() == WL_CONNECTED){
      digitalWrite(WIFILED, HIGH);
    } else {
      digitalWrite(WIFILED, LOW);
    }
    if(digitalRead(BTSTATE) == 1){
      digitalWrite(BTLED, HIGH);
    } else {
      digitalWrite(BTLED, LOW);
    }
  }
}

void loop() {}

void updateFirebase() {
  if (WiFi.status() != WL_CONNECTED) return;
  for (int i = 0; i < N; i++) {
    Firebase.setString(database, "/rack" + String(i) + "/status", stringRack[i]);
  }
}

void getSensorReadings() {
  for (int i = 0; i < N; i++) {
    if (serialRack(i).available()) {
      stringRack[i] = serialRack(i).readStringUntil('\n');
      Serial.println(stringRack[i]);
    }
  }
}

void updateRacks(String c) {
  int rackNo = (int)c[0];
  String cmd = c.substring(1);
  serialRack(rackNo).print(cmd);
}

SoftwareSerial& serialRack(int x) {
  switch (x) {
#if N >= 1
    case 0:
      return serialRack0;
#endif
#if N >= 2
    case 1:
      return serialRack1;
#endif
#if N >= 3
    case 2:
      return serialRack2;
#endif
#if N >= 4
    case 3:
      return serialRack3;
#endif
    default:
      break;
  }
  // Should never reach here
  return serialRack0;
}

/**************************************************************/

void startWifi() {
  Serial.println("connecting");
  int timeout_wifi = 60;  // 30 seconds on 500ms delay
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if (timeout_wifi == 0) {
      break;
    }
    timeout_wifi--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("connected: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wifi timed out, will try again later.");
  }
}

void startFirebase() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi not connected!");
    return;
  }
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(database, 1000 * 60);
  Firebase.setwriteSizeLimit(database, "small");
}