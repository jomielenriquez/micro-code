#include <ArduinoJson.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "HX711.h"

#define LOADCELL_DOUT_PIN  3
#define LOADCELL_SCK_PIN  2

#define DOGULTRASONICTRIG 8
#define DOGULTRASONICECHO 9

#define FOODULTRASONICTRIG 6
#define FOODULTRASONICECHO 7

#define WATERPOWER 13
#define WATERSENSOR 12

HX711 scale;

float calibration_factor = -284; //-7050 worked for my 440lb max scale setup
float units;

// dogWait is the duration of time that the system will wait for the dog
// if the dog was not detected after dogWait, it will be considered as fail dispense
int dogWait = 10000;

int statusCount = 0;
int statusCountLimit = 5000; // update every 5 sec

Servo servo;
SoftwareSerial esp8266(10,11);

String serialMessage = "";
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  esp8266.begin(19200);
  Serial.println("Initializing");

  servo.attach(4);

  // Setup Dog trigger
  pinMode(DOGULTRASONICTRIG, OUTPUT);
  pinMode(DOGULTRASONICECHO, INPUT);

  // Setup Food trigger
  pinMode(FOODULTRASONICTRIG, OUTPUT);
  pinMode(FOODULTRASONICECHO, INPUT);

  // Setup Water Sensor
  pinMode(WATERPOWER, OUTPUT);
  pinMode(WATERSENSOR, INPUT);
  digitalWrite(WATERPOWER, LOW);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare(); //Reset the scale to 0
}

void loop() {
  bool isOpen = false;
  
  float dogDistance = getDogDistance();
  Serial.print("Dog Distance: ");
  Serial.print(dogDistance);
  
  Serial.print(", Food Distance: ");
  Serial.print(getFoodDistance());

  // zero means there is water
  Serial.print(", Water Reading: ");
  Serial.println(readWaterSensor());

  if(statusCount >= statusCountLimit){
    Serial.print("passing status");
    Serial.print("{\"message\": \"UPDATE\",\"amount\": 0,\"water\":");
    Serial.print(readWaterSensor());
    Serial.print(",\"food\":");
    Serial.print(getFoodDistance());
    Serial.println("}");
    // below is the sample message to update food and water status
    // "{\"message\": \"UPDATE\",\"amount\": 0,\"water\": 0,\"food\": 0}"
    esp8266.print("{\"message\": \"UPDATE\",\"amount\": 0,\"water\":");
    esp8266.print(readWaterSensor());
    esp8266.print(",\"food\":");
    esp8266.print(getFoodDistance());
    esp8266.println("}");
    statusCount = 0;
  }

  while(esp8266.available()){
    char c = (char)esp8266.read();
    Serial.print(c);
    if(c == '\n'){
      isOpen = true;
    }
    else{
      serialMessage += c;
    }
    // Serial.print();
  }

  if(isOpen){
    DynamicJsonDocument doc(1024);

    deserializeJson(doc, serialMessage);
    String docMessage = doc["message"];
    String from = doc["from"];
    String to = doc["to"];
    if(docMessage != "null" && from.toInt() > 0 && to.toInt() > 0){
      Serial.println(from.toInt());

      Serial.println("Message: '" + docMessage + "'");
      
      Serial.print("Reading: "); 
      Serial.print(getWeight());
      Serial.println(" grams");

      bool isDone = false;

      // after the schedule is sent from esp it will wait for the dog to come near the utrasonic sensor
      int currentWaitingTime = 0;
      Serial.print("Waiting for the dog");
      while(currentWaitingTime < dogWait){
        dogDistance = getDogDistance();
        if(dogDistance < 7){
          break;
        }
        Serial.print(".");
        delay(1000);
        currentWaitingTime += 1000;
      }
      Serial.println(". Dog detected");

      if(dogDistance < 7){
        while((int)getWeight() < from.toInt()){
          
          Serial.print("Reading: "); 
          Serial.print(getWeight());
          Serial.println(" grams");

          servo.write(0);
          delay(1000);
          servo.write(90);
          delay(1000);

          isDone = true;
        }
        if(isDone){
          int amount = (int)getWeight();
          esp8266.print("{\"message\": \"POST\",\"amount\": ");
          esp8266.print(amount);
          esp8266.println(",\"remarks\": \"ok\"}");
        }
        else{
          // food is still full
          // feeder did not detect the dog.
          int scheduledAmount = from.toInt() * -1;
          esp8266.print("{\"message\": \"POST\",\"amount\": ");
          esp8266.print(scheduledAmount);
          esp8266.print(",\"remarks\": \" Did not dispense at ");
          esp8266.print(docMessage);
          esp8266.print(" (");
          esp8266.print(from);
          esp8266.print(") because current weight is ");
          esp8266.print((int)getWeight());
          esp8266.print(" grams");
          esp8266.println("\"}");
        }
      }
      else{
        // feeder did not detect the dog.
        int scheduledAmount = from.toInt() * -1;
        esp8266.print("{\"message\": \"POST\",\"amount\": ");
        esp8266.print(scheduledAmount);
        esp8266.print(",\"remarks\": \" Did not dispense at ");
        esp8266.print(docMessage);
        esp8266.println(" because dog was not detected.\"}");
      }
      Serial.println("Dispense Done");
      // delay 60000 or 1 minute after dispense process
      delay(60000);
    }
    else{
      Serial.println("Message: '" + serialMessage + "'");
    }
    finished:
    serialMessage = "";
  }
  delay(1000);
  statusCount = statusCount + 1000;
}

int readWaterSensor(){
  digitalWrite(WATERPOWER, HIGH);
  delay(10);
  int val = digitalRead(WATERSENSOR);
  digitalWrite(WATERPOWER, LOW);
  return val;
}

float getWeight(){
  units = scale.get_units() * -1, 10;
  float ounces;
  if (units < 0)
  {
    units = 0.00;
  }
  ounces = units * 0.035274;
  return units;
}

float getDogDistance(){
  digitalWrite(DOGULTRASONICTRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(DOGULTRASONICTRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(DOGULTRASONICTRIG, LOW);
  long duration = pulseIn(DOGULTRASONICECHO, HIGH);
  return duration * 0.0133 / 2;
}

float getFoodDistance(){
  digitalWrite(FOODULTRASONICTRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(FOODULTRASONICTRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(FOODULTRASONICTRIG, LOW);
  long duration = pulseIn(FOODULTRASONICECHO, HIGH);
  return duration * 0.0133 / 2;
}

// LOAD CELL
// DT => 3
// SCK => 2
// 5V


// Servo
// RED => 5V
// BROWN => GND
// YELLOW => 4


// ESP8266
// ESP TX => 10
// ESP RX => 11
// 3.3V


