/*
  night light
  fading through the colors of a rainbow
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creatorkit.
*/

int LEDGreen=9;                                      // LEDGreen pin declared
int LEDBlue=10;                                      // LEDBlue pin declared
int LEDRed=11;                                       // LEDRed pin declared

void setup(){
  pinMode(LEDRed,OUTPUT);                            // pin is output
  pinMode(LEDGreen,OUTPUT);                          // pin is output
  pinMode(LEDBlue,OUTPUT);                           // pin is output
}

void loop(){
 for (int frame=0; frame<900; frame++){              // frame is counted up from 0 to 900 
  if (frame<150) {                                   // if frame < 150  => red
    analogWrite(LEDRed,255);                         // LED switched on
    analogWrite(LEDBlue,0);                          // switch off LED
    analogWrite(LEDGreen,0);                         // switch off LED
  } else if (frame<300) {                            // if frame < 300
    analogWrite(LEDRed,map(frame,150,300,255,0));    // fade off LED
    analogWrite(LEDBlue,map(frame,150,300,0,255));   // fade on LED 
    analogWrite(LEDGreen,0);                         // switch off LED
  } else if (frame<450) {                            // if frame < 450  => blue
    analogWrite(LEDRed,0);                           // switch off LED
    analogWrite(LEDBlue,255);                        // switch on LED
    analogWrite(LEDGreen,0);                         // switch off LED  
  } else if (frame<600) {                            // if frame < 600
    analogWrite(LEDRed,0);                           // switch off LED
    analogWrite(LEDBlue,map(frame,450,600,255,0));   // fade off LED 
    analogWrite(LEDGreen,map(frame,450,600,0,255));  // fade on LED
  } else if (frame<750) {                            // if frame < 750  => green
    analogWrite(LEDRed,0);                           // switch off LED
    analogWrite(LEDBlue,0);                          // switch off LED 
    analogWrite(LEDGreen,255);                       // switch off LED
  } else if (frame<900) {                            // if frame < 900
    analogWrite(LEDRed,map(frame,750,900,0,255));    // fade on LED
    analogWrite(LEDBlue,0);                          // switch off LED 
    analogWrite(LEDGreen,map(frame,750,900,255,0));  // fade off LED
  }
  delay(10);                                         // short break
 }
}


