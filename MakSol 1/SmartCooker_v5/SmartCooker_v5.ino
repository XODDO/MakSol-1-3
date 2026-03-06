/* 27 OCT 2022 EDITION
  XODDOCODE 4.0 Design Pattern -- 19 Mar 2022
  ESP32 driven Cooker - superfast task time allocation and millis second precision monitoring
  uses TM1637 7seg 4 digit i2c as display
  with Grove 4-digit display 1.0.0 library by Seeed Studio
*/
#include "Arduino.h"
#include <EEPROM.h> // to store changing battery states to prevent ON-OFF loops
/*
//#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial BT;
*/
#include "TM1637.h"
#define CLK 22 //22 //18
#define DIO 23 // 23//4

TM1637 Screen(CLK, DIO);

// --- UNIVERSAL INDICATOR
 const int LED = 2;
 const int LED2  = 21;//23
 const int sound = 33;

// --- ANALOG INPUTS
 const int readBattery = 34;//26;

// --- PWM OUTPUTS
  const int Coil_1 = 13; // 14;//33
  const int Coil_2 = 25;
  const int fan_1 = 12;
  const int fan_2 = 27;
  const int SolarCharge = 26;//23; //12; //14;

// --- TOUCH INPUTS
 const int touch_1 = 15;   // up 1
 const int touch_2 = 36; //27; //15; // down 1
 const int touch_3 = 4; //13; // up 2
 const int touch_4 = 39; //16;// down 2

 const int power = 35; //17; //const int powerLED = 34;
// --- LED BATTERY LEVEL INDICATORS
  const int level1 = 32;
  const int level2 = 18;
  const int level3 = 19;

// HIGHLY VOLATILE STATES
  bool LED1_ON = false; 
  bool LED2_ON = false;
  bool LED3_ON = false;

uint8_t power_drain_counter = 0; //memory to store for how long battery has cycled between ON & OFF
bool just_dropped = false;

void beep(uint8_t times, bool battery_empty);

void setup() {  
//ATTACK THE COILS first --- use direct port access to lower response to nanosex 
  pinMode(Coil_1, OUTPUT);  digitalWrite(Coil_1, HIGH); //ACTIVE WHEN LOW
  pinMode(Coil_2, OUTPUT);  digitalWrite(Coil_2, HIGH); //ACTIVE WHEN LOW


      //digitalWrite(Coil_1, LOW); //ACTIVE WHEN LOW
      //digitalWrite(Coil_2, LOW); //ACTIVE WHEN LOW

  pinMode(fan_1, OUTPUT); pinMode(fan_2, OUTPUT); // ACTIVE HIGH


  //BT.begin("SmartCooker Mini");

  // THEN THE DISPLAYS
Serial.begin(115200); delay(100); Serial.flush(); Serial.println("Booting...");

  pinMode(LED, OUTPUT); digitalWrite(LED, HIGH);//INDICATOR LED

  pinMode(power, INPUT); 

bool activated = false, firstPass = false;
bool arrested = false;

int detectPress;

unsigned long readStart = 0;

//OFF
while(!activated){

       detectPress = digitalRead(power);
       if(detectPress) arrested = true;
       else {arrested = false; readStart = millis(); }

    if(arrested && !firstPass){readStart = millis(); firstPass = true;}
    if(firstPass == true){

        if((millis() - readStart) > 800) activated = true; //LONG PRESS
    } 
}

  digitalWrite(fan_1, HIGH); digitalWrite(fan_2, HIGH);

  Screen.init();
  Screen.set(6); // SET BRIGHTNESS OF THE 7 SEGMENT
  delay(100); //time to launch the serial monitor
  
  int foo = 9; 
  while(foo>=0) { delay(75);
        Screen.display(0, foo);
        Screen.display(3, foo);
        foo--;
      }


  
    pinMode(level1, OUTPUT);    digitalWrite(level1, HIGH);  LED1_ON = true; 
    pinMode(level2, OUTPUT);    digitalWrite(level2, HIGH);  LED2_ON = true; 
    pinMode(level3, OUTPUT);    digitalWrite(level3, HIGH);  LED3_ON = true; 
  
  
pinMode(LED2, OUTPUT);  digitalWrite(LED2, HIGH); //POWER LED

    pinMode(sound, OUTPUT); 
    uint8_t beep = 0;
    //beep(3);
 do{ digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50); beep++; } while(beep < 2);
 

    delay(200);

  pinMode(SolarCharge, OUTPUT); //digitalWrite(SolarCharge, LOW);//ACTIVE HIGH
    
 //for(;;)Serial.println(digitalRead(touch_2)); //ANALOGREAD SEES THE MF* BITCH

