#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> // Include LittleFS library instead of SPIFFS
#include <Wire.h>
#include <ArduinoJson.h>
#include <RTClib.h>

RTC_DS3231 rtc;

char t[32]; // time holder
StaticJsonDocument<200> schedulingJson;

// Wifi Credentials
const char* ssid = "Last_Warning_2.4G";
const char* password = "Wisbagslots121625";

// Host of the data
const char* host = "dataapi.somee.com";
const int port = 80; // Use port 80 for HTTP

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

float foodDistance = 6;
int water = 0; // 0 means there is water and 1 means no water

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

  // Serial.println("File updated successfully");
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(19200);

  // Initialize LittleFS
  if (!LittleFS.begin()) { // Use LittleFS instead of SPIFFS
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // IPAddress ip(192, 168, 1, 10); // Desired fixed IP address
  // IPAddress gateway(192, 168, 1, 1);
  // IPAddress subnet(255, 255, 255, 0);

  // WiFi.config(ip, gateway, subnet);
  
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
    Serial.println("{\"message\": \"this is a testing\"}");
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
    // Serial.printf(jsonString.c_str());
    // Serial.println();
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
    schedulingJson = jsonDocument;
    serializeJson(jsonDocument, jsonString);
    // Serial.println(jsonString);
    request->send(200, "application/json", jsonString);
  });

  server.on("/NotifData", HTTP_GET, [](AsyncWebServerRequest* request) {
    File file = LittleFS.open("/notifData.json", "r");
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
    // Serial.println(jsonString);
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

  server.on("/getupdates", HTTP_GET, [](AsyncWebServerRequest* request) {
    String updates = "[{\"water\":" + String(water) + ", \"food\":" + String(foodDistance) + "}]";
    request->send(200, "application/json", updates);
  });

  // Route for handling POST requests
  server.on("/postSchedule", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("data", true)) {
      AsyncWebParameter* dataParam = request->getParam("data", true);
      String datastring = dataParam->value();

      StaticJsonDocument<200> jsonDocument;
      DeserializationError error = deserializeJson(jsonDocument, datastring);
      
      schedulingJson = jsonDocument;

      overwriteJsonFile("/schedulingData.json", datastring);
    } else {
      Serial.println("No datetimeInput parameter found");
    }

    request->send(200, "application/json", "[{\"status\":\"success\"}]");
  });

  // Start server
  server.begin();

  rtc.begin();

  // if you need to set the time, use rtc.adjust.
  // Below is an example of setting the time to April 22, 2024 - 9:39 PM
  // rtc.adjust(DateTime(2024, 4, 30, 9, 0, 0)); 
  DateTime now = rtc.now();  // Get initial time from RTC

  File file = LittleFS.open("/schedulingData.json", "r");
  if (file) {
    StaticJsonDocument<200> jsonDocument;
    DeserializationError error = deserializeJson(jsonDocument, file);
    file.close();

    if (!error) {
      schedulingJson = jsonDocument;
    }
  }
}

void loop() {
  if(Serial.available()){
    String message = Serial.readStringUntil('\n');
    DynamicJsonDocument serialMessage(1024);

    deserializeJson(serialMessage, message);
    //"{\"message\": \"POST\",\"amount\": 10}"
    if(serialMessage["message"] == "POST"){
      if(serialMessage["remarks"] != "ok"){
        insertNotification("Sched", serialMessage["remarks"]);
      }
      postDispense(serialMessage["amount"]);
      delay(60000);
    }
    else if(serialMessage["message"] == "UPDATE" && serialMessage["water"] >= 0 && serialMessage["food"] >= 0){
      // Update water and food distance
      water = serialMessage["water"];
      foodDistance = serialMessage["food"];
    }
  }
  // Handle outgoing connections
  // handleOutgoingConnections();

  DateTime rtcTime = rtc.now();
  // Format and print RTC time on Serial monitor
  sprintf(t, "RTC Time: %02d:%02d:%02d %02d/%02d/%02d", rtcTime.hour(), rtcTime.minute(), rtcTime.second(), rtcTime.day(), rtcTime.month(), rtcTime.year());
  // Serial.println(t);

  char currentTime[5];
  sprintf(currentTime, "%02d:%02d", rtcTime.hour(), rtcTime.minute());
  String stringCurrentTime = String(currentTime);

  String jsonString;
  serializeJson(schedulingJson, jsonString);
  // Serial.println(jsonString);

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonString);

  // Count the number of elements in the document
  int count = doc.size();
  for(int row = 0; row < count; row++){
    JsonObject object = doc[row];
    const char* timeValue = object["time"];
    String timeString = String(timeValue);
    // Serial.println(timeString);
    if(stringCurrentTime == timeString){
      Serial.println("{\"message\": \"Dispense Time: "+timeString+"\",\"from\": \""+String(object["amountFrom"])+"\",\"to\": \""+String(object["amountTo"])+"\"}");
    }
  }

  delay(1000); // Allow time for handling events
}
void handleOutgoingConnections() {
  // Create a WiFiClient object
  WiFiClient client;
  
  if (client.connect(host, port)) {
    Serial.println("Connected to server");

    // Send a simple HTTP GET request
    client.print("GET /api/dispense/ HTTP/1.1\r\n");
    client.print("Host: ");
    client.print(host);
    client.print("\r\n");
    client.print("Connection: close\r\n\r\n");

    // Read response
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
  } else {
    Serial.println("Connection failed");
  }

  // Close connection
  client.stop();
  Serial.println("Connection closed");
}
void postDispense(int amount) {
  // Create a WiFiClient object
  WiFiClient client;

  // Connect to the host on the specified port
  if (client.connect(host, port)) {
    Serial.println("Connected to server");

    String postData = "{ \"amount\": " + String(amount) + ", \"unit\":\"grams\", \"type\":\"food\" }"; // Your POST data

    // Send a simple HTTP GET request
    client.print("POST /api/dispense/ HTTP/1.1\r\n");
    client.print("Host: ");
    client.print(host);
    client.print("\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: ");
    client.print(postData.length());
    client.print("\r\n\r\n");
    client.print(postData);
    client.print("Connection: close\r\n\r\n");

    // Read response
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
  } else {
    Serial.println("Connection failed");
  }

  // Close connection
  client.stop();
  Serial.println("Connection closed");
}
void insertNotification(String type, String message){
  File file = LittleFS.open("/notifData.json", "r");
  
  if (file) {
    DynamicJsonDocument doc(500);
    DeserializationError error = deserializeJson(doc, file);

    file.close();

    if (!error) {
      JsonObject newObj = doc.createNestedObject();
      newObj["Type"] = type;
      newObj["Message"] = message;
      
      String jsonString;
      serializeJson(doc, jsonString);

      overwriteJsonFile("/notifData.json", jsonString);
    }
  }

  file.close();
}