#include <Arduino.h>

#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif

#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <MicroGear.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>     

#define RGB_R_PIN D1
#define RGB_G_PIN D2
#define RGB_B_PIN D3
#define B_PIN D4
#define HDX_2_PIN D5

bool debug = false;

const int WIFI_BLINKING_RATE = 100;
const char *ssid = "khuneieiza";
const char *password = "12341234";

WiFiManager wifiManager;

Ticker ticker;

enum State{
  SETTING_UP,
  INIT,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  INIT_NETPIE,
  WAITING_URL,
  PLAYING_MP3,
  STOP_MP3,
  LOST_CONNECTION_NETPIE,
  SLEEP
};


State state = PLAYING_MP3;


unsigned long timeStart = 0;
unsigned long timePass = 0;

volatile uint8_t count;
uint8_t maxCount=3;
volatile uint8_t timeout;
uint8_t maxTimeout=10;

// Randomly picked URL
const char *URL="http://35.197.128.63/mp3/orjao64.mp3";

char url[1024];
AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;


// NETPIE Configurations 
#define APPID   "huggy"
#define KEY     "rxZRWneDKAa0Inm"
#define SECRET  "CN52FBb9TaUzvYUCqjDZkkXzJ"
#define ALIAS   "NodeMCU"

WiFiClient client;
MicroGear microgear(client);


void debugWiFi(){
  Serial.print("[DEBUG] Connecting to WiFi");
  if (WiFi.begin(ssid, password)) {
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
  }
  Serial.print("\n[DEBUG] WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void deployWiFi(){
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  wifiManager.autoConnect("Huggy","12345678");
}

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("[DEBUG] Incoming message --> ");
  memset(url,0,sizeof(url));
  for(int i=0; i<msglen; i++){
    url[i]=(char)msg[i];
  }
  url[msglen] = '\0';
  Serial.println(url);
  if(state==WAITING_URL){
    Serial.println("[DEBUG] Preparing to data");
    file = new AudioFileSourceICYStream(url);
    file->RegisterMetadataCB(MDCallback, (void*)"ICY");
    buff = new AudioFileSourceBuffer(file, 2048);
    buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
    out = new AudioOutputI2SNoDAC();
    mp3 = new AudioGeneratorMP3();
    mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
    mp3->begin(buff, out);
    turnstile(PLAYING_MP3);
  }
}
void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("[DEBUG] Found new member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();  
}
void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.print("[DEBUG] Lost member --> ");
    for (int i=0; i<msglen; i++)
        Serial.print((char)msg[i]);
    Serial.println();
}
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("[DEBUG] Connected to NETPIE...");
    microgear.setAlias(ALIAS);
    turnstile(WAITING_URL);
    //turnstile(PLAYING_MP3);
}

void LEDLoop1(){
  int color[3] = {0, 0, 0};
  for(int i=0;i<=255;i++){
    color[0] = i;
    analogWrite(RGB_R_PIN, color[0]);
    analogWrite(RGB_G_PIN, color[1]);
    analogWrite(RGB_B_PIN, color[2]);
    delay(5);
  }
  for(int j=0;j<=255;j++){
    color[1] = j;
    analogWrite(RGB_R_PIN, color[0]);
    analogWrite(RGB_G_PIN, color[1]);
    analogWrite(RGB_B_PIN, color[2]);
    delay(5);
  }
  for(int i=255;i>=0;i--){
    color[0] = i;
    analogWrite(RGB_R_PIN, color[0]);
    analogWrite(RGB_G_PIN, color[1]);
    analogWrite(RGB_B_PIN, color[2]);
    delay(5);
  }
  for(int k=0;k<=255;k++){
    color[2] = k;
    analogWrite(RGB_R_PIN, color[0]);
    analogWrite(RGB_G_PIN, color[1]);
    analogWrite(RGB_B_PIN, color[2]);
    delay(5);
  }
  for(int j=255;j>=0;j--){
    color[1] = j;
    analogWrite(RGB_R_PIN, color[0]);
    analogWrite(RGB_G_PIN, color[1]);
    analogWrite(RGB_B_PIN, color[2]);
    delay(5);
  }
  for(int i=0;i<=255;i++){
    color[0] = i;
    analogWrite(RGB_R_PIN, color[0]);
    analogWrite(RGB_G_PIN, color[1]);
    analogWrite(RGB_B_PIN, color[2]);
    delay(5);
  }
  for(int k=255;k>=0;k--){
    color[2] = k;
    analogWrite(RGB_R_PIN, color[0]);
    analogWrite(RGB_G_PIN, color[1]);
    analogWrite(RGB_B_PIN, color[2]);
    delay(5);
  }
  digitalWrite(RGB_R_PIN, LOW);
  digitalWrite(RGB_G_PIN, LOW);
  digitalWrite(RGB_B_PIN, LOW);
}
// Blinking blue led during WiFi connecting
void LEDLoop2(){
  for(int i=0;i<10;i++){
    while(state == WIFI_CONNECTING){
      digitalWrite(B_PIN,HIGH);
      delay(WIFI_BLINKING_RATE);
      digitalWrite(B_PIN,LOW);
      delay(WIFI_BLINKING_RATE); 
    }
  }
}
// Blinking blue led after connect to WiFi
void LEDLoop3(){
  digitalWrite(B_PIN,HIGH);
  delay(3000);
  digitalWrite(B_PIN,LOW);
  delay(3000);
}
// Show battery level
void LEDBatteryLevel(){
  unsigned int r_value;
  unsigned int g_value;
  unsigned int battery = ESP.getVcc();
  battery = map(battery,0,65535,0,100);
  r_value = map(battery,0,100,255,0);
  g_value = map(battery,0,100,0,255);
  analogWrite(RGB_R_PIN, r_value);
  analogWrite(RGB_G_PIN, g_value);
  analogWrite(RGB_B_PIN, 0);
  digitalWrite(B_PIN,LOW);
  Serial.print("BATTERY: ");
  Serial.println(battery);
}

