//Jonathan Balina
///////////////////////////////////////////////////////////////
//Car with Wireless Speed Control:
//The car uses a wireless transciever module to recieve messages from 
//two separate TowerStack Controllers. The controllers will send a message
//telling the car to increase speed, each controller telling a different direction.
//The speed variable with then be updated and the bidirectional DC motor will increase
//speed in the indicated direction

//input: Wireless Tranciever Module
//output: Motor

///////////////////////////////////////////////////////////////

#include <Wire.h>
#include <Adafruit_MotorShield.h>     //by adafruit https://github.com/adafruit/Adafruit_Motor_Shield_V2_Library
#include <SPI.h>  
#include "RF24.h"                     //by TMRh20 https://tmrh20.github.io/RF24/
RF24 myRadio (6, 7); //sets the pins being used by wireless transceiver module
struct package //type of data to be recieved
{
  int change = 0;
};
byte addresses[][6] = {"0"};
typedef struct package Package;
Package data;
int spd = 0; //variable to control speed
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); // Create the motor shield object with the default I2C address
Adafruit_DCMotor *myMotor = AFMS.getMotor(1); //Select motor port M1

void setup() {
  //Serial.begin(115200);

  //Set up wireless transciever module to LISTEN for messages
  ////////////////////////////////////////////////////////////
  myRadio.begin(); 
  myRadio.setChannel(115); 
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate( RF24_250KBPS ) ; 
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();
  AFMS.begin();
  ///////////////////////////////////////////////////////////// 
  myMotor->run(RELEASE);
  delay(1000);
}

void loop() {
  //waits to recieve a message from one of the controllers
  if ( myRadio.available()) 
  {
    //read data from the message
    while (myRadio.available())
    {
      myRadio.read( &data, sizeof(data) );
    }
    spd += data.change; //Update speed variable from info from the message
    //Use speed variable to change speed
    //Twenty speed levels per direction
    ////////////////////////////////////////////////////////////
    if(spd > 0){
      if(spd > 20)//Don't go past 20 because maximum is 255(so max speed of this car will be at 250)
        spd = 20;
      myMotor->run(FORWARD);
      myMotor->setSpeed((spd*10)+50); //I added 50 because that is around the time the motor actually starts moving
    }
    else if(spd < 0){
      if(spd < -20)
        spd = -20;
      myMotor->run(BACKWARD);
      myMotor->setSpeed((spd*-10)+50);
    }
    //At zero the cart will not move
    else
      myMotor->run(RELEASE);
    ///////////////////////////////////////////////////////////////

  }
}
