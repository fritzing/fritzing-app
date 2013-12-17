/*
  Ping Pong Spiel
  LED-Matrix und zwei Potis für ein Pingpong Spiel
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

// Pinbelegung für Arduino UNO
int reihenPins[8] = {9,3,2,12,15,11,7,6};      // Anschlüsse der Reihenpins am Arduino
int spaltenPins[8] = {13,8,17,10,5,16,4,14};   // Anschlüsse der Spaltenpins am Arduino


// Pinbelegung für Arduino MEGA
// int reihenPins[8] = {9,3,2,12,55,11,7,6};      // Anschlüsse der Reihenpins am Arduino
// int spaltenPins[8] = {13,8,57,10,5,56,4,54};   // Anschlüsse der Spaltenpins am Arduino

int pot1Pin=18;                                        // Pin an dem das Potentiometer von Spieler 1 angeschlossen ist
int pot2Pin=19;                                        // Pin an dem das Potentiometer von Spieler 2 angeschlossen ist

int image[8][8]={                                      // Bild, das auf der Matrix gezeigt wird. 1 = LED an, 0 = LED aus
{0,0,0,0,0,0,0,0},  
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};                                              

int death[8][8]={                                      // Alles an
{1,1,1,1,1,1,1,1},  
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1},  
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}
};   

int death2[8][8]={                                      // Totenkopf
{0,1,1,1,1,1,0,0},  
{1,1,1,1,1,1,1,0},
{1,0,0,1,0,0,1,0},
{1,1,1,1,1,1,1,0},
{0,1,1,1,1,1,0,0},
{0,1,0,1,0,1,0,0},
{0,1,0,1,0,1,0,0},
{0,0,0,0,0,0,0,0}
};   

int blank[8][8]={                                      // alles aus
{0,0,0,0,0,0,0,0},  
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};    

long theTimer;                                           // Timervariable

int gameState=0;                                         // speichert den Spielmodus
int animations=300;                                      // speichert die Dauer der Zwischenbilder

float ballXSpeed = 1;                                    // speichert die Beschleunigung in X-Richtung   
float ballXPosition=4;                                   // speichert die Ballposition als reale Zahl

float ballYSpeed = 0;                                    // speichert die Beschleunigung in Y-Richtung
float ballYPosition=4;                                   // speichert die Ballposition als reale Zahl

int imageYPosition;                                      // speichert die Ballposition als ganze Zahl
int imageXPosition;                                      // speichert die Ballposition als ganze Zahl

int player1Position=3;                                   // speichert die Paddel Position von Player1
int player2Position=3;                                   // speichert die Paddel Position von Player2

int gameSpeed;                                           // speichert die aktuelle Spielgeschwindigkeit

void setup(){
  for (int i=0; i<8; i++){                               // Alle Pins werden als OUTPUT deklariert 
    pinMode(reihenPins[i],OUTPUT);
    pinMode(spaltenPins[i],OUTPUT);
  }
}

void draw(){
  for (int y=0; y<8; y++){                               // zeilenweise werden 
    for (int x=0; x<8; x++){                             // von links nach rechts alle Einträge des Arrays geprüft
      if (image[x][y]==1){                               // ist der Eintrag = 1
        digitalWrite(spaltenPins[x],HIGH);               // wird der Spaltenpin eingeschalten
      } else {                                           // sonst
        digitalWrite(spaltenPins[x],LOW);                // abgeschalten
      }
    }
    digitalWrite(reihenPins[y],LOW);                     // nun wird der Reihenpin auf LOW geschalten (da es die Kathode der LED betrifft bedeutet LOW eingeschalten: LOW = GND)
    delayMicroseconds(1000);                             // das Programm hällt 1000 Mikrosekunden an
    digitalWrite(reihenPins[y],HIGH);                    // danach wird der Reihenpin wieder auf HIGH (in diesem Fall also abge-) schalten
  }
}

void update(){
  switch (gameState) {                                   // in welchem Modus befindet sich das Programm
  case 0:                                                // neues Spiel
    memcpy(image,blank,sizeof(blank));                   // Bildschirm wird gelöscht
    gameSpeed=300;                                       // Spieltempo wird festgelegt
    ballXPosition=3;                                     // festlegen der Ballposition
    ballYPosition=3;                                     // festlegen der Ballposition
    ballYSpeed=0;                                        // Ball fliegt am Anfang gerade
    if (random(0,2)>0){                                  // per Zufall nach Links oder Rechts
      ballXSpeed=1;
    } else {
      ballXSpeed=-1;
    }     
    theTimer=millis(); 
    gameState=1;
    break;
  case 1:                                                // Spiel aktiv
    image[player1Position][0]=0;                         // Paddel Player 1 alte Position löschen
    image[player1Position+1][0]=0;                       // Paddel Player 1 alte Position löschen
    image[player2Position][7]=0;                         // Paddel Player 2 alte Position löschen
    image[player2Position+1][7]=0;                       // Paddel Player 2 alte Position löschen

    player1Position=map(analogRead(pot1Pin),0,1023,0,6); // liest die Position von Spieler 1 aus
    player2Position=map(analogRead(pot2Pin),0,1023,0,6); // liest die Position von Spieler 2 aus
    
    image[player1Position][0]=1;                         // Paddel Player 1 darstellen              
    image[player1Position+1][0]=1;                       // Paddel Player 1 darstellen    
    image[player2Position][7]=1;                         // Paddel Player 2 darstellen              
    image[player2Position+1][7]=1;                       // Paddel Player 2 darstellen        
  
    if (millis()>theTimer+gameSpeed){                    // timer für Spielgeschwindigkeit
      if (gameSpeed>50) gameSpeed-=3;                    // spiel wird beschleunigt    
      theTimer=millis();                                 // neuer Timer wird gesetzt
      image[imageYPosition][imageXPosition]=0;           // alte Position wird überschrieben    
      ballXPosition+=ballXSpeed;                         // Update der Position
      ballYPosition+=ballYSpeed;                         // Update der Position      
      
      if (ballYPosition>=7) ballYSpeed*=-1;              // Koolision unterer Rand
      if (ballYPosition<=0) ballYSpeed*=-1;              // Koolision oberer Rand

      ballYPosition=constrain(ballYPosition,0,7);        // verhindert, dass Werte kleiner 0 oder größer 7 erreicht werden
      ballXPosition=constrain(ballXPosition,0,7);        // verhindert, dass Werte kleiner 0 oder größer 7 erreicht werden      
      imageYPosition=round(ballYPosition);
      imageXPosition=round(ballXPosition);      
      
      if ((ballXPosition>=6)&&(image[imageYPosition][7]==1)) {  // wenn der Ball den Rand erreicht und das Paddel trifft
        ballXSpeed*=-1;                                    // Ball wird reflektiert
        ballYSpeed=random(-2,3);                           // Abprallwinkel bekommt Zufallskomponente
      }
      if ((ballXPosition<=1)&&(image[imageYPosition][0]==1)) {  // wenn der Ball den Rand erreicht und das Paddel trifft
        ballXSpeed*=-1;                                    // Ball wird reflektiert
        ballYSpeed=random(-2,3);                           // Abprallwinkel bekommt Zufallskomponente
      }
      if (ballXPosition>=7){                               // Ball im Aus
        gameState=2;                                       // Spiel verloren Modus wird aktiviert
        theTimer=millis();                                 // neuer Timer wird gesetzt
      }

      if (ballXPosition<=0){                               // Ball im Aus
        gameState=2;                                       // Spiel verloren Modus wird aktiviert      
        theTimer=millis();                                 // neuer Timer wird gesetzt
      }

      image[imageYPosition][imageXPosition]=1;             // neue Position wird eingeschalten
      
    }
    break;
  case 2:                                                  // Spiel verloren, kurze Nachleuchtzeit
    if (millis()>theTimer+gameSpeed){
      theTimer=millis();
      gameState=3;                                         // Spiel verloren Anzeigemodus aufrufen
    }
    break;
  case 3:                                                  // Spiel verloren Anzeigemodus
    memcpy(image,death,sizeof(death));                     // Spiel verloren Bild wird aufgerufen, memcpy ist eine Funktion, die ein Array einem anderen Array gleich setzt
    if (millis()>theTimer+animations){                     // Wartzeit
      gameState=0;                                         // Spiel starten von vorn
    }
    break;
  }
}

void loop(){
  update();                                                // alle Berechnungen sind in der update-Methode
  draw();                                                  // Darstellung ist in der draw-Methode
}
