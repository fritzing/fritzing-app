/*
  Servo
  a potentiometer controls the position of a servo
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

#include <Servo.h>                   // include the library Servo.h
 
Servo myservo;                       // creates a servo object
int potpin = 0 ;                     // potentiometer pin declaration
int val;                             // val stores the position of the potentiometer
 
void setup() 
{ 
  myservo.attach(9);                  // connects a servo object to pin 9
} 
 
void loop() 
{ 
  val = analogRead(potpin);            // potentiometer value is read out
  val = map(val, 0, 1023, 0, 179);     // and mapped to the range 0 to 179
  myservo.write(val);                  // turning the servo to the angle in val
  delay(15);                           // give the servo time to react
} 
