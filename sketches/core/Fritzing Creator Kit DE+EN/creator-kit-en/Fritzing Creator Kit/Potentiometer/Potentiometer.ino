/*
  potentiometer
  two LEDs are controlled by a potentiometer
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int ledGreen = 6;                            // LED pin is declared
int ledRed = 5;                              // LED pin is declared
int potPin = 0;                              // potentiometer pin is declared

void setup(){
                                             // neither analog inputs nor outputs have to be declared in the setup
}

void loop(){
  int value = analogRead(potPin);            // this variable stores the value from the potentiometer
  int redValue = map(value,0,1023,0,255);    // mapped from a range of 0 to 1023 to a range of 0 to 255 for the red LED,
  int greenValue = map(value,0,1023,255,0);  // and for the green mapped to the reverse range from 255 to 0
  
  analogWrite(ledRed,redValue);              // the recalculated values are
  analogWrite(ledGreen,greenValue);          // send to the LEDs as analog values
}