pinMode(touch_1, INPUT); pinMode(touch_2, INPUT);
pinMode(touch_3, INPUT); pinMode(touch_4, INPUT);

  digitalWrite(fan_1, LOW); digitalWrite(fan_2, LOW);


power_drain_counter = EEPROM.read(10);
Serial.println("Boot Complete!");
}

 bool isPower_enough = false; bool isCharging = false;
  uint8_t Battery_Level_Counter; bool end_processes = false;
//float voltage = 0.00;

bool recoiled = false;
float voltage = 0.00;

void batteryLevel(){
 //Battery_Level_Counter = 0;
  uint32_t summation = 0;

uint16_t reading;

  for(int i=0; i<7; i++){ //5 samples
      reading = analogRead(readBattery);
      summation += reading;
  }
    summation /= 7;

  Serial.println();
  Serial.print("Raw Reading: "); Serial.println(reading);
  voltage = 14.70 * float(summation)/3095.0;
  
  if(voltage < 11.00) { //too low battery power
     Battery_Level_Counter = 0;
     isPower_enough = false; Serial.println("Power Too Low!!!");
     
     if(LED1_ON) {digitalWrite(level1, LOW); LED1_ON = false;}
     if(LED2_ON) {digitalWrite(level2, LOW); LED2_ON = false;}
     if(LED3_ON) {digitalWrite(level3, LOW); LED3_ON = false;}
         
    if(!isCharging) { digitalWrite(SolarCharge, HIGH); isCharging = true; } // ALWAYS PREVENT DEEP DISCHARGE when there is sunshining...

     if(voltage <= 10.80) {//kill all RUNNING TASKS...
        end_processes = true;
      }
  }
  else if(voltage >= 11.00 && voltage < 11.60) {Battery_Level_Counter = 1; isPower_enough = true; end_processes = false;
        if(!LED1_ON) {digitalWrite(level1, HIGH); LED1_ON = true; }
        if(LED2_ON)  {digitalWrite(level2, LOW); LED2_ON = false; } 
        if(LED3_ON)  {digitalWrite(level3, LOW); LED3_ON = false; }
        
        if(!isCharging) { digitalWrite(SolarCharge, HIGH); isCharging = true; }} //PWM THE CHARGING VOLTAGE
        
  if(voltage >= 11.60 && voltage < 12.70) {Battery_Level_Counter = 2; isPower_enough = true;
      if(!LED2_ON) {digitalWrite(level2, HIGH); LED2_ON = true; } //LOAD RECONNECT SEQRNCE AT 12.8 --- CHARGING VOLTAGE
      if(!LED1_ON) {digitalWrite(level1, HIGH); LED1_ON = true; }
      if(LED3_ON)  {digitalWrite(level3, LOW);  LED3_ON = false;}
      
      if(voltage <= 12.50) if(!isCharging) { digitalWrite(SolarCharge, HIGH); isCharging = true; }} //pulse width modulate the charging voltage

  if(voltage >= 12.70) {Battery_Level_Counter = 3; isPower_enough = true;
      if(!LED3_ON) {digitalWrite(level3, HIGH); LED3_ON = true; }
      if(!LED2_ON) {digitalWrite(level2, HIGH); LED2_ON = true; }
      if(!LED1_ON) {digitalWrite(level3, HIGH); LED1_ON = true; }
      
      if(voltage >= 14.30) if(isCharging) { digitalWrite(SolarCharge, LOW); isCharging = false; } // PREVENT OVERCHARGE
      }

  
    Serial.print("VOLTAGE: "); Serial.println(voltage);

    

}

 long now = 0, then = 0, stat = 0; 

