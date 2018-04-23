#include <Arduino.h>

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

const char* ssid     = "ZTE_2.4G_HqhbFA";
const char* password = "3NrqKMFQ";

#define APPID   "huggy"
#define KEY     "tTyBwFp77MZtzZE"
#define SECRET  "NsS06gdtUIV35YpJA6WGzk3WD"
#define ALIAS   "NodeMCU"

WiFiClient client;
MicroGear microgear(client);

bool debug = true;
int timer = 0;
const char* DEBUG_URL = "http://35.197.128.63/mp3/orjao128.mp3";

char url[1024];
bool ready = false;
bool playing = false;
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;

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

// Blinking blue led before connect to WiFi
void LEDLoop2(){
  for(int i=0 ;i<100;i ++){
      digitalWrite(B_PIN,HIGH);
      delay(100);
      digitalWrite(B_PIN,LOW);
      delay(100);
  }
  digitalWrite(B_PIN,LOW);
}
void debugWiFi(){
  Serial.print("[DEBUG] Connecting to WiFi");
  if (WiFi.begin(ssid, password)) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
  Serial.print("\n[DEBUG] WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void deployWiFi(){

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
/* If a new message arrives, do this */
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("[DEBUG] Incoming message --> ");
  memset(url,0,sizeof(url));
  for(int i=0; i<msglen; i++){
    url[i]=(char)msg[i];
  }
  url[msglen] = '\0';
  ready = true;
  Serial.println(url);
  if(!playing){
      Serial.println("[DEBUG] Preparing to data");
      file  = new AudioFileSourceHTTPStream((char *)url);
      buff  = new AudioFileSourceBuffer(file, 2048);
      buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
      out = new AudioOutputI2SNoDAC();
      mp3 = new AudioGeneratorMP3();
      mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
      mp3->begin(buff, out);
      playing = true;
      Serial.println("[DEBUG] Playing MP3");
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

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("[DEBUG] Connected to NETPIE...");
    /* Set the alias of this microgear ALIAS */
    microgear.setAlias(ALIAS);
}
void setupLEDs(){
  Serial.println("[DEBUG] Setting LED pins");
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
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
  setupLEDs();
  setupNETPIE();
  Serial.begin(115200);
  Serial.println("\nStarting...");
  LEDLoop1();
  LEDLoop2();
  if(debug){
    Serial.println("Entered debug mode");
    debugWiFi();
  }else{
  
  }
  microgear.init(KEY,SECRET,ALIAS);
  microgear.connect(APPID);
}

void loop() {
  if(debug)
    debugging(); 
  else 
    deploy();
}

void debugging(){
  if (!playing) {
    Serial.print("[DEBUG] Publish url --> ");
    Serial.println(DEBUG_URL);
    microgear.chat(ALIAS,DEBUG_URL);
  }else {
    if (mp3->isRunning()) {
      if (!mp3->loop()) mp3->stop();
    } else {
      Serial.printf("[DEBUG] MP3 done\n");
      playing = false;
    }
  }
  if(!microgear.connected()){
        Serial.print("[DEBUG] Connection lost, reconnect");
        while(!microgear.connected()){
          Serial.print('.');
          microgear.connect(APPID);
        }
        Serial.println();
  }
}

void deploy(){
  if(!microgear.connected()){
    Serial.print("[DEPLOYED] Connection lost, reconnect");
    while(!microgear.connected()){
      Serial.print('.');
      microgear.connect(APPID);
      delay(500);
    }
    Serial.println(); 
  }
  if(playing){
    if (mp3->isRunning()) {
      if (!mp3->loop()) {
        mp3->stop();
        playing = false;
      }
    } else {
      Serial.printf("[DEBUG] MP3 done\n");
      playing = false;
    }
  }
}
