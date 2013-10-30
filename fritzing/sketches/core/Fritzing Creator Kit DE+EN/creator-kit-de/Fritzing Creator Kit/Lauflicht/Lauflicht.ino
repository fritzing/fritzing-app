/*
  Lauflicht
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int pinsGesamt=8;                        // deklariert die ganzzahlige Variable pinsGesamt
int pins[] = {2,3,4,5,6,7,8,9};          // deklariert das Array pins[]

void setup() {                
  for (int i=0; i<pinsGesamt; i=i+1){    // zählt die Variable i von 0 bis 7
    pinMode(pins[i], OUTPUT);            // initialisiert den Pin an der pins-Array-Stelle i als OUTPUT
  }
}

void loop() {
  for (int i=0; i<pinsGesamt; i=i+1){    // Lauflicht Hin
    digitalWrite(pins[i], HIGH);         // LED an der Stelle i einschalten
    delay(100);                          // hält das Programm für 100 Millisekunden an
    digitalWrite(pins[i], LOW);          // LED an der Stelle i ausschalten
  }
  for (int i=6; i>0; i=i-1){             // Lauflicht Zurück (die äußeren LEDs werden nicht angesteuert)
    digitalWrite(pins[i], HIGH);         // LED an der Stelle i einschalten
    delay(100);                          // hält das Programm für 100 Millisekunden an
    digitalWrite(pins[i], LOW);          // LED an der Stelle i ausschalten
  }
}

