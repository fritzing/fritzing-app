/*
  Fading
  fading two LEDs slowly on and off
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int led1Pin = 9;                                                  // led1Pin is declared
int led2Pin = 10;                                                 // led2Pin is declared

void setup(){ 
  // no need to declare the analog output in the setup 
} 

void loop(){ 
  for(int fadeValue = 0; fadeValue <= 255; fadeValue += 5) {      // fadeValue is counted up in steps of five 
    analogWrite(led1Pin, fadeValue);                              // and sent to the led as an analog value 
    analogWrite(led2Pin, 255-fadeValue);                          // 255-fadeValue produces the opposite fading behavior     
    delay(2);                                                     // short stop                          
  } 
  for(int fadeValue = 255; fadeValue >= 0; fadeValue -= 5) {      // fadeValue is counted down in steps of five 
    analogWrite(led1Pin, fadeValue);                              // and sent to the led as an analog value 
    analogWrite(led2Pin, 255-fadeValue);                          // 255-fadeValue produces the opposite fading behavior    
    delay(2);                                                     // short stop                     
  } 
}


