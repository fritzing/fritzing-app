/*
  Flip Flop
  schaltet mittels eines Tasters zwischen zwei LEDs hin und her
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int tasterPin=2;                       // Pin an dem der Taster angeschlossen ist
int grueneLED=9;                       // Pin an dem die grüne LED angeschlossen ist
int roteLED=10;                        // Pin an dem die rote LED angeschlossen ist 

void setup(){
  pinMode(tasterPin, INPUT);           // Taster Pin wird als INPUT initialisiert
  pinMode(grueneLED, OUTPUT);          // LED Pin wird als OUTPUT initialisiert
  pinMode(roteLED, OUTPUT);            // LED Pin wird als OUTPUT initialisiert 
}

void loop(){
  if (digitalRead(tasterPin)==LOW){    // wenn der Taster gedrückt ist (LOW) 
    digitalWrite(grueneLED, HIGH);     // grüne LED einschalten
    digitalWrite(roteLED, LOW);        // rote LED abschalten
  } else {                             // sonst
    digitalWrite(grueneLED, LOW);      // grüne LED abschalten
    digitalWrite(roteLED, HIGH);       // rote LED einschalten
  }
}
