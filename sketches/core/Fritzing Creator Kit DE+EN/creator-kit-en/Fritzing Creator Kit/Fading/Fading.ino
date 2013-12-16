/*
  Fading
  fading an LED slowly on and off again
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int ledPin = 9;                                                   // ledPin is declared

void setup(){ 
  // no need to declare the analog output in the setup 
} 

void loop(){ 
  for(int fadeValue = 0; fadeValue <= 255; fadeValue += 5) {      // fadeValue is counted up in steps of five
    analogWrite(ledPin, fadeValue);                               // and sent to the led as an analog value
    delay(2);                                                     // short stop                        
  } 
  for(int fadeValue = 255; fadeValue >= 0; fadeValue -= 5) {      // fadeValue is counted down in steps of five
    analogWrite(ledPin, fadeValue);                               // and sent to the led as an analog value 
    delay(2);                                                     // short stop                         
  } 
}


