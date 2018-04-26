#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <MicroGear.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

#define RGB_R_PIN D1
#define RGB_G_PIN D2
#define RGB_B_PIN D3
#define B_PIN D4
#define HDX_2_PIN D5

const int WIFI_BLINKING_RATE = 100;
const char* ssid     = "ZTE_2.4G_HqhbFA";
const char* password = "3NrqKMFQ";

#define APPID   "huggy"
#define KEY     "rxZRWneDKAa0Inm"
#define SECRET  "CN52FBb9TaUzvYUCqjDZkkXzJ"
#define ALIAS   "NodeMCU"

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

volatile State state = SETTING_UP;
volatile State debugState = STOP_MP3;

WiFiClient client;
MicroGear microgear(client);

bool debug = false;

volatile uint8_t count;
uint8_t maxCount=3;
volatile uint8_t timeout;
uint8_t maxTimeout=10;

const char* DEBUG_URL = "http://35.197.128.63/mp3/orjao64.mp3";

char url[1024];
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;

Ticker ticker;

// Blinking LED when setting up
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
  digitalWrite(B_PIN,HIGH);
  delay(WIFI_BLINKING_RATE);
  digitalWrite(B_PIN,LOW);
  delay(WIFI_BLINKING_RATE);
}
// Blinking blue led after connect to WiFi
void LEDLoop3(){
  digitalWrite(B_PIN,HIGH);
  delay(3000);
  digitalWrite(B_PIN,LOW);
}
void debugWiFi(){
  Serial.print("[DEBUG] Connecting to WiFi");
  if (WiFi.begin(ssid, password)) {
    while (WiFi.status() != WL_CONNECTED) {
      LEDLoop2();
      Serial.print(".");
    }
  }
  Serial.print("\n[DEBUG] WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}
void deployWiFi(){
  // Enanble WiFi Manager
}
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
      file  = new AudioFileSourceHTTPStream((char *)url);
      buff  = new AudioFileSourceBuffer(file, 2048);
      buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
      out = new AudioOutputI2SNoDAC();
      mp3 = new AudioGeneratorMP3();
      mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
      mp3->begin(buff, out);
      Serial.println("[DEBUG] Playing MP3");
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
    if(debug)
      turnstile(debugState);
    else
      turnstile(WAITING_URL);
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
  else 
    deployWiFi();
  turnstile(WIFI_CONNECTED);
  LEDLoop3();
  Serial.println("[INITIALIZING_NETPIE]");
  turnstile(INIT_NETPIE);
  microgear.init(KEY,SECRET,ALIAS);
  microgear.connect(APPID);
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
void runningMP3(){
  if (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  } else {
    turnstile(STOP_MP3);
  }
}
void stopMP3(){
  count = 0;  
  timeout = 0;
  Serial.println("[DEBUG] MP3 done");
  ticker.attach(1,waiting);
  while(state==STOP_MP3){
    delay(500);
    if(digitalRead(HDX_2_PIN)==HIGH){
      Serial.println("[DEBUG] Tapped");
      count++;
    }
    else if(count>maxCount){
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
  if(debug)
     turnstile(WAITING_URL);
  else
    ESP.deepSleep(0);
}
void loop() {
  if(debug)
    debugging(); 
  else 
    deploy();
}
void debugging(){
  while(1){
    switch(state){
      case WAITING_URL:
        Serial.print("[DEBUG] Publish url --> ");
        Serial.println(DEBUG_URL);
        microgear.chat(ALIAS,DEBUG_URL);
        delay(500);
        break;
      case PLAYING_MP3:
        runningMP3();
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
}

void deploy(){
    while(1){
    switch(state){
      case WAITING_URL:
        delay(500);
        break;
      case PLAYING_MP3:
        runningMP3();
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
