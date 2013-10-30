/*
  fortune teller
  switching between five LEDs to tell the future
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creatorkit.
*/

int pins[] = {8,9,10,11,12};            // the pins of the LEDs are stored in an array
int tiltPin = 3;                        // pin number where the tilt switch is attached

void setup() {                           
  for (int i=0; i<5; i=i+1){            
    pinMode(pins[i] , OUTPUT);          // LED pins are declared as outputs
  }
  pinMode(tiltPin,INPUT);               // pin of the tilt switch is declared as input
  randomSeed(analogRead(0));            // initialize random number generator based on random state of pin zero
}

void loop() {
  if (digitalRead(tiltPin)==HIGH){      // when the tilt switch is open
   for (int i=0; i<5; i=i+1){           // all the LEDs
     digitalWrite(pins[i],LOW);         // get switched off
   }                                    // 
   int myRandom=random(0,5);            // a random LED 
   digitalWrite(pins[myRandom],HIGH);   // is switched on
   delay(20);                           // the program is waiting to breath
  }
}
