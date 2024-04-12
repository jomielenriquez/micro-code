#include <ArduinoJson.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "HX711.h"

#define LOADCELL_DOUT_PIN  3
#define LOADCELL_SCK_PIN  2

HX711 scale;

float calibration_factor = -284; //-7050 worked for my 440lb max scale setup
float units;

Servo servo;
SoftwareSerial esp8266(10,11);

String serialMessage = "";
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  esp8266.begin(9600);
  Serial.println("Initializing");

  servo.attach(4);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare(); //Reset the scale to 0
}

void loop() {
  bool isOpen = false;
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
    }
    else{
      Serial.println("Message: '" + serialMessage + "'");
    }

    serialMessage = "";
  }
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