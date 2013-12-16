/*
  Exercise 1_2 Blink SOS
  This program blinks SOS in Morse code with an LED:
  * short-short-short = S
  * long-long-long = O
  * short-short-short = S
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int led = 13;                   // integer variable led is declared

void setup() {                  // the setup() method is executed only once
  pinMode(led, OUTPUT);         // the led PIN is declared as digital output
}

void loop() {                   // the loop() method is repeated
  digitalWrite(led, HIGH);      // switching on the led 
  delay(200);                   // stopping the program for 200 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(200);                   // stopping the program for 200 milliseconds
  
  digitalWrite(led, HIGH);      // switching on the led 
  delay(200);                   // stopping the program for 200 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(200);                   // stopping the program for 200 milliseconds
  
  digitalWrite(led, HIGH);      // switching on the led 
  delay(200);                   // stopping the program for 200 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(200);                   // stopping the program for 200 milliseconds
  

  digitalWrite(led, HIGH);      // switching on the led 
  delay(500);                   // stopping the program for 500 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(500);                   // stopping the program for 500 milliseconds
  
  digitalWrite(led, HIGH);      // switching on the led 
  delay(500);                   // stopping the program for 500 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(500);                   // stopping the program for 500 milliseconds
  
  digitalWrite(led, HIGH);      // switching on the led 
  delay(500);                   // stopping the program for 500 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(500);                   // stopping the program for 500 milliseconds
   
  
  digitalWrite(led, HIGH);      // switching on the led 
  delay(200);                   // stopping the program for 200 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(200);                   // stopping the program for 200 milliseconds
  
  digitalWrite(led, HIGH);      // switching on the led 
  delay(200);                   // stopping the program for 200 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(200);                   // stopping the program for 200 milliseconds
  
  digitalWrite(led, HIGH);      // switching on the led 
  delay(200);                   // stopping the program for 200 milliseconds
  digitalWrite(led, LOW);       // switching off the led
  delay(2000);                  // stopping the program for 2000 milliseconds to have a longer break
  }
