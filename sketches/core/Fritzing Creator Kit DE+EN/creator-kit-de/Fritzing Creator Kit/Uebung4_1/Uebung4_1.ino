/*
  Flip Flop Übung
  schaltet mittels eines Tasters zwischen zwei LEDs hin und her
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int tasterPin=2;                       // Pin an dem der Taster angeschlossen ist
int grueneLED=9;                       // Pin an dem die grüne LED angeschlossen ist
int roteLED=10;                        // Pin an dem die rote LED angeschlossen ist 

int zustand=1;                         // speichert den Zustand des Programmes : 1 = gruen, -1 = rot
boolean tasterAusgeloest=false;        // speichert, ob der Taster gerade gedrückt wurde

void setup(){
  pinMode(tasterPin, INPUT);           // Taster Pin wird als INPUT initialisiert
  pinMode(grueneLED, OUTPUT);          // LED Pin wird als OUTPUT initialisiert
  pinMode(roteLED, OUTPUT);            // LED Pin wird als OUTPUT initialisiert 
}

void loop(){
  if ((digitalRead(tasterPin)==LOW)&&(tasterAusgeloest==false)){  // wenn der Taster gedrückt wird und tasterAusgeloest falsch ist
    tasterAusgeloest=true;             // diese Variable und die Abfrage in der letzten Zeile verhindern ein dauerndes Umschalten, probiere aus, was passiert, wenn Du diese Zeile weglässt
    zustand=zustand*-1;                // der Zustand wird umgekehrt
  }
  
  if (digitalRead(tasterPin)==HIGH){   // wenn der Taster nicht gedrückt wird
    tasterAusgeloest=false;            // wird tasterAusgelöst auf falsch gesetzt
  }

  if (zustand==-1){                    // wenn der Zustand -1 ist 
    digitalWrite(grueneLED, HIGH);     // grüne LED einschalten
    digitalWrite(roteLED, LOW);        // rote LED abschalten
  } else {                             // sonst
    digitalWrite(grueneLED, LOW);      // grüne LED abschalten
    digitalWrite(roteLED, HIGH);       // rote LED einschalten
  }
}
