#include <Servo.h>

Servo servo;

String serialMessage = "";
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial.println("Initializing");

  servo.attach(3);
}

void loop() {
  // put your main code here, to run repeatedly:
  // if (Serial3.available() > 0){
  //   Serial.println("Available");
  //   char ch = Serial3.read();     // should 0 or 1 to blient LED
  //   Serial.print(ch);
  // }
  bool isOpen = false;
  while(Serial3.available()){
    char c = (char)Serial3.read();
    if(c == '\n'){
      Serial.println("Message: '" + serialMessage + "'");
      serialMessage = "";
    }
    else{
      serialMessage += c;
    }
    // Serial.print();
  }

  // if(isOpen){
  //   servo.write(0); // move MG996R's shaft to angle 0°
  //   delay(1000); // wait for one second
  //   servo.write(45); // move MG996R's shaft to angle 45°
  //   delay(1000); // wait for one second 
  //   servo.write(90); // move MG996R's shaft to angle 90°
  //   delay(1000); // wait for one second
  //   servo.write(135); // move MG996R's shaft to angle 135°
  //   delay(1000); // wait for one second
  //   servo.write(180); // move MG996R's shaft to angle 180°
  //   delay(1000); // wait for one second
  // }
}
