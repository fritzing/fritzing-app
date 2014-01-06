/*
  Lichtautomatik
  schaltet die Lichter eines Autos ja nach Helligkeit an und ab
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int lichtsensorPin = 0;                            // lichtsensorPin wird deklariert
int scheinwerferPin = 2;                           // scheinwerferPin wird deklariert
int schaltgrenze = 300;                            // ab diesem Wert wird »Dunkelheit« definiert
int wartezeit = 1000;                              // Timer-Timeout sind 1000 Millisekunden
long timer = 0;                                    // Timervariabel wird deklariert
int lichtwert;                                     // speichert die Lichtsittuation

void setup(){
  Serial.begin(9600);                              // startet die serielle Kommunikation
  pinMode(scheinwerferPin,OUTPUT);                 // scheinwerferPin wird als OUTPUT initialisiert
}

void loop(){
  lichtwert = analogRead(lichtsensorPin);          // lichtwert wird eingelesen
  Serial.println(lichtwert);                       // lichtwert wird an die serielle Schnittstelle gesendet
  if (lichtwert>schaltgrenze){                     // wenn die Schaltgrenze unterschritten wird
    timer=millis();                                // timer wird auf aktuelle Zeit gesetzt
    digitalWrite(scheinwerferPin, HIGH);           // Scheinwerfer werden eingeschalten
  } else if (millis()>wartezeit+timer){            // sonst wenn er Timer abgelaufen ist
    digitalWrite(scheinwerferPin, LOW);            // werden die Scheinwerfer abgeschalten
  }
  delay(10);                                       // Pause
}
