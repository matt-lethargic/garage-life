#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

// network
char* ssid     = "galaxyfaraway";
char* password = "Starship69";

// Set web server port number to 80
AsyncWebServer server(80);

// inputs
int latchPin_IP = D6; // Pin 15
int clockPin_IP = D5; // Pin 7
int dataPin_IP = D7; // Pin 13

// outputs (595 shift register)
int latchPin_SIPO = D1; // D5;
int clockPin_SIPO = D2; //D6;
int dataPin_SIPO = D3; //D4;

// vars
byte leds = 0;



void setup() {
  Serial.begin(9600);

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Pins for output shift register
  pinMode(latchPin_SIPO, OUTPUT);
  pinMode(dataPin_SIPO, OUTPUT);
  pinMode(clockPin_SIPO, OUTPUT);

  // Pins for input shift register
  pinMode(latchPin_IP, OUTPUT);
  pinMode(dataPin_IP, INPUT);
  pinMode(clockPin_IP, OUTPUT);


  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/bulb.svg", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bulb.svg", "text/html");
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncJsonResponse * response = new AsyncJsonResponse();
    const JsonObject& root = response->getRoot();

    const JsonArray& lights = root.createNestedArray("lites");
    for (int i = 7; i >= 0; i--)
    {
      bool b = bitRead(leds, i);
      const JsonObject object = lights.createNestedObject();
      object[String(i)] = b;
    }

    response->setLength();
    request->send(response);
  });

  // Light on route
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/lite", [](AsyncWebServerRequest * request, JsonVariant & json) {

    const JsonObject& obj = json.as<JsonObject>();
    const char* lite = obj["lite"];
    const int light = obj["lite"].as<int>();
    const bool state = obj["status"].as<bool>();

    switchLight(light, state);

    AsyncJsonResponse * response = new AsyncJsonResponse();
    const JsonObject& root = response->getRoot();
    root["lite"] = ESP.getFreeHeap();
    root["ssid"] = WiFi.SSID();
    response->setLength();
    request->send(response);

  });
  server.addHandler(handler);

  server.begin();
}

int j;
byte switchVar = 0;


void loop() {
  byte dataIn = 0;
  digitalWrite(latchPin_IP, LOW);
  digitalWrite(clockPin_IP, LOW);
  digitalWrite(clockPin_IP, HIGH);
  digitalWrite(latchPin_IP, HIGH);

  byte incoming = shiftIn(dataPin_IP, clockPin_IP, MSBFIRST);
  if (switchVar != incoming) {

    for (int j = 0; j < 8; j++) {
      int t = bitRead(incoming, j);
      if (t == 1) {
        toggleLight(j);
        printByte(incoming);
      }
    }
    switchVar = incoming;
  }

  delay(100);
}

void toggleLight(int light) {
  int state = bitRead(leds, light);
  if (state == 0) {
    switchLight(light, true);
  } else {
    switchLight(light, false);
  }
}

void switchLight(int light, bool state) {
  if (state) {
    bitSet(leds, light);
  } else {
    bitClear(leds, light);
  }

  printByte(leds);
  updateShiftRegister();
}

void updateShiftRegister() {
  digitalWrite(latchPin_SIPO, LOW);
  shiftOut(dataPin_SIPO, clockPin_SIPO, MSBFIRST, leds);
  digitalWrite(latchPin_SIPO, HIGH);
}

void printByte(byte bi) {
  for (int i = 7; i >= 0; i--)
  {
    bool b = bitRead(bi, i);
    Serial.print(b);
  }

  Serial.println(" ");
}
