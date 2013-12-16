/*
  Lauflicht von beiden Seiten
  
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
  for (int i=0; i<4; i=i+1){             // Lauflicht hin
    digitalWrite(pins[i], HIGH);         // LED an der Stelle i einschalten
    digitalWrite(pins[7-i], HIGH);       // LED an der Stelle 7-i einschalten    
    delay(100);                          // hält das Programm für 100 Millisekunden an
    digitalWrite(pins[i], LOW);          // LED an der Stelle i ausschalten
    digitalWrite(pins[7-i], LOW);        // LED an der Stelle 7-i ausschalten    
  }
  for (int i=2; i>=1; i=i-1){            // Lauflicht her
    digitalWrite(pins[i], HIGH);         // LED an der Stelle i einschalten
    digitalWrite(pins[7-i], HIGH);       // LED an der Stelle 7-i einschalten    
    delay(100);                          // hält das Programm für 100 Millisekunden an
    digitalWrite(pins[i], LOW);          // LED an der Stelle i ausschalten
    digitalWrite(pins[7-i], LOW);        // LED an der Stelle 7-i ausschalten    
  }
}

