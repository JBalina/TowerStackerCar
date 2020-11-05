//Jonathan Balina

////////////////////////////////////////////////////////////
//TowerStack Controller:
//Tower stacking game displayed on an 8x8 led display, controlled 
//with a one button input. Three lights will move left to right on the bottom 
//layer and then will stay in place after the button is pushed. Then the same 
//will happen on next layer up and the goal will be to stop the lights when they 
//line up with the previous layer. The speed at which the line moves will increase
//as the tower gets taller. Failing to line up the layers perfectly will 
//result in smaller layers going up. When the player misses the previous layer 
//completely the game will delay for five seconds before restarting.
//With every level that the player's tower gets taller, the controller
//will send a message to the car telling it to increase speed in its direction.

//Input: Button
//Output: 8x8 LED Display and wireless transciever module

////////////////////////////////////////////////////////////


#include <Wire.h>
#include "Adafruit_LEDBackpack.h"     //by adafruit https://github.com/adafruit/Adafruit_LED_Backpack
#include "Adafruit_GFX.h"             //by adafruit https://github.com/adafruit/Adafruit-GFX-Library
#include <SPI.h>  
#include "RF24.h"                     //by TMRh20 https://tmrh20.github.io/RF24/
#ifndef _BV
#define _BV(bit) (1<<(bit))
#endif
Adafruit_8x8matrix matrix = Adafruit_8x8matrix();
RF24 myRadio (7, 8); //sets wireless transciever module to use pins 7 and 8
byte addresses[][6] = {"0"};
bool input = false; //true when button is pressed
bool toggle = true; //determines if the line is moving left or right
int x = 0; //x coordinate of line
int y = 0; //y coordinate of line
int width = 3; //length of line
int tower[8][5]; //2 dimenional grid to save the data of the tower, used for recognizing if current stack lines up and for scrolling
int dif=250; //the amount of milliseconds the line will show before moving to next frame, will decrease as the game goes meaning the line will move faster
long lastDebounceTime = 0; //for debouncing, will compare lastDebounceTime with current time to see if bouncing is occurring
long debounceDelay = 250; //how long to wait to determine if bouncing
bool lose = false; //true if player drops the stack
struct package // The data that will be sent to the car
{
  int change = -1; // For the other controller, this is just 1 instead of -1
};
typedef struct package Package;
Package data;


void setup() {
  //matrix set up
  //////////////////////////////////////////////////////////
  //Serial.begin(115200);
  //Serial.println("HT16K33 test");
  matrix.begin(0x70); // pass in the address
  matrix.setBrightness(0); // defualt brightness is too bright
  matrix.setRotation(3); // rotate entire grid 90 degrees
  //////////////////////////////////////////////////////////
  
  //Set up button and interrupt for user input
  //////////////////////////////////////////////////////////
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), button, RISING);
  delay(1000);
  //////////////////////////////////////////////////////////
  
  //Turn on and set wireless trnsmission reciever to SEND packages
  //////////////////////////////////////////////////////////
  myRadio.begin();  
  myRadio.setChannel(115); //must match channel with other devices
  ///In order to maximize range:
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate( RF24_250KBPS ) ; 
  myRadio.openWritingPipe( addresses[0]);  
  ///////////////////////////////////////////////////////////
  delay(1000);

}

