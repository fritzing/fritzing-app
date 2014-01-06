import processing.serial.*; 
import cc.arduino.*; 

Arduino meinArduino; 

int potPin=0;
int value;

int redX = 20;
int greenX = 20;
int blueX = 20;

int redPin = 9;
int greenPin = 10;
int bluePin = 11;

int oldGreen;
int oldRed;
int oldBlue;

void setup() {
  size(255,300,P3D);
  println(Serial.list()); // List all the available serial ports:
  meinArduino = new Arduino(this, Arduino.list()[4], 57600); 
  noStroke();
}

void graphics(int theValue){
  colorMode(RGB,255);
  background(0);
  fill(100);
  rect(0,20,width,20);
  rect(0,60,width,20);
  rect(0,100,width,20);  
  fill(255,0,0); rect(0,20,redX,20);
  fill(0,255,0); rect(0,60,greenX,20);
  fill(0,0,255); rect(0,100,blueX,20);
  colorMode(HSB, 100);
  fill(value,100,100);  
  rect(0,160,width,height);
}

void draw(){
  value=meinArduino.analogRead(potPin);
  value=(int)map(value,0,1023,0,100);
  if (mousePressed==true){
    if ((mouseX>0)&&(mouseX<=width)){
      if ((mouseY>20)&&(mouseY<40)) {
        redX = mouseX;
      }
      if ((mouseY>60)&&(mouseY<80)) {
        greenX = mouseX;
      }
      if ((mouseY>100)&&(mouseY<120)) {
        blueX = mouseX;
      }
    }
  }

  graphics(value);
  
  if (redX!=oldRed){
    meinArduino.analogWrite(redPin,redX);
    oldRed=redX;
  }
  if (greenX!=oldGreen){
    meinArduino.analogWrite(greenPin,greenX);
    oldGreen=greenX;
  }
  if (blueX!=oldBlue){
    meinArduino.analogWrite(bluePin,blueX);
    oldBlue=blueX;
  }
}
