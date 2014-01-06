/*
  Ping Pong game
  LED-Matrix is used to play Ping Pong
  
  This example is part of the Fritzing Creator Kit: www.fritzing.org/creator-kit.
*/

// pin order for Arduino UNO
int rowPins[8] = {9,3,2,12,15,11,7,6};      // matrix rows connected to the Arduino
int colPins[8] = {13,8,17,10,5,16,4,14};   // matrix columns connected to the Arduino


// pin order for Arduino MEGA
// int rowPins[8] = {9,3,2,12,55,11,7,6};       // matrix rows connected to the Arduino
// int colPins[8] = {13,8,57,10,5,56,4,54};    // matrix columns connected to the Arduino

int pot1Pin=18;                                // declaring the pin for player 1's potentiometer
int pot2Pin=19;                                // declaring the pin for player 2's potentiometer

int image[8][8]={                              // clear 
{0,0,0,0,0,0,0,0},  
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};                                              

int death[8][8]={                              // all on
{1,1,1,1,1,1,1,1},  
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1},  
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}, 
{1,1,1,1,1,1,1,1}
};   

int death2[8][8]={                             // skull
{0,1,1,1,1,1,0,0},  
{1,1,1,1,1,1,1,0},
{1,0,0,1,0,0,1,0},
{1,1,1,1,1,1,1,0},
{0,1,1,1,1,1,0,0},
{0,1,0,1,0,1,0,0},
{0,1,0,1,0,1,0,0},
{0,0,0,0,0,0,0,0}
};   

int blank[8][8]={                              // all off
{0,0,0,0,0,0,0,0},  
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}
};    

long theTimer;                                           // timer variable

int gameState=0;                                         // storing the game status
int animations=300;                                      // storing the duration of the images between the games

float ballXSpeed = 1;                                    // storing the x acceleration   
float ballXPosition=4;                                   // storing the ball position as number

float ballYSpeed = 0;                                    // storing the y acceleration
float ballYPosition=4;                                   // storing the ball position as number

int imageYPosition;                                      // storing the image position as number
int imageXPosition;                                      // storing the image position as number

int player1Position=3;                                   // storing the paddle positoin of player 1
int player2Position=3;                                   // storing the paddle positoin of player 2

int gameSpeed;                                           // storing the current game speed

void setup(){
  for (int i=0; i<8; i++){                               // all pins are outputs
    pinMode(rowPins[i],OUTPUT);
    pinMode(colPins[i],OUTPUT);
  }
}

void draw(){
  for (int y=0; y<8; y++){                               // rowwise 
    for (int x=0; x<8; x++){                             // from left to right, entries are checked
      if (image[x][y]==1){                               // if entry equals 1
        digitalWrite(colPins[x],HIGH);                   // the column pin is switched on
      } else {                                           // else
        digitalWrite(colPins[x],LOW);                    // the column pin is switched off
      } 
    }
    digitalWrite(rowPins[y],LOW);                        // switch the row pin to LOW (because it is the cathod of the LED LOW menas ON)
    delayMicroseconds(1000);                             // stop the program for 1 seconds
    digitalWrite(rowPins[y],HIGH);                       // switch the row pin to HIGH (what means OFF)
  }
}

void update(){
  switch (gameState) {                                   // switching game mode (called state machine)
  case 0:                                                // new game
    memcpy(image,blank,sizeof(blank));                   // clear screen
    gameSpeed=300;                                       // set the game speed
    ballXPosition=3;                                     // set ball position
    ballYPosition=3;                                     // set ball position
    ballYSpeed=0;                                        // ball should fly straight
    if (random(0,2)>0){                                  // but randomly left or right
      ballXSpeed=1;
    } else {
      ballXSpeed=-1;
    }     
    theTimer=millis(); 
    gameState=1;
    break;
  case 1:                                                // game active
    image[player1Position][0]=0;                         // paddle player 1 clear old position
    image[player1Position+1][0]=0;                       // paddle player 1 clear old position
    image[player2Position][7]=0;                         // paddle player 2 clear old position
    image[player2Position+1][7]=0;                       // paddle player 2 clear old position

    player1Position=map(analogRead(pot1Pin),0,1023,0,6); // reading the position of player 1
    player2Position=map(analogRead(pot2Pin),0,1023,0,6); // reading the position of player 2
    
    image[player1Position][0]=1;                         // paddle player 1 display             
    image[player1Position+1][0]=1;                       // paddle player 1 display
    image[player2Position][7]=1;                         // paddle player 2 display
    image[player2Position+1][7]=1;                       // paddle player 2 display
  
    if (millis()>theTimer+gameSpeed){                    // timer for game speed
      if (gameSpeed>50) gameSpeed-=3;                    // accelerate game
      theTimer=millis();                                 // set new timer
      image[imageYPosition][imageXPosition]=0;           // overwrite old position
      ballXPosition+=ballXSpeed;                         // update position
      ballYPosition+=ballYSpeed;                         // update position      
      
      if (ballYPosition>=7) ballYSpeed*=-1;              // collision bottom border
      if (ballYPosition<=0) ballYSpeed*=-1;              // collision top border

      ballYPosition=constrain(ballYPosition,0,7);        // constrain values between 0 and 7
      ballXPosition=constrain(ballXPosition,0,7);        // constrain values between 0 and 7
      imageYPosition=round(ballYPosition);                
      imageXPosition=round(ballXPosition);      
      
      if ((ballXPosition>=6)&&(image[imageYPosition][7]==1)) {  // if ball hits a paddle
        ballXSpeed*=-1;                                    // reflect the ball
        ballYSpeed=random(-2,3);                           // random reflection angle
      }
      if ((ballXPosition<=1)&&(image[imageYPosition][0]==1)) {  // if ball hits a paddle
        ballXSpeed*=-1;                                    // reflect the ball
        ballYSpeed=random(-2,3);                           // random reflection angle
      }
      if (ballXPosition>=7){                               // ball out
        gameState=2;                                       // change status to lost game
        theTimer=millis();                                 // new timer is set
      }

      if (ballXPosition<=0){                               // ball out
        gameState=2;                                       // change status to lost game
        theTimer=millis();                                 // new timer is set
      }

      image[imageYPosition][imageXPosition]=1;             // set new image position
      
    }
    break;  
  case 2:                                                  // game was lost
    if (millis()>theTimer+gameSpeed){                      // wait for a short time
      theTimer=millis();
      gameState=3;                                         // game state to lost game display
    }
    break;
  case 3:                                                  // lost game display
    memcpy(image,death,sizeof(death));                     // show image of lost game, memcpy is a function copying one array to another
    if (millis()>theTimer+animations){                     // wait again
      gameState=0;                                         // game state change to »start a game«
    }
    break;
  }
}

void loop(){
  update();                                                // all calculations are in the update method
  draw();                                                  // all display methods are in here
}
