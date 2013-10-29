/*
  Übung 1_2 Blink SOS
  Blinkt ein SOS Signal:
  * kurz-kurz-kurz = S
  * lang-lang-lang = O
  * kurz-kurz-kurz = S
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int led = 13;                   // ganzzahlige Variable led wird deklariert

void setup() {                  // die setup()-Methode wird einmal ausgeführt
  pinMode(led, OUTPUT);     
}

void loop() {                   // die loop()-Methode wird immer wiederholt
  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(200);                   // hält das Programm für 200 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(200);                   // hält das Programm für 200 Millisekunden an
  
  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(200);                   // hält das Programm für 200 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(200);                   // hält das Programm für 200 Millisekunden an

  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(200);                   // hält das Programm für 200 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(200);                   // hält das Programm für 200 Millisekunden an

  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(500);                   // hält das Programm für 500 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(500);                   // hält das Programm für 500 Millisekunden an

  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(500);                   // hält das Programm für 500 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(500);                   // hält das Programm für 500 Millisekunden an 
  
  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(500);                   // hält das Programm für 500 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(500);                   // hält das Programm für 500 Millisekunden an   
  
  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(200);                   // hält das Programm für 200 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(200);                   // hält das Programm für 200 Millisekunden an
  
  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(200);                   // hält das Programm für 200 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(200);                   // hält das Programm für 200 Millisekunden an

  digitalWrite(led, HIGH);      // schaltet die LED ein 
  delay(200);                   // hält das Programm für 200 Millisekunden an
  digitalWrite(led, LOW);       // schaltet die LED ab
  delay(2000);                  // hält das Programm für 2000 Millisekunden an  
}
