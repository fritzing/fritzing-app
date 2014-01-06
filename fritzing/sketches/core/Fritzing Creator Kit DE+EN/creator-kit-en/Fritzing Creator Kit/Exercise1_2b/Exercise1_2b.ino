/*
  Exercise 1_2 Blink SOS an alternative way
  This program blinks SOS in Morse code with an LED:
  * short-short-short = S
  * long-long-long = O
  * short-short-short = S
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/


int led = 13;                               // integer variable led is declared
char signals[] = {'.','.','.','-','-','-','.','.','.'}; // variable storing the signals: . = short, - = long

void setup() {                              // the setup() method is executed only once
  pinMode(led, OUTPUT);                     // the led PIN is declared as digital output
}

void loop() {                               // the loop() method is repeated
  for (int i=0; i<sizeof(signals); i++){    // repeat as often as the array has got entries (here 9 times)
    digitalWrite(led,HIGH);                 // switching on the led
    if (signals[i]=='.') delay(200);        // if the array contains an "." at position i wait short
    if (signals[i]=='-') delay(500);        // if the array contains an "-" at position i wait long
    digitalWrite(led,LOW);                  // switching off the led
    if (signals[i]=='.') delay(200);        // if the array contains an "." at position i wait short
    if (signals[i]=='-') delay(500);        // if the array contains an "-" at position i wait long
  }
  delay(2000);                              // stopping the program for 2000 milliseconds to have a longer break
}
