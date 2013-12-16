/*
  Flip Flop
  switching between two LEDs using a button
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int buttonPin=2;                       // Pin where the button is attached
int greenLED=9;                        // Pin where the green LED is attached
int redLED=10;                         // Pin where the red LED is attached 

void setup(){
  pinMode(buttonPin, INPUT);           // button is initialised as INPUT 
  pinMode(greenLED, OUTPUT);           // green LED pin is initialised as OUTPUT 
  pinMode(redLED, OUTPUT);             // red LED pin is initialised as OUTPUT  
}

void loop(){
  if (digitalRead(buttonPin)==LOW){    // if the button is pressed (LOW) 
    digitalWrite(greenLED, HIGH);      // switch on green LED
    digitalWrite(redLED, LOW);         // switch off red LED
  } else {                             // else (if button is not pressed)
    digitalWrite(greenLED, LOW);       // switch off green LED
    digitalWrite(redLED, HIGH);        // switch on red LED
  }
}
