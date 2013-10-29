/*
  Übung 1_2b Blink SOS alternativer Weg
  Blinkt ein SOS Signal:
  * kurz-kurz-kurz = S
  * lang-lang-lang = O
  * kurz-kurz-kurz = S
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int led = 13;                               // ganzzahlige Variable led wird deklariert
char signals[] = {'.','.','.','-','-','-','.','.','.'}; // variable speichert die Signale: . = kurz, - = lang

void setup() {                              // die setup()-Methode wird einmal ausgeführt
  pinMode(led, OUTPUT);     
}

void loop() {                               // die loop()-Methode wird immer wiederholt
  for (int i=0; i<sizeof(signals); i++){    // wiederhole so oft, wie das Array signals Einträge hat (hier also 9 Mal)
    digitalWrite(led,HIGH);                 // schaltet die LED ein
    if (signals[i]=='.') delay(200);        // wenn signals an der Stelle i einen Punkt hat, warte kurz
    if (signals[i]=='-') delay(500);        // wenn signals an der Stelle i einen Strich hat, warte lang
    digitalWrite(led,LOW);                  // schaltet die LED aus
    if (signals[i]=='.') delay(200);        // wenn signals an der Stelle i einen Punkt hat, warte kurz
    if (signals[i]=='-') delay(500);        // wenn signals an der Stelle i einen Strich hat, warte lang
  }
  delay(2000);                              // hält das Programm für 2000 Millisekunden an  
}
