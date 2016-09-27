/*Janus The Gate Keeper - JANUS MK-01 Ver.08b 05/Jun/2016
  Daniel Zulato Dias de Moraes - daniel_diasm@hotmail.com
  Project started 29 May 2016 - Started real world testing 16 Jun 2016
  Bluetooth Serial Version FOR ARDUINO NANO
  
Version 04 ALPHA -  Position sensors working.
Version 06 ALPHA -  Timer were introduced as hard coded values .
Version 06 ALPHA -  The command open or close by numbers has been removed, now it works with O for Open, C for Close and S for Stop.
Version 07 ALPHA -  Programable Timer Indroduced, when 0 timer is off, when more than 0 and less than 60 timer is marked in minutes,
                    if some value over 60 is inputed, it will mark 60 into the variable.
Version 08 ALPHA -  Comments improved, errorLED has been replaced by WarnLED. It will work on the gate warning.
*/

//#include <Wire.h> cannot be included in Arduino Nano I2C comms are not accepted by the 168 processor

//-----------------------------------------SetUp Variables
int btVal;  //Bluetooth Value Var
int oldBtVal;//Old Bluetooth Value
int gateStat; //Gate Position Status
int oldGateStat;//Before last Update Gate Status
int OpSenVal; //Reed Sensor Open
int ClSenVal; //Reed Sensor Down
int mvProgress;
int engineMsg;

char gateControl; //Gate Control Value

String statMesg; //Feedback Status Message String

//Time must be treated with unsigned long, otherwise may happen some bugs.
unsigned long cureTime; // Current Time
unsigned long prevTime; // Previous Marked Time
unsigned long closeTimer; // Timer for closing
unsigned long inputTimer; // If user wants to set up a time for the timer.

//-----------------------------------------Set up GPIO Pins
const int gateKey = 2; //Gate Activation Transistor
const int openSen = 11;//Reed Sensor on Open position
const int closSen = 12;//Reed Sensor on Closed position
const int WarnLED = 13;//LED that indicates a Warning Fault
const int motorState = 10; //Sensor for the electric engine working status -- NOT IMPLEMENTED YET

void setup() ////////////////////////////////////////////////// Set up I/O
{
  //-----------------------Variables SetUp
  prevTime = 0;
  btVal = '0';
  gateStat = '0';
  oldGateStat = '0';
  closeTimer = 300000;
  
  //-----------------------I/O Ports Configurations
  pinMode(gateKey, OUTPUT);
  pinMode(WarnLED,OUTPUT);
  pinMode(openSen, INPUT);
  pinMode(closSen, INPUT);
  pinMode(motorState, INPUT);
  
  //-----------------------Serial - For Input and Output of Data
  Serial.begin(9600);
}
  