int currentMode = 1;
uint8_t heat_1_level = 0;
uint8_t heat_2_level = 0;


int up1=0; int up2=0; //const int threshold_up = 30; //for touch abilities

int down1=0; int down2=0; //const int threshod_dn = 40; // for touch abilities


uint8_t CoilNum = 0; uint8_t whichCoil = 0;
bool Coil_1_ON = false, Coil_2_ON = false;
bool waiting = false;

long interval = 0; long interval2 = 0;
bool waiting2 = false;
long stat2 = 0;

const   long wait_ko = 20000; // 20 sec ideal Cooling time \\ 300,000

const   long interval_1 = 300000; //  5 minutes
const   long interval_2 = 600000; // 10 minutes
const   long interval_3 = 900000; // 15 minutes
const   long interval_4 = 1200000; // 20 minutes
const   long interval_5 = 1800000; // 30 minutes
const   long interval_6 = 2700000; // 45 minutes

unsigned long idle = 0;

void loop() {
/* ONLY FIVE TASKS:
 * Read battery level
 * Listen to touch interrupts
 * Listen to bluetooth interrupts
 * Update cooking coils manager
 * Shutdown when pressed or when idle for 20 minutes 
 */

  batteryLevel();
  touchReader();
//bluetoothScanner();
  Monitor(heat_1_level, heat_2_level);

  listen_to_shutdown(); 
  Serial_Debugger();
}


void listen_to_shutdown(){ //go to deep sleep
       //int readBtn = digitalRead(power);
       //     if(readBtn) setup(); //kill all running processes

  idle = millis();

  unsigned long readStart = 0;
  bool activated = false, firstPass = false;
  bool arrested = false;

  int detectPress;

}

String CMD = ""; char letter;

/*
void bluetoothScanner(){

       if(BT.available()) {letter = BT.read();}
       if(letter == '1') { up1 = 1; touchProcessor(); }
       if(letter == '2') { down1 = 1; touchProcessor();}

       if(letter == '3') { up2 = 1; touchProcessor();}
       if(letter == '4') { down2 = 1; touchProcessor();}

}
*/
  bool locked_up = false;   bool locked_down = false;
  bool locked_up2 = false;  bool locked_down2 = false;

  bool lock_press = false, passed_yet = false;
  unsigned long start_counting = 0;

void touchReader(){
  up1 = 0; down1 = 0;
  up2 = 0; down2 = 0;
  lock_press = false;
  

//lock max at 5 and min at 0 ... no press beyond those limits
if(!locked_up) {    up1 = digitalRead(touch_1); /*if(up1) {delay(150); touchProcessor();}*/}
if(!locked_down){ down1 = digitalRead(touch_2); /*if(down1) { delay(150); touchProcessor();}*/}
if(!locked_up2) {   up2 = digitalRead(touch_3); /*if(up2) {delay(150); touchProcessor();}*/}
if(!locked_down2){down2 = digitalRead(touch_4); /*if(down2) { delay(150); touchProcessor();}*/}


if(digitalRead(touch_1) || digitalRead(touch_2) || digitalRead(touch_3) || digitalRead(touch_4)) lock_press = true;
else start_counting = millis(); // /1000; //to prevent overflow when statted at 4.2Bn

      if(lock_press && !passed_yet) {start_counting = millis(); passed_yet = true;}
        if(passed_yet){
          if((millis() - start_counting) >= 50 && (millis() - start_counting) < 80) {touchProcessor();}
        }
    


//recoiled = true; EEPROM.update(10, 1);
//OFF
/*
if(!activated){

       detectPress = digitalRead(power);
       if(detectPress) arrested = true;
       else {arrested = false; readStart = millis(); }

    if(arrested && !firstPass){readStart = millis(); firstPass = true;}
    if(firstPass == true){

        if((millis() - readStart) > 800) activated = true; Go to Deep Sleep
    } 
}
*/

}

