/*
  Blink
  
  Switching a LED on and off
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int led = 13;                   // integer variable led is declared

void setup() {                  // the setup() method is executed only once
  pinMode(led, OUTPUT);         // the led PIN is declared as digital output
}

void loop() {                   // the loop() method is repeated
  digitalWrite(led, HIGH);      // switching on the led 
  delay(1000);                  // stopping the program for 1000 milliseconds
  digitalWrite(led, LOW);       // switching off the led 
  delay(1000);                  // stopping the program for 1000 milliseconds
}