void loop()/////////////////////////////////////////////////// Start Loop
{
//----------------------------------------------Bluetooth/Serial Input
   if (Serial.available())
     {     
       btVal = Serial.read(); 
     }
   
//---------------------------------------------Gate Control Filter
   if ((btVal == 'O')||(btVal == 'C')||(btVal == 'S'))
     {
       gateControl = btVal;
       btVal = 'R';
     }
   
//---------------------------------------------Timer Selector IF changed
   if ((btVal >= '1')||(btVal <= '3'))
    {
     
       if (btVal == '1')
         {closeTimer = 0;
          Serial.println("TIMER OFF");
          btVal = 0;}
       if (btVal == '2')
         {closeTimer = 300000;
          Serial.println("TIMER 5 MIN");
          btVal = 0;}
       if (btVal == '3')
         {closeTimer = 600000;
          Serial.println("TIMER 10 MIN");
          btVal = 0;}
       if (btVal == '4')
         {closeTimer = 900000;
          Serial.println("TIMER 15 MIN");
          btVal = 0;}
       if (btVal == '9')
         {closeTimer = 5000;
          Serial.println("TIMER ON TEST MODE");
          btVal = 0;}
      }
  
//--------------------------------------------Sensors readings
  OpSenVal = digitalRead(openSen);
  ClSenVal = digitalRead(closSen);
  mvProgress = digitalRead(motorState);

//-------------------------------------------Gate State Updater
  
  //Old value for comparison and avoid duplicated outputs
  oldGateStat = gateStat;

  //Reed sensors indicate Open position 0 and Closed Position 1. 
     if ((OpSenVal == LOW)&&(ClSenVal == HIGH))
      {
      gateStat = 1; //It is Closed.
      statMesg = "STAT-01 CLOSED";
      }
 //Reed sensors indicate Open position 1 and Closed Position 0.   
   else if ((OpSenVal == HIGH)&&(ClSenVal == LOW))
      {
      gateStat = 2; //It is Open.
      statMesg = "STAT-02 OPEN";
      }
 //Reed sensors indicate Open position 0 and Closed Position 0.  
   else if ((OpSenVal == LOW)&&(ClSenVal == LOW))
      {
      gateStat = 4; // It is stopped somewhere along the way.
      statMesg = "STAT-03 HALF-WAY";
      }
 //Reed sensors indicate Open position 1 and Closed Position 1.  
  else if ((OpSenVal == HIGH)&&(ClSenVal == HIGH))
      {
      gateStat = 3; //There is some short circuit - ERROR 4.
      statMesg = "STAT-04 ERROR";
      }
 
 //------------------------------------------Gate Timer - Current Time Updater
     cureTime = millis();
 
 //------------------------------------------Gate Electric Engine State
   if (mvProgress == HIGH)
     {
     // Serial.println("that shit is on...");
       
       if ((oldGateStat == 1)&&(gateStat == 3)){
           engineMsg = 1; //Opening
         }
         
       else if ((oldGateStat == 2)&&(gateStat == 3)){
           engineMsg = 2; //Closing
         }
        
        if (engineMsg == 1)
        {Serial.println("Openning, wait...");}
        
        if (engineMsg == 2)
        {Serial.println("Closing, wait...");}
     }
 //-------------------------------------------Serial State Updater
   if (gateStat != oldGateStat)
     {
      Serial.println("STATE HAS CHANGED");
      Serial.println(statMesg);

      if ((gateStat == 2)||(gateStat == 3)){
        prevTime = cureTime;
        }
     }
  if (closeTimer > 0)
  {  
    if ((cureTime - prevTime >= closeTimer)&&((gateStat == 2)||(gateStat == 3)))
     {
       if (gateStat == 2)
       {
       gateControl = 'C';
       prevTime = cureTime;
       }
       if (gateStat == 3)
       {
       gateControl = 'S';
       prevTime = cureTime;
       }
     }
  }     
 //delay(350);
 
//------------------------------------------------------------------Gate Controler
 if ((gateStat > 0)&&(gateStat < 4))
    {    
      digitalWrite (WarnLED, LOW); //If lit, the light goes off.
    
      //Gate OPEN
      if (gateControl == 'O')
        {
          if (gateStat == 1) //If closed, will open.
            {
              digitalWrite (gateKey, HIGH);
              delay (150);
              digitalWrite (gateKey, LOW);
              Serial.println("OKAY - NOW OPENNING");
              gateControl = 'F';
            }
          else //If already open, respond Already Open (AO) status.
            {
              Serial.println("ALREADY OPEN - EVENT-01");
              gateControl = 'F';
            }
        }
    
    
      //Gate CLOSE  
      if (gateControl == 'C')
        {
          if (gateStat == 2) //If open, will close.
            {
              digitalWrite (gateKey, HIGH);
              delay (150);
              digitalWrite (gateKey, LOW);
              Serial.println("OKAY - NOW CLOSING");
              gateControl = 'F';
            }
          else //If already close, respond Already Closed (AC) status.
            {
              Serial.println("ALREADY CLOSED - EVENT-02");
              gateControl = 'F';
            }
        }
    
      //Gate STOP - INVERT
      if (gateControl == 'S')
        {
          if (gateStat == 3) //If open, will close.
            {
              digitalWrite (gateKey, HIGH);
              delay (150);
              digitalWrite (gateKey, LOW);
               Serial.println("OKAY - MOVING");
              gateControl = 'F';
            }
          else //If already close, respond Not Stopped (NS) status.
            {
              Serial.println("NOT HALF-WAY - EVENT-03");
              gateControl = 'F';
            }
        }
    }
 //If Error is TRUE - Halt Janus Operation   
 else
    {
      if (gateStat == 4)
        {
          Serial.println("ERROR 4");
          digitalWrite (WarnLED, HIGH);
        }
    }

}
