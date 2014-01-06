/*
  Matrix
  LED-Matrix displaying image
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

// pin order for Arduino UNO
int rowPins[8] = {9,3,2,12,15,11,7,6};      // matrix rows connected to the Arduino
int colPins[8] = {13,8,17,10,5,16,4,14};   // matrix columns connected to the Arduino


// pin order for Arduino MEGA
// int rowPins[8] = {9,3,2,12,55,11,7,6};       // matrix rows connected to the Arduino
// int colPins[8] = {13,8,57,10,5,56,4,54};    // matrix columns connected to the Arduino

int image[8][8]={                               // the image displayed on the matrix : 1 = LED on, 0 = LED off
  
{0,1,0,0,0,0,1,0},  
{1,1,1,0,0,1,1,1},
{0,1,0,0,0,0,1,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{1,0,0,0,0,0,0,1},
{0,1,1,1,1,1,1,0},
{0,0,1,1,1,1,0,0}

};                                              

void setup(){
  for (int i=0; i<8; i++){                       // all pins are declared as outputs 
    pinMode(rowPins[i],OUTPUT);
    pinMode(colPins[i],OUTPUT);
  }
}

void loop(){
  for (int y=0; y<8; y++){                       // rowwise 
    for (int x=0; x<8; x++){                     // check all entries of the array from left to right 
      if (image[x][y]==1){                       // is the entry 1
        digitalWrite(colPins[x],HIGH);           // switch on column pin
      } else {                                   // else
        digitalWrite(colPins[x],LOW);            // switch it off
      }
    }
    digitalWrite(rowPins[y],LOW);                // switch the row pin to LOW (because it is the cathode of the LED; LOW means ON)
    delayMicroseconds(100);                      // stop the program for 100 milliseconds
    digitalWrite(rowPins[y],HIGH);               // switch the row pin to HIGH (this means OFF)
  }
}