bool reducing = false;
//Power Mgt Activated: ONCLICK ---- basic Power Saving mode
void touchProcessor(){ // LEVEL SHIFTER WITH SEMI-DYNAMIC POWER MANAGEMENT INTEGRATED
   if(Battery_Level_Counter < 1) { //only react to LEVEL LOWERING when in ultra low power mode
        if(down1 == 1) {  
              if(heat_1_level > 0) {locked_down=false; locked_up=false; heat_1_level--; } 
               else {heat_1_level = 0; locked_down = true; locked_up=false; } 
                    Screen.display(0, heat_1_level); reducing = true; }
        
        if(down2 == 1) { 
              if(heat_2_level > 0) {locked_down2=false; locked_up2=false; heat_2_level--; } 
               else {heat_2_level = 0; locked_down2 = true; locked_up2=false; } 
                     Screen.display(3, heat_2_level); }
           
            if(up1 || up2) beep(2, true); // battery warning is true

        } //if in shifting gear, power is discovered to be low
   
   else {
    if(up1 == 1) {  if(heat_1_level < 5) { locked_up=false; locked_down=false; //free_run
                        if(Battery_Level_Counter == 3 && heat_1_level < 5)  heat_1_level++;  //maxes out at 5:: FULL BLAST
                        if(Battery_Level_Counter == 2 && heat_1_level < 4) {heat_1_level++; if(heat_1_level == 4) locked_up = true;} //maxes out at 4
                        if(Battery_Level_Counter == 1 && heat_1_level < 3) {heat_1_level++; if(heat_1_level == 3) locked_up = true;} //maxes out at 3
                        } 

                  else { locked_up = true; locked_down=false;
                        if(Battery_Level_Counter == 1) heat_1_level = 3;  
                        if(Battery_Level_Counter == 2) heat_1_level = 4;  
                        if(Battery_Level_Counter == 3) heat_1_level = 5;       }
               Screen.display(0, heat_1_level); CoilNum = 1; reducing = false; }

if(down1 == 1) {  if(heat_1_level > 0) {locked_down=false; locked_up=false; heat_1_level--; } 
               else {heat_1_level = 0; locked_down = true; locked_up=false; } 
                    Screen.display(0, heat_1_level); reducing = true; }
  
if(up2 == 1) {  if(heat_2_level < 5) {locked_up2=false; locked_down2=false; //free_run
                        if(Battery_Level_Counter == 3 && heat_2_level < 5) heat_2_level++;  //maxes out at 5:: FULL BLAST
                        if(Battery_Level_Counter == 2 && heat_2_level < 4) {heat_2_level++; if(heat_2_level == 4) locked_up2 = true;}  //maxes out at 4
                        if(Battery_Level_Counter == 1 && heat_2_level < 3) {heat_2_level++; if(heat_2_level == 3) locked_up2 = true;}//maxes out at 3
                         } 
               
                else {locked_up2 = true; locked_down2=false;
                        if(Battery_Level_Counter == 1) heat_2_level = 3;  
                        if(Battery_Level_Counter == 2) heat_2_level = 4;  
                        if(Battery_Level_Counter == 3) heat_2_level = 5;   } 
                    
                    Screen.display(3, heat_2_level); CoilNum = 2;  }
  
if(down2 == 1) { if(heat_2_level > 0) {locked_down2=false; locked_up2=false; heat_2_level--; } 
               else {heat_2_level = 0; locked_down2 = true; locked_up2=false; } 
                     Screen.display(3, heat_2_level); }

   }

   //universal tactile feedback
if(up1 == 1 || up2 == 1 || down1 == 1 || down2 == 1){ digitalWrite(LED2, HIGH); beep(1, false); digitalWrite(LED2, LOW);}

Serial.println();
Serial.print("Coil 1 Level: "); Serial.println(heat_1_level);
Serial.print("Coil 2 Level: "); Serial.println(heat_2_level);
Serial.println();
//vTask.execute();

  //return 0;
}

unsigned long start_beeping = 0;
unsigned long sounding_duration; 

bool sound_started = false;
bool sound_locked = false; 

