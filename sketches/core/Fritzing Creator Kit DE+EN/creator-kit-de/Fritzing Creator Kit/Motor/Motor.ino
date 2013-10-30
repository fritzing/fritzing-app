/*
  Motor
  dreht einen Motor erst rechts, dann links herum
  
  Dieses Beispiel aus dem Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

int motor_A=5;
int motor_B=4;
int motor_Speed=3;

void setup(){
  pinMode(motor_A,OUTPUT);
  pinMode(motor_B,OUTPUT);
}

void loop(){
  digitalWrite(motor_A,HIGH); 
  digitalWrite(motor_B,LOW);
  for (int i=0; i<256; i+=5){
    analogWrite(motor_Speed,i); 
    delay(20);
  }
  for (int i=255; i>0; i-=5){
    analogWrite(motor_Speed,i); 
    delay(20);
  }

  digitalWrite(motor_A,LOW); 
  digitalWrite(motor_B,HIGH);
  for (int i=0; i<256; i+=5){
    analogWrite(motor_Speed,i);
    delay(20);
  }
  for (int i=255; i>0; i-=5){
    analogWrite(motor_Speed,i);
    delay(20);
  }
}


