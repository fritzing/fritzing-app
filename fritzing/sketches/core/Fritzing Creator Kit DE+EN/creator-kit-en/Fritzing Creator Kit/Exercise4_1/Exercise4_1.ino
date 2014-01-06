/*
  Flip Flop
  switching between two LEDs using a button
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int buttonPin=2;                       // Pin where the button is attached
int greenLED=9;                        // Pin where the green LED is attached
int redLED=10;                         // Pin where the red LED is attached 

int state=1;                           // store the state of the program : 1 = green, -1 = red
boolean buttonPressed=false;           // stpre if a button was pressed

void setup(){
  pinMode(buttonPin, INPUT);           // button is initialised as INPUT 
  pinMode(greenLED, OUTPUT);           // green LED pin is initialised as OUTPUT 
  pinMode(redLED, OUTPUT);             // red LED pin is initialised as OUTPUT  
}

void loop(){
  if ((digitalRead(buttonPin)==LOW)&&(buttonPressed==false)){  // if the button is pressed and was not pressed before
    buttonPressed=true;                // this line of code stops the state from flipping unnecessarily - see what happens if you comment it out
    state=-state;                      // the state is reversed
  }
  
  if (digitalRead(buttonPin)==HIGH){   // if the button is not pressed anymore
    buttonPressed=false;               // the flag buttonPressed is set to false
  }

  if (state==-1){                      // if the condition equals -1
    digitalWrite(greenLED, HIGH);      // switch on green LED
    digitalWrite(redLED, LOW);         // switch off red LED
  } else {                             // else
    digitalWrite(greenLED, LOW);       // switch off green LED
    digitalWrite(redLED, HIGH);        // switch on red LED
  }
}