void beep(uint8_t times, bool battery_empty){
/*
  for(int xy=0; xy<times; xy++){

      if(!sound_started) { start_beeping = millis(); digitalWrite(sound, HIGH);  sound_started = true;  }
      if (sound_started) {
        if((millis() - start_beeping) > 50) { 
           digitalWrite(sound, LOW); sound_started = false;
        }
      }

      //if(debounceDelay)
  }
*/
      for(int xy=0; xy<times; xy++){
        digitalWrite(sound, HIGH);
        
        if(battery_empty)delay(100);
        else delay(50);
        
        digitalWrite(sound, LOW);
        delay(50);
      }
  
  }


void Monitor(uint8_t coil_1_index, uint8_t coil_2_index){

if(Battery_Level_Counter < 1) { //if POWER IS ALARMINGLY LOW
  if(Coil_1_ON) {digitalWrite(fan_1, LOW); //switch OFF
                  digitalWrite(Coil_1, HIGH); Coil_1_ON = false; waiting = false;
                  stat = 0; Serial.println("Turned OFF"); }
  if(Coil_2_ON) { digitalWrite(fan_2, LOW); //switch OFF
                  digitalWrite(Coil_2, HIGH); Coil_2_ON = false; waiting2 = false;
                  stat2 = 0; Serial.println("Turned OFF"); }

    if(waiting)  {digitalWrite(fan_1, LOW); waiting  = false;}
    if(waiting2) {digitalWrite(fan_2, LOW); waiting2 = false;}
  }

else {
  //kozesa coil 1 index otambuze coil 1 state ... and likewise 2 4 2
  if(coil_1_index == 0){ Serial.println("Coil 1 OFF!"); // if either we haven't started or they have reduced all the way to zero
   if(Coil_1_ON) {digitalWrite(fan_1, LOW); //switch OFF
                  digitalWrite(Coil_1, HIGH); Coil_1_ON = false; waiting = false;
                  stat = 0; Serial.println("Turned OFF"); }// 
    if(waiting) {digitalWrite(fan_1, LOW); waiting = false;}
    } 
//close if zero

else if(coil_1_index != 0) { //if toggled to a non zero integer
     if(Coil_1_ON) { 
        Serial.print("Coil "); Serial.print(CoilNum); Serial.print(" running for: "); 
        Serial.println((1000 + stat + interval - millis())/1000);
        
       }
 else if(!Coil_1_ON) { //check if it hasn't alreay been turned ON
      if(!waiting) { //turn it ON if it is not in waiting state
          digitalWrite(fan_1, HIGH); 
          digitalWrite(Coil_1, LOW); Coil_1_ON = true; stat = millis();
             Serial.print("Coil "); Serial.print(CoilNum); Serial.println("FIRED !!!"); 
             Serial.print("At: "); Serial.println(stat/1000);
            }
        else if(waiting){
            Serial.print("\tWaiting for ... "); Serial.println(((1000 + stat + interval + wait_ko) - millis())/1000);
        }
      }

  switch(coil_1_index){ 
      case 1:  interval = interval_1; running(interval); Serial.print("Level: "); Serial.println(interval);  
            break;

      case 2: interval = interval_2; running(interval); Serial.print("Level: "); Serial.println(interval); 
              break;

      case 3: interval = interval_3; running(interval);  Serial.print("Level: "); Serial.println(interval); 
              break;

      case 4: interval = interval_4; running(interval);   Serial.print("Level: "); Serial.println(interval); 
              break;
      
      case 5: interval = interval_5; running(interval);   Serial.print("Level: "); Serial.println(interval); 
              break;
     
    }

  }//closing if not zero
  
if(coil_2_index == 0){ Serial.println("Coil 2 OFF!"); // if they have reduced to zero
   if(Coil_2_ON) {digitalWrite(fan_2, LOW); //switch OFF
                  digitalWrite(Coil_2, HIGH); Coil_2_ON = false; waiting2 = false;
                  stat2 = 0; Serial.println("Turned OFF"); }// 
    if(waiting2) {digitalWrite(fan_2, LOW); waiting2 = false;}
    }
//close if zero

else if(coil_2_index != 0) {  //if toggled to a non zero integer
     if(Coil_2_ON) { 
        Serial.print("Coil "); Serial.print(CoilNum); Serial.print(" running for: "); 
        Serial.println((1000 + stat2 + interval2 - millis())/1000);
       }
 else if(!Coil_2_ON) { //check if it hasn't alreay been turned ON
      if(!waiting2) { //turn it ON if it is not in waiting state
          digitalWrite(fan_2, HIGH); 
          digitalWrite(Coil_2, LOW); Coil_2_ON = true; stat2 = millis();
             Serial.print("Coil "); Serial.print(CoilNum); Serial.println("FIRED !!!"); 
             Serial.print("At: "); Serial.println(stat2/1000);
            }
        else if(waiting2){
            Serial.print("\tWaiting for ... "); Serial.println(((1000 + stat2 + interval2 + wait_ko) - millis())/1000);
        }
      }


  switch(coil_2_index){ 
      case 1: interval2 = interval_1; running(interval2); Serial.print("Level: "); Serial.println(interval2);  
            break;

      case 2: interval2 = interval_2; running(interval2); Serial.print("Level: "); Serial.println(interval2); 
              break;

      case 3: interval2 = interval_3; running(interval2);  Serial.print("Level: "); Serial.println(interval2); 
              break;

      case 4: interval2 = interval_4; running(interval2);   Serial.print("Level: "); Serial.println(interval2); 
              break;
      
      case 5: interval2 = interval_5; running(interval2);   Serial.print("Level: "); Serial.println(interval2); 
              break;
     
      }

    }//closing if coil 2 not zero

 }  //close if power is enough

//close cooking monitor  
}


