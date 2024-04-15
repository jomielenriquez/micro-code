#include <SoftwareSerial.h>
#include <TinyGPS++.h>

TinyGPSPlus gps;  // The TinyGPS++ object
SoftwareSerial GPS(5, 6); // The serial connection to the GPS device
SoftwareSerial SIM800L(7, 8); // GSM module connection
String MobileNumber = "+639568774952";
bool locationRequested = false; // Flag to track if location request has been received

void setup() {
  Serial.begin(9600);
  GPS.begin(9600);
  delay(1000);
  SIM800L.begin(9600);
  
  Serial.print("Connecting to network");
  while (true){
    SIM800L.println("AT+CREG?");
    Serial.print(".");
    if(!isConnected()){ delay(1000); }
    else { break; }
  }

  SIM800L.println("AT+CMGF=1");
  delay(1000);
  SIM800L.println("AT+CNMI=1,2,0,0,0");
}

void loop() {
  // Check for incoming SMS messages
  if (SIM800L.available()) {
    String message = SIM800L.readStringUntil('\n');
    Serial.println(message);
    getnumber(message);
    if (message.indexOf("LOCATION") != -1) {
      locationRequested = true;
      Serial.println("Location requested...");
    }
  }

  // If location requested, send the latitude and longitude
  if (locationRequested) {
    sendLocation();
    locationRequested = false; // Reset flag
  }
  //Read GPS data
  while (GPS.available()) {
    if (gps.encode(GPS.read())) {
      if (gps.location.isValid()) {
        Serial.print("Latitude: ");
        Serial.print(gps.location.lat(), 6);
        Serial.print(", Longitude: ");
        Serial.println(gps.location.lng(), 6);
      }
    }
  }
}

bool isConnected() {
  delay(500);
  while (true){
    if (SIM800L.available()) {
      String message = SIM800L.readStringUntil('\n');
      Serial.println("'" + message + "'");
      if (message.indexOf("+CREG: 0,1") != -1) {
        Serial.println("GSM Connected");
        return true;
      }
      else{
        return false;
      }
    }
    break;
  }
  return false;
}

void sendLocation() {
  String latStr = "";
  String lngStr = "";
  
  GPS.begin(9600);

  Serial.println("waiting for GPS");
  while (true) {
    GPS.begin(9600);
    if (gps.encode(GPS.read())) {
      if (gps.location.isValid()) {
        Serial.print("Latitude: ");
        Serial.print(gps.location.lat(), 6);
        Serial.print(", Longitude: ");
        Serial.println(gps.location.lng(), 6);
        latStr = String(gps.location.lat(), 6);
        lngStr = String(gps.location.lng(), 6);
        break;
      }
    }
  }

  SIM800L.begin(9600);
  Serial.println("GPS AVAILABLE!!");
  Serial.println("Sending");

  // Send the message
  SIM800L.println("AT+CMGF=1\r"); // Set SMS mode to text
  delay(1000);
  SIM800L.println("AT+CMGS=\"" + MobileNumber + "\"");
  delay(1000);
  SIM800L.print(latStr);
  SIM800L.print(",");
  SIM800L.println(lngStr);
  delay(1000);
  SIM800L.write(26);
  // SIM800L.write(0x1A); // End message with Ctrl+Z
  delay(1000);
  
  SIM800L.begin(9600);
  while(true){
    if (SIM800L.available()) {
      String message = SIM800L.readStringUntil('\n');
      Serial.println("'" + message + "'");
      if (message.indexOf("+CMGS") != -1) {
        Serial.println("Sending End..");
        break;
      }
    }
  }

  SIM800L.begin(9600);

  Serial.print("Connecting to network");
  while (true){
    SIM800L.println("AT+CREG?");
    Serial.print(".");
    if(!isConnected()){ delay(1000); }
    else { break; }
  }
  
  SIM800L.println("AT+CMGF=1");
  delay(1000);
  SIM800L.println("AT+CNMI=1,2,0,0,0");
}

void getnumber(String input){
  int startPos = input.indexOf('+');
  
  // If "+"" is found
  if (startPos != -1) {
    // Extract the substring starting from the position of "+"
    String phoneNumber = input.substring(startPos);
    
    // Find the position of the first occurrence of ","
    int endPos = phoneNumber.indexOf(',');
    
    // If "," is found
    if (endPos != -1) {
      // Extract the substring from the position of "+" to the position of ","
      phoneNumber = phoneNumber.substring(7, endPos - 1);
      
      // Print the extracted phone number
      Serial.println(phoneNumber);
      MobileNumber = phoneNumber;
    }
  }
}

// WIRING

// GPS
// TX => lilypad 5
// RX => lilypad 6
// VCC => lilypad +
// GND => lilypad -

// GSM

// RXD => lilypad 7
// TXD => lilypad 8
// VCC => 3.4v
// GND => GND

// USB CONNECTION
// USB DTR => LEFT SIDE