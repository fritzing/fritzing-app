/*
  Servo
  ein Potentiometer steuert die Position eines Servos
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

#include <Servo.h>                   // einbinden der Servo Bibliothek (library)
 
Servo myservo;                       // erzeugt ein Servo-Objekt
int potpin = 0 ;                     // Potentiometer-Pin wird deklariert
int val;                             // val speichert den Wert des Potentiometers zwischen
 
void setup() 
{ 
  myservo.attach(9);                  // verbindet das Servoobjekt an Pin 9
} 
 
void loop() 
{ 
  val = analogRead(potpin);            // Potentiometer-Wert wird ausgelesen
  val = map(val, 0, 1023, 0, 179);     // und in den Wertebereich von 0 bis 179 überführt
  myservo.write(val);                  // stellt den Servo auf den Wert val
  delay(15);                           // wartet, damit sich der Servo drehen kann
} 
