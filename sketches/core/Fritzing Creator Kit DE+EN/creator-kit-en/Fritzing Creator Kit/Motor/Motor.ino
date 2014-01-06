/*
  Motor
  turning a motor clock then counter clock wise
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creatorkit.
*/

int motor_A=6;                    // motor A Pin
int motor_B=5;                    // motor B Pin
int motor_Speed=3;                // speed Pin

void setup(){
  pinMode(motor_A,OUTPUT);        // pin A declared as OUTPUT
  pinMode(motor_B,OUTPUT);        // pin B declared as OUTPUT
}

void loop(){
  digitalWrite(motor_A,HIGH);     // motor A pin switched to HIGH (+5V)
  digitalWrite(motor_B,LOW);      // motor B pin switched to LOW  (GND)
  for (int i=0; i<256; i+=5){     // count up to 255 in steps of five 
    analogWrite(motor_Speed,i);   // and write it as speed to the speed pin
    delay(20);                    // wait 20 ms
  }
  for (int i=255; i>0; i-=5){    // count down from 255 to 0 by 5
    analogWrite(motor_Speed,i);  // and write it as speed to the speed pin
    delay(20);                   // wait 20 ms
  }

  digitalWrite(motor_A,LOW);     // motor A pin switched to HIGH (GND) 
  digitalWrite(motor_B,HIGH);    // motor B pin switched to LOW  (+5V)
  for (int i=0; i<256; i+=5){    // count up to 255 in steps of five 
    analogWrite(motor_Speed,i);  // and write it as speed to the speed pin
    delay(20);                   // wait 20 ms
  }
  for (int i=255; i>0; i-=5){    // count down from 255 to 0 by 5
    analogWrite(motor_Speed,i);  // and write it as speed to the speed pin
    delay(20);                   // wait 20 ms
  }
}