void running(long intervo){ // , uint8_t COIL_NO

    Serial.print("Running Time: "); Serial.println(intervo); 

              if(millis() - stat < interval) { Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON");
                  /*Serial.print(" Firing till ");  Serial.println((1+interval + stat)/1000);*/  /*if(reducing) waiting = true;*/}
              else { // if has been ON for more than first interval
                  if(millis() - stat < (interval + wait_ko)){ // --- TURN OFF & induce internal FORCED COOLING
                     if(Coil_1_ON) { waiting = true; //Turn off COIL to COOL
                        digitalWrite(Coil_1, HIGH); /*digitalWrite(fan_1, LOW);*/ Coil_1_ON = false;
                       }
                       else {Serial.print("Waiting Till "); Serial.println((interval + stat + wait_ko)/1000); }
                    }
                    else { waiting = false; Serial.println("Waiting Time Over - !"); } // --- WAITING TIME OVER 
               }

    

              if(millis() - stat2 < interval2) { Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON"); }
                
              else { // if has been ON for more than first interval
                  if(millis() - stat2 < (interval2 + wait_ko)){ // --- TURN OFF & induce internal FORCED COOLING
                     if(Coil_2_ON) { waiting2 = true; //Turn off COIL to COOL
                        digitalWrite(Coil_2, HIGH); /*digitalWrite(fan_2, LOW);*/ Coil_2_ON = false;
                       }
                       else {Serial.print("Waiting Till "); Serial.println((interval2 + stat2 + wait_ko)/1000); }
                    }
                    else { waiting2 = false; Serial.println("Waiting Time Over - !"); } // --- WAITING TIME OVER 
        


    }

}


//single serial handler
void Serial_Debugger(){
/*
  Serial.print("Time: "); Serial.println(now); Serial.println();
  Serial.print("Coil 1: "); Serial.println(Coil_1_ON?"ON":"OFF");
  Serial.print("Coil 2: "); Serial.println(Coil_2_ON?"ON":"OFF");
 */

Serial.println();
Serial.print("Voltage: "); Serial.println(voltage);
Serial.print("Level: "); Serial.println(Battery_Level_Counter);

/*
  Serial.print("UP1 = "); Serial.println(up1);
  Serial.print("UP2 = "); Serial.println(up2);
  Serial.print("DOWN1 = "); Serial.println(down1);
  Serial.print("DOWN2 = "); Serial.println(down2);
*/

//now = millis();
//Serial.println(); Serial.print("\t\tTime Now: "); Serial.println(now/1000); 
}