void setupLEDs(){
  Serial.println("[DEBUG] Setting LED pins");
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(HDX_2_PIN,INPUT_PULLUP);
  digitalWrite(RGB_R_PIN, LOW);
  digitalWrite(RGB_G_PIN, LOW);
  digitalWrite(RGB_B_PIN, LOW);
  digitalWrite(B_PIN, LOW);
}
void setupNETPIE(){
  Serial.println("[DEBUG] Setting NETPIE");
  microgear.on(MESSAGE,onMsghandler);
  microgear.on(PRESENT,onFoundgear);
  microgear.on(ABSENT,onLostgear);
  microgear.on(CONNECTED,onConnected);
}

void setup() {
  Serial.begin(115200);
  if(debug){
    Serial.println("\nEntered debug mode");
  }else{
    Serial.println("\nEntered deployment mode");
  }
  Serial.println("[START]");
  turnstile(SETTING_UP);
  setupLEDs();
  setupNETPIE();
  LEDLoop1();  
  turnstile(WIFI_CONNECTING);
  if(debug)
    debugWiFi();
  else {
    deployWiFi();
  }
  turnstile(WIFI_CONNECTED);
  LEDLoop3();
  Serial.println("[INITIALIZING_NETPIE]");
  turnstile(INIT_NETPIE);
  microgear.init(KEY,SECRET,ALIAS);
  microgear.connect(APPID);
  LEDBatteryLevel();
}

void reconnectNETPIE(){
  while(!microgear.connected()){
    Serial.print(".");
    microgear.connect(APPID);
    delay(500);
  }
  turnstile(WAITING_URL);
}

void watingURL(){
  if(!microgear.connected())
    turnstile(LOST_CONNECTION_NETPIE);
}

void playingMP3(){
  timePass = millis();
  if( (timePass-timeStart) > 600000 ){
    if(mp3->isRunning()){
      mp3->stop();
      turnstile(STOP_MP3);
      return;
    }
    Serial.println("MP3 timeout");
  }else{
    if (mp3->isRunning()) {
      if (!mp3->loop()) mp3->stop();
    }
    else{
      mp3->stop();
      turnstile(STOP_MP3);
      return;
    }
  }
}

void stopMP3(){
  count = 0;  
  timeout = 0;
  LEDBatteryLevel();
  Serial.println("[DEBUG] MP3 done");
  ticker.attach(1,waiting);
  while(state==STOP_MP3){
    delay(500);
    if(digitalRead(HDX_2_PIN)==HIGH){
      Serial.println("[DEBUG] Tapped");
      count++;
    }
    if(count>maxCount){
      Serial.println("[DEBUG] Tapped to maximum");
      mp3->begin(buff, out);
      turnstile(PLAYING_MP3);
      return;
    }
  }
}

void waiting(){
  if(state==STOP_MP3){
    Serial.print("[DEBUG] Counting down ");
    Serial.println(maxTimeout-timeout);
    timeout++;
  }
  if(timeout>maxTimeout){
    turnstile(SLEEP);
    ticker.detach();
  }
}

void sleep(){
  analogWrite(RGB_R_PIN, 0);
  analogWrite(RGB_G_PIN, 0);
  analogWrite(RGB_B_PIN, 0);
  ESP.deepSleep(0);
}

void loop()
{
  switch(state){
    case WAITING_URL:
      microgear.loop();
      break;
    case PLAYING_MP3:
      playingMP3();
      break;
    case STOP_MP3:
      stopMP3();
      break;
    case LOST_CONNECTION_NETPIE:
      reconnectNETPIE();
    case SLEEP:
      sleep();
    default:
      break;
  }
}

void turnstile(State s){
  state = s;
  switch(state){
    case SETTING_UP:
      Serial.println("[SETTING_UP]");
      break;
    case INIT:
      Serial.println("[INIT]");
      break;
    case WIFI_CONNECTING:
      Serial.println("[WIFI_CONNECTING]");
      break;
    case WIFI_CONNECTED:
      Serial.println("[WIFI_CONNECTED]");
      break;
    case INIT_NETPIE:
      Serial.println("[INIT_NETPIE]");
      break;
    case WAITING_URL:
      Serial.println("[WAITING_URL]");
      break;
    case PLAYING_MP3:
      Serial.println("[PLAYING_MP3]");
      break;
    case STOP_MP3:
      Serial.println("[STOP_MP3]");
      break;
    case LOST_CONNECTION_NETPIE:
      Serial.println("[LOST_CONNECTION_NETPIE]");
    case SLEEP:
      Serial.println("[SLEEP]");
      break;
  }
}