void loop() {
  //lose condition, if player drops the stack (fails to match previous layer completely)
  //waits a five seconds penalty and display a sad face
  ///////////////////////////////////////////////////////////
  if(lose) {
    //Just some pixel art
    matrix.drawPixel(2,6,0xF800);
    matrix.drawPixel(2,5,0xF800);
    matrix.drawPixel(5,6,0xF800);
    matrix.drawPixel(5,5,0xF800);
    matrix.drawPixel(1,2,0xF800);
    matrix.drawPixel(2,3,0xF800);
    matrix.drawPixel(3,3,0xF800);
    matrix.drawPixel(4,3,0xF800);
    matrix.drawPixel(5,3,0xF800);
    matrix.drawPixel(6,2,0xF800);
    matrix.writeDisplay();
    delay(5000);
    matrix.clear();
    //reset lose condition to false to reset game
    lose = false;
    input = false;
  }
  /////////////////////////////////////////////////////////////

  //I adjusted for debouncing by checking when the last time input was provided
  else {
    //width keeps track of how long the line is
    for(int i = 0; i < width; i++) { //draw line with saved width
      matrix.drawPixel(x+i, y, 0xF800); //0xF800 just refers to color red
    }  
    matrix.writeDisplay();
    delay(dif);//dif represents the speed the line moves, dif will get smaller as the tower grows
    //checks if input is from bouncing, if so then disregards input
    if(input && (millis() - lastDebounceTime) <= debounceDelay) {
      input = false;
    }
    else if(input && (millis() - lastDebounceTime) > debounceDelay){ //if input recieved and not from bouncing
      //records and saves where the line is at point of button push
      ///////////////////////////////
      for(int i=0; i<8; i++) {
        if(i>=x && i<x+width)
          tower[i][y] = 1;
        else
          tower[i][y] = 0;
      }
      ///////////////////////////////
      if(y>0 && y!=5){ //If not the starting line and not at the top of the stack/screen
        //Check if line matched
        ///////////////////////////////////////////////
        for(int i=0; i<8; i++) {
          if(tower[i][y]==1 && tower[i][y-1]==0) { //if part of line does not match previous layer
           matrix.drawPixel(i, y, 0);//turns off that light
           tower[i][y] = 0;//saves that space as empty
           width--; //decrease width of line
          }
        }
        ////////////////////////////////////////////////
      }
      y++;//increases height of tower
      dif*=.9;//increase speed of line
      
      
      //if reaches "top" of screen(really just five lines up), then the screen should
      //scroll up. It does this by erasing the entire grid and rewriting it from the 
      //data that was saved but one layer lower
      //////////////////////////////////////////////////
      if(y==5) {//if tower reached "top" of screen
        matrix.clear();//erases grid
        for(int i=0; i<4; i++) {
          for(int j=0; j<8; j++) {
            tower[j][i] = tower[j][i+1];//each layer is now the layer that was previously above it
            if(tower[j][i]==1)//turns on light if the light was on on the layer previously above
              matrix.drawPixel(j, i, 0xF800);
          }
        }
        y--; //since scrolled up there is one less layer on screen
      }
      ////////////////////////////////////////////////////

      //lose condition: current layer did not match with previous one at all
      //resets game and initializes penalty
      ////////////////////////////////////////////////////
      if(width==0) {
        y=0; //back to layer one
        width = 3; //back to original line width
        dif = 250; //back to original speed
        matrix.clear(); //clear tower
        lose = true; //initialize lose condition
      }
      /////////////////////////////////////////////////////
      else
        myRadio.write(&data, sizeof(data)); //if successfull stack then sends message to car
      input = false; //reset input
      x = 0; //x is the location of the line. line always starts from edge of screen
      toggle = true; //toggle controls direction the line is moving. line always starts from right going left
      lastDebounceTime = millis();
    }
    //else no input, the line just moves left and right
    //x represents where on the x axis of the grid the line is located
    ///////////////////////////////////////////////////////
    else {
      for(int i = 0; i < width; i++) {//Line is drawn with saved width
        matrix.drawPixel(x+i, y, 0);
      }
      if(x+width-1<7 && toggle) //if line has not yet reached edge of screen, then moves left
        x++;
      else if(x+width-1 == 7) { //if hits the edge then changes direction and moves right
        x--;
        toggle = false;
      }
      else if(!(toggle) && x!= 0)//if line has not yet reached edge of screen, then moves right
        x--;
      else if(x == 0) { //if hits the edge then changes direction and moves left
        x++;
        toggle = true;
      }
    }
    /////////////////////////////////////////////////////////
  }

}

void button() { //when button is pressed, triggers input path in loop
  input = true;
}
