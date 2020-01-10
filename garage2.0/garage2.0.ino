#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <RotaryEncoder.h>

void connect();
void redirect();
void handleRoot();
void handleNotFound();
void servePage();
void apiStatus();
void calibrate();

IPAddress ip(192, 168, 0, 144);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
const char* ssid = "";
const char* password = "";
bool doorOpen = false;
bool lightOn = false;
const int doorSwitch = 4;
const int lightSwitch = 0;

#define PIN_A   D5 //ky-040 clk pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!
#define PIN_B   D6 //ky-040 dt  pin,             add 100nF/0.1uF capacitors between pin & ground!!!
#define BUTTON  D7 //ky-040 sw  pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!
int16_t position = 0;
RotaryEncoder encoder(PIN_A, PIN_B, BUTTON);
void ICACHE_RAM_ATTR encoderISR();
void ICACHE_RAM_ATTR encoderButtonISR();

ESP8266WebServer server(80); 

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(doorSwitch, OUTPUT); 
  pinMode(lightSwitch, OUTPUT);  
  digitalWrite(doorSwitch, HIGH);
  digitalWrite(lightSwitch, HIGH);
  
  Serial.begin(115200); 
  encoder.begin();
  SPIFFS.begin();

  connect();
   
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  SPIFFS.begin();

  
  attachInterrupt(digitalPinToInterrupt(PIN_A),  encoderISR,       CHANGE);  //call encoderISR()       every high->low or low->high changes
  attachInterrupt(digitalPinToInterrupt(PIN_B),  encoderISR,       CHANGE);  //call encoderISR()       every high->low or low->high changes
  attachInterrupt(digitalPinToInterrupt(BUTTON), encoderButtonISR, FALLING); //call encoderButtonISR() every high->low              changes
  
  server.on("/", HTTP_GET, handleRoot);     
  server.on("/DOOR", HTTP_GET, handleDoor);  
  server.on("/LIGHT", HTTP_GET, handleLight); 
  server.on("/status", HTTP_GET, apiStatus); 
  server.on("/calibrate", HTTP_GET, calibrate); 
  server.on("/restart", HTTP_GET, restartESP); 
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Server started");
  
  digitalWrite(lightSwitch, HIGH);
  digitalWrite(doorSwitch, HIGH);
  
}

void loop() {
  server.handleClient(); 
  
  if (position != encoder.getPosition())
  {
    position = encoder.getPosition();
    Serial.println(position);
  }
  
  if (encoder.getPushButton() == true) Serial.println(F("PRESSED"));         //(F()) saves string to flash & keeps dynamic memory free
  
  if(WiFi.status() != WL_CONNECTED){
    int j = 0;
      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      delay(5000);
      ESP.restart();
    }
  }
}

void connect(){
  int i = 0;

  WiFi.config(ip, gateway, subnet);
  delay(500);
  WiFi.disconnect(true);
  delay(1000);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    //Serial.println(++i);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
  }
    //MDNS.begin("garage");
  
  Serial.println("Connection established!");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void handleRoot() {  
  File webpageFile = SPIFFS.open("/index.html", "r");
  server.streamFile(webpageFile, "text/html");
  //server.send(200, "text/html", webpageFile);
  webpageFile.close();
}

void handleDoor() {                          
  digitalWrite(doorSwitch,!digitalRead(doorSwitch));
  doorOpen = !doorOpen;
  if(server.arg("api").toInt()!=1){
    redirect();
  }else{
    server.send(200,"text/plain", "Done");
  }
  delay(200);
  digitalWrite(doorSwitch,!digitalRead(doorSwitch));
}

void handleLight() {              
  lightOn = !lightOn;
  digitalWrite(lightSwitch,!digitalRead(lightSwitch));
  delay(50);
  if(server.arg("api").toInt()!=1){
    redirect();
  }else{
    server.send(200,"text/plain", "Done"); 
  }
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found");
}

void redirect(){
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void apiStatus(){
  server.send(200,"text/plain", String(doorOpen) + "," + String(lightOn));
  //server.send(200,"text/plain", String(doorOpen) + "," + String(lightOn) + "," + String(raw));
}

void calibrate(){
  doorOpen = false;
  lightOn = false;
  server.send(200,"text/plain", String(doorOpen) + "," + String(lightOn));
}

void restartESP(){
    server.send(200,"text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
}

void encoderISR(){
  encoder.readAB();
}

void encoderButtonISR(){
  encoder.readPushButton();
}
