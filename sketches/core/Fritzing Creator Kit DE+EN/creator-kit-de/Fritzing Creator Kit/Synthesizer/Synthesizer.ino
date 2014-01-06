/*
  Synthesizer
  erzeugt eine Frequenzschema aus den Werten zweier Potientiometer
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int buzzerPin = 8;                             // deklariert den Pin des Piezos
int potPin1 = 0;                               // Potentiometer 0 Pin
int potPin2 = 1;                               // Potentiometer 1 Pin

int toneHeight;                                // speichert die Tonhöhe zwischen
int lfo;                                       // speichert die Frequensgeschwindigkeit

/* lfo bezeichnet einen "niederfrequenten Oszillator" - also eine zeitliche Frequenzänderung */

void setup() {
  pinMode(buzzerPin, OUTPUT);                  // buzzerPin wird als OUTPUT deklariert
}

void play(int myToneHeight) {                  // play-Methode
    digitalWrite(buzzerPin, HIGH);             // Piezo wird angeschalten
    delayMicroseconds(myToneHeight);           // Wartezeit wird durch toneHeight beeinflusst
    digitalWrite(buzzerPin, LOW);              // Piezo wird abgeschalten
    delayMicroseconds(myToneHeight);           // Wartezeit wird durch toneHeight beeinflusst
}

void loop() {
  toneHeight=analogRead(potPin1);              // Tonhöhe ist Wert vom Potentiometer 1
  lfo=analogRead(potPin2);                     // lfo ist Wert vom Potentiometer 1
  
  for (int i = (lfo/10); i > 0; i--) {         // Tonhöhe steigend in Abhängigkeit des lfo Wertes
    play(toneHeight);
  }
  delayMicroseconds(lfo);                      // Pause
  for (int i = 0; i < lfo/10; i++) {           // Tonhöhe sinkend in Abhängigkeit des lfo Wertes
    play(toneHeight);
  }
}
