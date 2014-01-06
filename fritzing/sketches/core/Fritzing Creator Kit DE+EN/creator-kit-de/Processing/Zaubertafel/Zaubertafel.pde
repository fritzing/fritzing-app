import processing.serial.*;         
import cc.arduino.*;                

Arduino meinArduino;                

float theta;
float theX, theY, oldX, oldY;

boolean firstRun=true;

void setup() {
  size(600, 400);
  println(Serial.list());
  meinArduino = new Arduino(this, Arduino.list()[5], 57600);
  meinArduino.pinMode(2, Arduino.INPUT);

  background(224, 169, 75);
  strokeWeight(3);
  stroke(118, 132, 64);
}

void draw() {
  theX = meinArduino.analogRead(0);
  theY = meinArduino.analogRead(1);

  theX = map(theX, 0, 1023, 0, width);
  theY = map(theY, 0, 1023, 0, height);

  if (firstRun==true) {
    oldX=theX;
    oldY=theY;
    firstRun=false;
  }

  line(theX, theY, oldX, oldY);  

  oldX=theX;
  oldY=theY;

  if (meinArduino.digitalRead(2)!=0) {
    background(224, 169, 75);
    firstRun=true;
  }
}

