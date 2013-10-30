/*
  Potentiometer
  zwei LEDs werden mit einem Potentiometer gesteuert
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int ledGreen = 6;                            // LED Pin wird deklariert
int ledRed = 5;                              // LED Pin wird deklariert
int potPin = 0;                              // Potentiometer wird deklariert

void setup(){
                                             // werder analoge Ein- noch Ausgaben müssen im Setup initialisiert werden
}

void loop(){
  int value = analogRead(potPin);            // die Variable value speichert den Wert des Potentiometers
  int redValue = map(value,0,1023,0,255);    // diser wird mit der Map-Funktion umgerechnet - hier von 0 bis 255,
  int greenValue = map(value,0,1023,255,0);  // hier von 255 - 0
  
  analogWrite(ledRed,redValue);              // die umgerechneten Werte
  analogWrite(ledGreen,greenValue);          // werden an beide LEDs gesendet
}


