/*
  Fading
  dimmt zwei LEDs langsam an und aus
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int led1Pin = 9;                                                  // led1Pin wird deklariert
int led2Pin = 10;                                                 // led2Pin wird deklariert

void setup(){ 
  // die analoge Ausgabe muss nicht im Setup initialisiert werden
} 

void loop(){ 
  for(int leuchtwert = 0; leuchtwert <= 255; leuchtwert++) {      // fadeValue wird in Fünferschritten hochgezählt 
    analogWrite(led1Pin, leuchtwert);                             // und als analoger Wert an die LED übertragen 
    analogWrite(led2Pin, 255-leuchtwert);                         // 255-leuchtwert erzeugt genau das umgekehrte Verhalten     
    delay(2);                                                     // kurze Wartezeit                          
  } 
  for(int leuchtwert = 255; leuchtwert >= 0; leuchtwert--) {      // fadeValue wird in Fünferschritten runtergezählt 
    analogWrite(led1Pin, leuchtwert);                             // und als analoger Wert an die LED übertragen 
    analogWrite(led2Pin, 255-leuchtwert);                         // 255-leuchtwert erzeugt genau das umgekehrte Verhalten    
    delay(2);                                                     // kurze Wartezeit                     
  } 
}


