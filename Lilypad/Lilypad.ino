#include <SoftwareSerial.h>
#include <TinyGPS++.h>

TinyGPSPlus gps;  // The TinyGPS++ object
SoftwareSerial ss(4, 5); // The serial connection to the GPS device
SoftwareSerial mySerial(10, 11); // GSM module connection
String MobileNumber = "+639517108081";
bool locationRequested = false; // Flag to track if location request has been received

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  ss.begin(9600);
}

void loop() {
  // Check for incoming SMS messages
  if (mySerial.available()) {
    String message = mySerial.readStringUntil('\n');
    Serial.println(message);
    delay(2000);
    if (message.indexOf("GET_LOCATION") != -1) {
      locationRequested = true;
      Serial.println("Location requested...");
    }
  }

  // If location requested, send the latitude and longitude
  if (locationRequested) {
    sendLocation();
    locationRequested = false; // Reset flag
  }

  // Read GPS data
  while (ss.available()) {
    if (gps.encode(ss.read())) {
      if (gps.location.isValid()) {
        Serial.print("Latitude: ");
        Serial.print(gps.location.lat(), 6);
        Serial.print(", Longitude: ");
        Serial.println(gps.location.lng(), 6);
      }
    }
  }
}

void sendLocation() {
  Serial.println("sending");
  String latStr = String(gps.location.lat(), 6);
  String lngStr = String(gps.location.lng(), 6);
  String message = "Latitude: " + latStr + ", Longitude: " + lngStr;

  // Send the message
  Serial.println("AT+CMGF=1"); // Set SMS mode to text
  mySerial.println("AT+CMGF=1"); // Set SMS mode to text
  delay(1000);
  Serial.println("AT+CMGS=\"" + MobileNumber + "\""); // Specify recipient phone number
  mySerial.println("AT+CMGS=\"" + MobileNumber + "\""); // Specify recipient phone number
  delay(1000);
  Serial.println(message); // Send message content
  mySerial.println(message); // Send message content
  delay(100);
  mySerial.write(26); // End message with Ctrl+Z
  delay(1000);
}

// WIRING

// GPS
// TX => lilypad 4
// RX => lilypad 5
// VCC => lilypad +
// GND => lilypad -

// GSM

// RXD => lilypad 10
// TXD => lilypad 11
// VCC => 3.4v
// GND => GND