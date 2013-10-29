/*
  chaserlight from both sides
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int pinCount=8;                        // declaring the integer variable pinCount
int pins[] = {2,3,4,5,6,7,8,9};          // declaring the array pins[]

void setup() {                
  for (int i=0; i<pinCount; i=i+1){    // counting the variable i from 0 to 7
    pinMode(pins[i], OUTPUT);            // initialising the pin at index i of the array of pins as OUTPUT
  }
}

void loop() {
  for (int i=0; i<4; i=i+1){             // chasing right
    digitalWrite(pins[i], HIGH);         // switching the LED at position i on
    digitalWrite(pins[7-i], HIGH);       // switching the LED at position 7-i on   
    delay(100);                          // stopping the program for 100 milliseconds
    digitalWrite(pins[i], LOW);          // switching the LED at position i off
    digitalWrite(pins[7-i], LOW);        // switching the LED at position 7-i off    
  }
  for (int i=2; i>=1; i=i-1){            // chasing left (except the outer leds)
    digitalWrite(pins[i], HIGH);         // switching the LED at position i on
    digitalWrite(pins[7-i], HIGH);       // switching the LED at position 7-i on
    delay(100);                          // stopping the program for 100 milliseconds
    digitalWrite(pins[i], LOW);          // switching the LED at position i off
    digitalWrite(pins[7-i], LOW);        // switching the LED at position 7-i off   
  }
}

