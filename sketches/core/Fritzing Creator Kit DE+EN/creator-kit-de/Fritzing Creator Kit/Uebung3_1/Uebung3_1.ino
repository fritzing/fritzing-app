/*
  Matrix Übung
  LED-Matrix zeigt ein anderes Bild
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

// Pinbelegung für Arduino UNO
int reihenPins[8] = {9,3,2,12,15,11,7,6};      // Anschlüsse der Reihenpins am Arduino
int spaltenPins[8] = {13,8,17,10,5,16,4,14};   // Anschlüsse der Spaltenpins am Arduino


// Pinbelegung für Arduino MEGA
// int reihenPins[8] = {9,3,2,12,55,11,7,6};      // Anschlüsse der Reihenpins am Arduino
// int spaltenPins[8] = {13,8,57,10,5,56,4,54};   // Anschlüsse der Spaltenpins am Arduino

int image[8][8]={                              // Bild, das auf der Matrix gezeigt wird. 1 = LED an, 0 = LED aus
  
{0,0,1,1,0,1,1,0},  
{0,1,0,0,1,0,0,1},
{0,1,0,0,0,0,0,1},
{0,0,1,0,0,0,1,0},
{0,0,0,1,0,1,0,0},
{0,0,0,0,1,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}

};                                              

void setup(){
  for (int i=0; i<8; i++){                       // Alle Pins werden als OUTPUT declariert 
    pinMode(reihenPins[i],OUTPUT);
    pinMode(spaltenPins[i],OUTPUT);
  }
}

void loop(){
  for (int y=0; y<8; y++){                       // zeilenweise werden 
    for (int x=0; x<8; x++){                     // von links nach rechts alle Einträge des Arrays geprüft
      if (image[x][y]==1){                       // ist der Eintrag = 1
        digitalWrite(spaltenPins[x],HIGH);       // wird der Spaltenpin eingeschalten
      } else {                                   // sonst
        digitalWrite(spaltenPins[x],LOW);        // abgeschalten
      }
    }
    digitalWrite(reihenPins[y],LOW);             // nun wird der Reihenpin auf LOW geschalten (da es die Kathode der LED betrifft bedeutet LOW eingeschalten: LOW = GND)
    delayMicroseconds(100);                      // das Programm hällt 100 Mikrosekunden an
    digitalWrite(reihenPins[y],HIGH);            // danach wird der Reihenpin wieder auf HIGH (in diesem Fall also abge-) schalten
  }
}
