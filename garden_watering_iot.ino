#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

//On board LED Connected to GPIO2
#define LED 2  

int ledStatus = LOW;

const char* ssid = "NETVIRTUA AP.302";
const char* password = "25408195";

String webPage = "";

ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", webPage);
}

void handleToggleLight() {
  Serial.print("Controlling LED... Status is: ");
  Serial.println(ledStatus);
  ledStatus = 1 - ledStatus;
  digitalWrite(LED, ledStatus);
  server.send(200, "application/json", "");
}

void handleGetHumidity() {
  int humidity = analogRead(A0);
  humidity = map(humidity, 854, 1024, 0, 100);
  char humidityBuffer[3];
  itoa(humidity, humidityBuffer, 10);
  server.send(200, "text/plain", humidityBuffer);
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

  server.on("/", handleRoot);
  server.on("/toggle-light", handleToggleLight);
  server.on("/humidity", handleGetHumidity);

  server.begin();
}

void loop() {
  server.handleClient();
}
