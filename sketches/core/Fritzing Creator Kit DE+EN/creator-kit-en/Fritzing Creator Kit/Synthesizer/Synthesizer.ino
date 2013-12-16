/*
  Synthesizer
  creating sound from electronics
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int buzzerPin = 8;                             // piezo pin declaration
int potPin1 = 0;                               // potentiometer 1 pin declaration
int potPin2 = 1;                               // potentiometer 2 pin declaration

int toneHeight;                                // store the tone height
int lfo;                                       // store the frequency speed

/* lfo is a "low frequency oscillator" - so a frequency change depending on time */

void setup() {
  pinMode(buzzerPin, OUTPUT);                  // buzzerPin as output
}

void play(int myToneHeight) {                  // play method
    digitalWrite(buzzerPin, HIGH);             // piezo is switched on
    delayMicroseconds(myToneHeight);           // stopping time influenced by toneHeight 
    digitalWrite(buzzerPin, LOW);              // piezo is switched off
    delayMicroseconds(myToneHeight);           // stopping time influenced by toneHeight 
}

void loop() {
  toneHeight=analogRead(potPin1);              // toneheight is value of potentiometer 1
  lfo=analogRead(potPin2);                     // lfo is value of potentiometer 2
  
  for (int i = (lfo/10); i > 0; i--) {         // toneheight rising depending on lfo
    play(toneHeight);                          // execute play method
  }
  delayMicroseconds(lfo);                      // break
  for (int i = 0; i < lfo/10; i++) {           // toneheight sinking depending on lfo
    play(toneHeight);                          // execute play method
  }
}
