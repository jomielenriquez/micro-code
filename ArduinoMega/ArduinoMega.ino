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

Servo servo;
SoftwareSerial esp8266(10,11);

String serialMessage = "";
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  esp8266.begin(9600);
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
  
  /* 
  Create a variable that will hold the next schedule
  when the dog is need, check the next schedule
  */
  float dogDistance = getDogDistance();
  Serial.print("Dog Distance: ");
  Serial.println(dogDistance);
  if(dogDistance < 5){
    isOpen = true;
    serialMessage = "{\"message\": \"Detected.\"}";
  }

  Serial.print("Food Distance: ");
  Serial.println(getFoodDistance());

  // zero means there is water
  Serial.print("Water Reading: ");
  Serial.println(readWaterSensor());

  while(esp8266.available()){
    char c = (char)esp8266.read();
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
    if(docMessage != "null"){
      Serial.println("Message: '" + docMessage + "'");
      
      Serial.print("Reading: "); 
      Serial.print(getWeight());
      Serial.println(" grams");

      servo.write(0); // move MG996R's shaft to angle 180°
      delay(1000); // wait for one second
      servo.write(90);
      delay(1000); // wait for one second
      servo.write(0); // move MG996R's shaft to angle 180°
      delay(1000); // wait for one second
      servo.write(90);
      delay(1000); // wait for one second
    }
    else{
      Serial.println("Message: '" + serialMessage + "'");
    }

    serialMessage = "";
  }
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


