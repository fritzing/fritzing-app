/*
  Wahrsager
  schaltet zwischen fünf LEDs um die Zukunft vorher zu sagen
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int pins[] = {8,9,10,11,12};            // Pins der einzelnen LEDs werden in einem Array gespeichert
int tiltPin = 3;                        // Pin-Nummer des Neigungsschalters wird deklariert

void setup() {                           
  for (int i=0; i<5; i=i+1){            
    pinMode(pins[i] , OUTPUT);          // LED Pins werden als OUTPUTS deklariert
  }
  pinMode(tiltPin,INPUT);  // Pin des Neigungsschalters wird als INPUT deklariert
  randomSeed(analogRead(0));
}

void loop() {
  if (digitalRead(tiltPin)==HIGH){      // wenn der Tilt-Switch geöffnet ist
   for (int i=0; i<5; i=i+1){           // werden alle LEDs
     digitalWrite(pins[i],LOW);         // abgeschalten
   }                                    // sonst
   int myRandom=random(0,5);            // eine zufällige LED wird ausgewählt
   digitalWrite(pins[myRandom],HIGH);   // und angeschalten
   delay(20);                           // Programm macht eine kurze Pause
  }
}
