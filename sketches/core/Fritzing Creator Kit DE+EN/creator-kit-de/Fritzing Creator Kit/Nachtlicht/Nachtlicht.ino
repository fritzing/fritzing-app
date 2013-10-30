/*
  Nachtlicht
  blendet Farben durch
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int LEDGreen=9;                                      // LEDGreen Pins wird deklariert
int LEDBlue=10;                                      // LEDBlue Pins wird deklariert
int LEDRed=11;                                       // LEDRed Pins wird deklariert 

void setup(){
  pinMode(LEDRed,OUTPUT);                            // Pin wird als OUTPUT initialisiert
  pinMode(LEDGreen,OUTPUT);                          // Pin wird als OUTPUT initialisiert
  pinMode(LEDBlue,OUTPUT);                           // Pin wird als OUTPUT initialisiert
}

void loop(){
 for (int frame=0; frame<900; frame++){              // frame wird von 0 bis 900 hochgezählt
  if (frame<150) {                                   // wenn frame < 150  > rot
    analogWrite(LEDRed,255);                         // LED wird eingeschaltet
    analogWrite(LEDBlue,0);                          // LED wird abgeschaltet
    analogWrite(LEDGreen,0);                         // LED wird abgeschaltet
  } else if (frame<300) {                            // wenn frame < 300
    analogWrite(LEDRed,map(frame,150,300,255,0));    // LED wird ausgedimmt
    analogWrite(LEDBlue,map(frame,150,300,0,255));   // LED wird angedimmt 
    analogWrite(LEDGreen,0);                         // LED wird abgeschaltet
  } else if (frame<450) {                            // wenn frame < 450  > blau
    analogWrite(LEDRed,0);                           // LED wird abgeschaltet
    analogWrite(LEDBlue,255);                        // LED wird eingeschaltet
    analogWrite(LEDGreen,0);                         // LED wird abgeschaltet  
  } else if (frame<600) {                            // wenn frame < 600
    analogWrite(LEDRed,0);                           // LED wird abgeschaltet
    analogWrite(LEDBlue,map(frame,450,600,255,0));   // LED wird ausgedimmt 
    analogWrite(LEDGreen,map(frame,450,600,0,255));  // LED wird angedimmt
  } else if (frame<750) {                            // wenn frame < 750  > grün
    analogWrite(LEDRed,0);                           // LED wird abgeschaltet
    analogWrite(LEDBlue,0);                          // LED wird abgeschaltet 
    analogWrite(LEDGreen,255);                       // LED wird abgeschaltet
  } else if (frame<900) {                            // wenn frame < 900
    analogWrite(LEDRed,map(frame,750,900,0,255));    // LED wird angedimmt
    analogWrite(LEDBlue,0);                          // LED wird abgeschaltet 
    analogWrite(LEDGreen,map(frame,750,900,255,0));  // LED wird ausgedimmt
  }
  delay(10);                                         // kurze Pause
 }
}


