/*
  Fading
  dimmt eine LED langsam an und aus
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int ledPin = 9;                                                   // ledPin wird deklariert

void setup(){ 
  // die analoge Ausgabe muss nicht im Setup initialisiert werden
} 

void loop(){ 
  for(int leuchtwert = 0; leuchtwert <= 255; leuchtwert++) {      // fadeValue wird in Fünferschritten hochgezählt 
    analogWrite(ledPin, leuchtwert);                              // und als analoger Wert an die LED übertragen 
    delay(2);                                                     // kurze Wartezeit                          
  } 
  for(int leuchtwert = 255; leuchtwert >= 0; leuchtwert--) {      // fadeValue wird in Fünferschritten runtergezählt 
    analogWrite(ledPin, leuchtwert);                              // und als analoger Wert an die LED übertragen 
    delay(2);                                                     // kurze Wartezeit                     
  } 
}


