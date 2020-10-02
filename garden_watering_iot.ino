#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include "ESPAsyncWebServer.h"

//On board LED Connected to GPIO2
#define LED 2  
#define HUMIDITY_SENSOR A0  
#define AUTOMATIC_MODE "auto"
#define MANUAL_MODE "manual"
#define CONTROL_INTERVAL 1000

int ledStatus = LOW;

const char* ssid = "NETVIRTUA AP.302";
const char* password = "25408195";

String webPage = "";

//Automatic control function interval variables
unsigned long lastAutoControlTime;

//Automatic control targets
String controlMode = MANUAL_MODE;
int targetHumidity = 0;

AsyncWebServer server(80);

void handleRoot(AsyncWebServerRequest *request) {
  request->send(200, "text/html", webPage);
}

void handleGetHumidity(AsyncWebServerRequest *request) {
  char humidityBuffer[3];
  itoa(getHumidity(), humidityBuffer, 10);
  request->send(200, "text/plain", humidityBuffer);
}

void handleControlValve(AsyncWebServerRequest *request) {
  int paramsNumber = request->params();
  if(paramsNumber > 3 || paramsNumber == 0){
    //TODO Throw exception
    Serial.println("ERROR - Invalid number of query parameters passed to /control-valve.");
    return;
  }
  
  AsyncWebParameter* valveMode;
  AsyncWebParameter* valveState;
  AsyncWebParameter* humidity;

  for(int i = 0; i < paramsNumber; i++){
    AsyncWebParameter* param = request->getParam(i);
    
    if(param->name() == "mode"){
      valveMode = param;
    }
    else if(param->name() == "state"){
      valveState = param;
    }
    else if(param->name() == "humidity"){
      humidity = param;
    }
    else {
      //TODO throw Exception
      Serial.println("ERROR - Invalid query parameter passed to /control-valve.");
      return;
    }
  }

  if(valveMode->value() == MANUAL_MODE){ 
    controlMode = MANUAL_MODE;
    targetHumidity = 0;
    toggleValve(valveState->value().toInt());
    String stateString = valveState->value().toInt() ? "ON" : "OFF";
    request->send(200, "text/plain", "Automatic control turned OFF. Valve turned " + stateString + " via manual mode.");
  }
  else if(valveMode->value() == AUTOMATIC_MODE){
    controlMode = valveState->value().toInt() ? AUTOMATIC_MODE : MANUAL_MODE;
    targetHumidity = humidity->value().toInt();
    String stateString = valveState->value().toInt() ? "ON" : "OFF";
    request->send(200, "text/plain", "Automatic control turned " + stateString + ".");
  }
  else {
    Serial.println("ERROR - Valve operating mode not recognized.");
  }
}

int getHumidity() {
  //TODO Improve sensor range mapping (does not go to 0 apparently)
  return map(analogRead(HUMIDITY_SENSOR), 1024, 0, 0, 100);
}

void automaticControl() {
  
    Serial.print("Executing automatic control at time: ");
    Serial.println(millis());

    int newValveState = LOW;
    if((targetHumidity > realHumidity - HUMIDITY_DIFF_THRESHOLD) && (targetHumidity < realHumidity + HUMIDITY_DIFF_THRESHOLD)) {
      newValveState = LOW;
    }
    else if(targetHumidity < realHumidity - HUMIDITY_DIFF_THRESHOLD) {
      newValveState = LOW;
    }
    else if(targetHumidity > realHumidity + HUMIDITY_DIFF_THRESHOLD) {
      newValveState = HIGH;
    }
    else {
      newValveState = LOW;
    }

    toggleValve(newValveState);
}

void toggleValve(int state) {
  Serial.print("Controlling mock valve... Status is: ");
  Serial.println(state);
  digitalWrite(LED, state);
}

void readFile(void) {
  //Faz a leitura do arquivo HTML
  File rFile = SPIFFS.open("/index.html", "r");
  Serial.println("Lendo arquivo HTML...");
  webPage = rFile.readString();
  yield();
  //Serial.println(webPage);
  rFile.close();
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, ledStatus);
  
  pinMode(A0, INPUT);
  
  Serial.begin(9600);
  SPIFFS.begin();

  if (SPIFFS.exists("/index.html"))
  {
    Serial.println("\n\nfile exists!");
  }
  else Serial.println("\n\nNo File :(");

  readFile();

  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/humidity", HTTP_GET, handleGetHumidity);
  server.on("/control-valve", HTTP_GET, handleControlValve);

  server.begin();
}

void loop() {
  unsigned long currentTime = millis();
  if(controlMode == AUTOMATIC_MODE && currentTime >= lastAutoControlTime + CONTROL_INTERVAL){
    lastAutoControlTime = currentTime;
    automaticControl();
  }
}
