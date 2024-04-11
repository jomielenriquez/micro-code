#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> // Include LittleFS library instead of SPIFFS
#include <Wire.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "Last_Warning_2.4G";
const char* password = "Wisbagslots121625";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void overwriteJsonFile(const char* filename, String datastring) {
  // Open the file for writing
  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Write the JSON string to the file
  file.print(datastring);

  // Close the file
  file.close();

  Serial.println("File updated successfully");
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(9600);

  // Initialize LittleFS
  if (!LittleFS.begin()) { // Use LittleFS instead of SPIFFS
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  IPAddress ip(192, 168, 1, 10); // Desired fixed IP address
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.config(ip, gateway, subnet);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // FOR TESTING -- NEED TO DELETE
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/test.html");
  });

  server.on("/SendMessage", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("SampleMessage");
    request->send(LittleFS, "/test.html");
  });

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/index.html");
  });

  server.on("/vetAppointmentsData", HTTP_GET, [](AsyncWebServerRequest* request) {
    File file = LittleFS.open("/vetAppointmentsData.json", "r");
    if (!file) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    StaticJsonDocument<200> jsonDocument;
    DeserializationError error = deserializeJson(jsonDocument, file);
    file.close();

    if (error) {
      request->send(500, "text/plain", "Failed to parse JSON");
      return;
    }

    String jsonString;
    serializeJson(jsonDocument, jsonString);
    Serial.printf(jsonString.c_str());
    Serial.println();
    request->send(200, "application/json", jsonString);
  });

  server.on("/schedulingData", HTTP_GET, [](AsyncWebServerRequest* request) {
    File file = LittleFS.open("/schedulingData.json", "r");
    if (!file) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    StaticJsonDocument<200> jsonDocument;
    DeserializationError error = deserializeJson(jsonDocument, file);
    file.close();

    if (error) {
      request->send(500, "text/plain", "Failed to parse JSON");
      return;
    }

    String jsonString;
    serializeJson(jsonDocument, jsonString);
    Serial.println(jsonString);
    request->send(200, "application/json", jsonString);
  });

  // Route for handling POST requests
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("data", true)) {
      AsyncWebParameter* dataParam = request->getParam("data", true);
      String datastring = dataParam->value();
      
      overwriteJsonFile("/vetAppointmentsData.json", datastring);
    } else {
      Serial.println("No datetimeInput parameter found");
    }

    request->send(200, "application/json", "[{\"status\":\"success\"}]");
  });

  // Route for handling POST requests
  server.on("/postSchedule", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("data", true)) {
      AsyncWebParameter* dataParam = request->getParam("data", true);
      String datastring = dataParam->value();
      
      overwriteJsonFile("/schedulingData.json", datastring);
    } else {
      Serial.println("No datetimeInput parameter found");
    }

    request->send(200, "application/json", "[{\"status\":\"success\"}]");
  });

  // Start server
  server.begin();
}

void loop() {
}
