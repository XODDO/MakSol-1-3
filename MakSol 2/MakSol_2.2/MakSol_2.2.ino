/* 4th MAR 2023 EDITION ~650lines
  --- XODDOCODE 5.0 Design Pattern ---
  * VERY MINIMAL DELAY functions - 2 implementations: sound & shut-down
  * SHUT DOWN ENABLED
  * INTELLIGENT POWER MGT => Low Voltage AUTO Load disconnect, Charging indication & Over charge prevention
  * 4-KEY NAVIGATION
  * HIGH PRECISION SCHEDULING => superfast (millis()) time allocation
  * BASED ON 6-DIGIT TM1637 7seg DISP, uses pnly 2 digits
*/
#include "Arduino.h"
#include <TM1637TinyDisplay6.h>

//#include <TM1637TinyDisplay.h>


#define CLK 23//19  //23 //22 //18
#define DIO 22//21 //22 // 23//4

TM1637TinyDisplay6 Screen(CLK, DIO);
//TM1637TinyDisplay Screen(CLK, DIO);

// --- UNIVERSAL INDICATORs
 const int LED = 0;
 const int LED2  = 25;//21;  //  23
 const int sound = 32;//33;

// --- ANALOG INPUTS
 const int readBattery = 34; //39;//34;  //  26;
 const int readSolar = 36;

// --- PWM OUTPUTS
  const int Coil_1 = 13; uint8_t coil_1_LED = 2; //13; // 14; //33
  const int Coil_2 = 14; uint8_t coil_2_LED = 15;
  const int fan_1 = 27;
  const int fan_2 = 26;
  const int SolarCharge = 0; //26; //14; //17-Dec-2022

// --- TOUCH INPUTS
 const int touch_1 = 5;//5;   // up 1
 const int touch_2 = 19;//23;//36; //27; //15; // down 1
 const int touch_3 = 4;//4; //13; // up 2
 const int touch_4 = 21;//22;//39; //16;// down 2

 const int reset = 35; //17; //const int powerLED = 34;
// --- LED BATTERY LEVEL INDICATORS
  const int level1 = 17;//32; //RX2
  const int level2 = 18;
  const int level3 = 16; //19; //TX2

// HIGHLY VOLATILE STATES
  bool LED1_ON = false; 
  bool LED2_ON = false;
  bool LED3_ON = false;

uint8_t power_drain_counter = 0; //memory to store for how long battery has cycled between ON & OFF
bool just_dropped = false;

void beep(uint8_t times);
// Example of manually defining a display pattern

const uint8_t ANIMATION[16][6] = {
  { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00 },  // Frame 0
  { 0x00, 0x08, 0x00, 0x00, 0x00, 0x00 },  // Frame 1
  { 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 },  // Frame 2
  { 0x00, 0x00, 0x00, 0x08, 0x00, 0x00 },  // Frame 3
  { 0x00, 0x00, 0x00, 0x00, 0x08, 0x00 },  // Frame 4
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 },  // Frame 5
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 },  // Frame 6
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },  // Frame 7
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },  // Frame 8
  { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },  // Frame 9
  { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 },  // Frame 10
  { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 },  // Frame 11
  { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 },  // Frame 12
  { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },  // Frame 13
  { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },  // Frame 14
  { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 }   // Frame 15
};


void setup() {  
//ATTACK THE COILS first --- use direct port access to lower response to nanosex 
  pinMode(Coil_1, OUTPUT);  digitalWrite(Coil_1, HIGH); //ACTIVE WHEN LOW
  pinMode(Coil_2, OUTPUT);  digitalWrite(Coil_2, HIGH); //ACTIVE WHEN LOW

  pinMode(coil_1_LED, OUTPUT); pinMode(coil_2_LED, OUTPUT);

  pinMode(fan_1, OUTPUT); pinMode(fan_2, OUTPUT); // ACTIVE HIGH


  // THEN THE DISPLAYS
Serial.begin(115200); delay(100); Serial.flush(); Serial.println("Booting...");

  pinMode(LED, OUTPUT); digitalWrite(LED, HIGH);//INDICATOR LED

//pinMode(LED2, OUTPUT);  //digitalWrite(LED2, HIGH);

uint16_t blink_counta = 0; bool is_togod = false;

  pinMode(reset, INPUT); 

bool activated = false, firstPass = false;
bool arrested = false;

int detectPress;

unsigned long readStart = 0;
/*
//OFF
while(!activated){ 

       detectPress = digitalRead(reset);
       if(detectPress) {arrested = true; digitalWrite(LED2, HIGH);}
       else {arrested = false; readStart = millis();}

    if(arrested && !firstPass){readStart = millis(); firstPass = true;}
    if(firstPass == true){

        if((millis() - readStart) > 800) activated = true; //really LONG PRESS
    }

 //Serial.print("Power ON Blink Indicator: "); Serial.println(blink_counta);
             if(blink_counta <= 300){ if(!is_togod) { digitalWrite(LED2, HIGH); is_togod = true;}}
        else if(blink_counta > 300 && blink_counta <= 800) { if(is_togod){digitalWrite(LED2, LOW); is_togod = false;} }
        else if(blink_counta > 800) {blink_counta = 0;}

 blink_counta++;
}
*/
  digitalWrite(fan_1, HIGH); digitalWrite(fan_2, HIGH);
  digitalWrite(coil_1_LED, HIGH); digitalWrite(coil_2_LED, HIGH); 

    
    pinMode(level1, OUTPUT);    digitalWrite(level1, HIGH);  LED1_ON = true; 
    pinMode(level2, OUTPUT);    digitalWrite(level2, HIGH);  LED2_ON = true; 
    pinMode(level3, OUTPUT);    digitalWrite(level3, HIGH);  LED3_ON = true; 
  

  //Screen.init();
  Screen.setBrightness(7); // SET BRIGHTNESS OF THE 7 SEGMENT

  Screen.clear();
    for(int count = 0; count < 2; count++) {
          Screen.showAnimation(ANIMATION, FRAMES(ANIMATION), TIME_MS(10));
      }
  Screen.clear();

  Screen.showString("OFF-ON");        //  --------- 1
  delay(1500);

  Screen.showNumber(1234567);  // overflow     --- 3

  delay(500);


  // All segments on
  Screen.showNumber(9, false);  //             -----9 
  Screen.showNumber(9, false, 1, 0);  //       8-----

 delay(50);
  int foo = 8; 
  while(foo>=0) { delay(50);
        Screen.showNumber(foo, false); // the right-most digit
        Screen.showNumber(foo, false, 1, 0); // the left most digit
             foo--;
     }


  

    pinMode(sound, OUTPUT); 
    uint8_t beepu = 0;
    //beep(3);
 do{ digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50); beepu++; } while(beepu < 2);
 

    delay(200);

      pinMode(SolarCharge, OUTPUT); digitalWrite(SolarCharge, HIGH);//ACTIVE HIGH
        
    //for(;;)Serial.println(digitalRead(touch_2)); //ANALOGREAD SEES THE MF* BITCH

    pinMode(touch_1, INPUT); pinMode(touch_2, INPUT);
    pinMode(touch_3, INPUT); pinMode(touch_4, INPUT);

    Serial.println("Boot Complete!"); //0n
  
  digitalWrite(fan_1, LOW); digitalWrite(fan_2, LOW); digitalWrite(SolarCharge, LOW);
  digitalWrite(coil_1_LED, LOW); digitalWrite(coil_2_LED, LOW); 


}


int currentMode = 1;
uint8_t heat_1_level = 0;
uint8_t heat_2_level = 0;

/* ONLY 4 SUPERLOOP TASKS:
 * Read battery level
 * Listen to touch interrupts
 * Update cooking coils manager
 * Shutdown when pressed or when idle (running & unloaded) for 30 minutes 
 */

void loop() { //to max out the 6 digit display:
/* 6-DIGIT 7-SEGMENT
  1. Present cooking power on either side...
  2. flash voltage reading [12.75] if need be
  3. flash countdown timer [08:27] if need be
*/
  MonitorBattery(); 
  listen_to_TouchCtrl(); 
  Monitor_cookingZones(heat_1_level, heat_2_level);
  //listen_to_reset()
   
}


  bool isPower_enough = false; bool isCharging = false;
  uint8_t Battery_Level_Counter; bool end_processes = false;
//float voltage = 0.00;

  bool recoiled = false;
  float voltage = 0.00;

  uint16_t charge_blinker = 0; // 65k
  bool toggled = false;


  bool locked_up = false;   bool locked_down = false;
  bool locked_up2 = false;  bool locked_down2 = false;


void MonitorBattery(){ //Power Manage(battery, solar)
 
/*
  * 2 VOLTAGES:
  * 1 during charging - charging voltage
  * 1 not charging - nominal voltage
*/

//Battery_Level_Counter = 0;
uint16_t reading;
uint32_t summation = 0;

  for(int i=0; i<7; i++){ //7 samples
      reading = analogRead(readBattery);
      summation += reading;
  }
    summation /= 7;

      //Serial.println();
     // Serial.print("Raw Reading: "); Serial.println(reading);
    //  Voltage = 14.50 * float(summation)/3095.0;
  
  voltage = 15.00 * float(summation)/3095.0;
  Serial.print("Voltage Reading: "); Serial.println(voltage);
  Screen.showNumber(heat_1_level, false, 1, 0);


  if(voltage < 11.15) { //too low battery power
     Battery_Level_Counter = 0;
     isPower_enough = false; Serial.println("Power Too Low!!!");
     
     if(LED1_ON) {digitalWrite(level1, LOW); LED1_ON = false;}
     if(LED2_ON) {digitalWrite(level2, LOW); LED2_ON = false;}
     if(LED3_ON) {digitalWrite(level3, LOW); LED3_ON = false;}
         
     if(!isCharging) { digitalWrite(SolarCharge, HIGH); isCharging = true; beep(3); } // ALWAYS PREVENT DEEP DISCHARGE when there is sunshining...
/*
     if(voltage <= 10.80) {//kill all RUNNING TASKS...
        end_processes = true;
      }
*/

    // the right-most cooking zone
    if(heat_1_level > 0) { heat_1_level = 0; Monitor_cookingZones(heat_1_level, heat_2_level); locked_down = true; locked_up=false; 
                           Screen.showNumber(heat_1_level, false, 1, 0); } 
                          
    // the left most cooking zone                      
    if(heat_2_level > 0) { heat_2_level = 0; Monitor_cookingZones(heat_1_level, heat_2_level); locked_down2 = true; locked_up2=false; 
                        Screen.showNumber(heat_2_level, false); Screen.showNumber(heat_1_level, false, 1, 0);}
  
 }
  
  else if(voltage >= 11.15 && voltage < 12.00) {Battery_Level_Counter = 1; isPower_enough = true; end_processes = false;
        if(!LED1_ON) {digitalWrite(level1, HIGH); LED1_ON = true; }
        if(LED2_ON)  {digitalWrite(level2, LOW); LED2_ON = false; } 
        if(LED3_ON)  {digitalWrite(level3, LOW); LED3_ON = false; }
        
        if(!isCharging) { digitalWrite(SolarCharge, HIGH); isCharging = true; beep(3); } //PWM THE CHARGING VOLTAGE
    } 
        
 else if(voltage >= 12.00 && voltage < 12.60) {Battery_Level_Counter = 2; isPower_enough = true;
      if(!LED1_ON) {digitalWrite(level1, HIGH); LED1_ON = true; } //LOAD RECONNECT SEQRNCE AT 12.8 --- CHARGING VOLTAGE
      if(!LED2_ON) {digitalWrite(level2, HIGH); LED2_ON = true; }
      if(LED3_ON)  {digitalWrite(level3, LOW);  LED3_ON = false;}
      
       if(!isCharging) { digitalWrite(SolarCharge, HIGH); isCharging = true; beep(3); }
    } //pulse width modulate the charging voltage

 else if(voltage >= 12.60) {Battery_Level_Counter = 3; isPower_enough = true;
      if(!LED3_ON) {digitalWrite(level3, HIGH); LED3_ON = true; }
      if(!LED2_ON) {digitalWrite(level2, HIGH); LED2_ON = true; }
      if(!LED1_ON) {digitalWrite(level3, HIGH); LED1_ON = true; }
      
      if(voltage >= 14.35) if(isCharging) { digitalWrite(SolarCharge, LOW); isCharging = false; beep(3);} // PREVENT OVERCHARGE
      }

//blink power button when charginh
if(isCharging){ /*
        // --- charge_blinker++; Serial.print("Charging Indicator: "); Serial.println(charge_blinker); --- //
             if(charge_blinker <= 30){ if(!toggled) { digitalWrite(LED2, HIGH); toggled = true;}}
        else if(charge_blinker > 30 && charge_blinker <= 100) { if(toggled){digitalWrite(LED2, LOW); toggled = false;} }
        else if(charge_blinker > 100) {charge_blinker = 0;}
*/
}


else {charge_blinker = 0;}

  
        
  
}


  bool Coil_1_ON = false, Coil_2_ON = false;


  uint8_t up1=0, up2=0; //const int threshold_up = 30; //for touch abilities
  uint8_t down1=0, down2=0; //const int threshod_dn = 40; // for touch abilities


  bool lock_press = false, passed_yet = false;
  unsigned long start_counting = 0;

void listen_to_TouchCtrl(){
  up1 = 0; down1 = 0;
  up2 = 0; down2 = 0;
  lock_press = false;
  
//lock max at 5 and min at 0 ... no press beyond those limits
if(!locked_up) {    up1 = digitalRead(touch_1); }
if(!locked_down){ down1 = digitalRead(touch_2); }
if(!locked_up2) {   up2 = digitalRead(touch_3); }
if(!locked_down2){down2 = digitalRead(touch_4); }


//if(digitalRead(touch_1) || digitalRead(touch_2) || digitalRead(touch_3) || digitalRead(touch_4)){ lock_press = true; } 
//universal tactile feeedback
if(up1 || up2 || down1 || down2){ lock_press = true;  } 
else start_counting = millis(); // // --- 1000; // --- to prevent overflow when statted at 4.2Bn ---//

      if(lock_press && !passed_yet) {start_counting = millis(); passed_yet = true;}
        if(passed_yet){
          if((millis() - start_counting) >= 50 && (millis() - start_counting) < 100) { beep(1);
                                                                                      //digitalWrite(LED2, HIGH);  digitalWrite(LED2, LOW);
                                                                                      touchProcessor();}
        }

}


uint8_t CoilNum = 0; uint8_t whichCoil = 0;

bool reducing = false;
//Power Mgt Activated: ONCLICK ---- basic Power Saving mode
void touchProcessor(){ // LEVEL SHIFTER WITH SEMI-DYNAMIC POWER MANAGEMENT INTEGRATED
   if(Battery_Level_Counter < 1) { //only react to LEVEL LOWERING when in ultra low power mode
    
        if(up1 || up2) beep(2); //Screen.display(1, 'n'); Screen.display(2, 'o'); // prevent deep discharge

        if(down1 == 1) {  
              if(heat_1_level > 0) {locked_down=false; locked_up=false; heat_1_level--; } 
               else {heat_1_level = 0; locked_down = true; locked_up=false; } 
                    Screen.showNumber(heat_1_level, false, 1, 0); reducing = true; }
        
        if(down2 == 1) { 
              if(heat_2_level > 0) {locked_down2=false; locked_up2=false; heat_2_level--; } 
               else {heat_2_level = 0; locked_down2 = true; locked_up2=false; } 
                     Screen.showNumber(heat_2_level, false);  Screen.showNumber(heat_1_level, false, 1, 0); }

        } //if in shifting gear but power is discovered to be low
   
   else { //battery level counter == 1, 2, 3 ... > 11.2V
    if(up1 == 1) {  
                    if(heat_1_level < 5) { locked_up=false; locked_down=false; //free_run
                        if(Battery_Level_Counter == 3 && heat_1_level < 5)  heat_1_level++;  //maxes out at 5:: FULL BLAST
                        if(Battery_Level_Counter == 2 && heat_1_level < 4) {heat_1_level++; if(heat_1_level == 4) locked_up = true;} //maxes out at 4
                        if(Battery_Level_Counter == 1 && heat_1_level < 3) {heat_1_level++; if(heat_1_level == 3) locked_up = true;} //maxes out at 3
                        } 

                  else { locked_up = true; locked_down=false;
                        if(Battery_Level_Counter == 1) heat_1_level = 3;  
                        if(Battery_Level_Counter == 2) heat_1_level = 4;  
                        if(Battery_Level_Counter == 3) heat_1_level = 5;       
                        }
                 Screen.showNumber(heat_1_level, false, 1, 0); CoilNum = 1; reducing = false; 
               }

if(down1 == 1) {  if(heat_1_level > 0) {locked_down=false; locked_up=false; heat_1_level--; } 
               else {heat_1_level = 0; locked_down = true; locked_up=false; } 
                   Screen.showNumber(heat_1_level, false, 1, 0); reducing = true; }
  
if(up2 == 1) {  if(heat_2_level < 5) {locked_up2=false; locked_down2=false; //free_run
                        if(Battery_Level_Counter == 3 && heat_2_level < 5) heat_2_level++;  //maxes out at 5:: FULL BLAST
                        if(Battery_Level_Counter == 2 && heat_2_level < 4) {heat_2_level++; if(heat_2_level == 4) locked_up2 = true;}  //maxes out at 4
                        if(Battery_Level_Counter == 1 && heat_2_level < 3) {heat_2_level++; if(heat_2_level == 3) locked_up2 = true;}//maxes out at 3
                         } 
               
                else {locked_up2 = true; locked_down2=false;
                        if(Battery_Level_Counter == 1) heat_2_level = 3;  
                        if(Battery_Level_Counter == 2) heat_2_level = 4;  
                        if(Battery_Level_Counter == 3) heat_2_level = 5;   } 
                    
                    Screen.showNumber(heat_2_level, false); Screen.showNumber(heat_1_level, false, 1, 0); CoilNum = 2;  }
  
if(down2 == 1) { if(heat_2_level > 0) {locked_down2=false; locked_up2=false; heat_2_level--; } 
               else {heat_2_level = 0; locked_down2 = true; locked_up2=false; } 
                     Screen.showNumber(heat_2_level, false); Screen.showNumber(heat_1_level, false, 1, 0); }

   }

      Serial.println();
      Serial.print("Coil 1 Level: "); Serial.println(heat_1_level);
      Serial.print("Coil 2 Level: "); Serial.println(heat_2_level);
      Serial.println();
      //vTask.execute();

        //return 0;
}




unsigned long now = 0;
unsigned long idle = 0;


long interval = 0; long interval2 = 0;
unsigned long stat = 0, stat2 = 0; 

bool waiting = false;
bool waiting2 = false;

const   long wait_ko = 30000; // 30 sec constant Cooling time \\ 300,000

const   long interval_1 = 600000; //  10 minutes
const   long interval_2 = 900000; // 15 minutes
const   long interval_3 = 1200000; // 20 minutes
const   long interval_4 = 1800000; // 30 minutes
const   long interval_5 = 2700000; // 45 minutes
const   long interval_6 = 3600000; // 60 minutes


void Monitor_cookingZones(uint8_t coil_1_index, uint8_t coil_2_index){ //COIL INDEX IS THE MASTER CONTROLLER 

if(stat > 4294960000) {stat = 0; return; } // prevent superoverflow for last minute ...4,294,967,295
if(stat2> 4294960000) {stat2 = 0; return; } // prevent superoverflow for last minute


if(Battery_Level_Counter < 1) { //if POWER IS ALARMINGLY LOW < 11.4V auto stop cooking
      if(coil_1_index == 0) { Coil_1_ON = false; Serial.println("COIL 1 Turned OFF DUE TO POWER"); 
                      
                      if(!Coil_1_ON) {
                        digitalWrite(fan_1, LOW); waiting = false;
                        digitalWrite(Coil_1, HIGH); Coil_1_ON = false; }
                        stat = 0; }

      if(coil_2_index == 0) { Coil_2_ON = false; Serial.println("COIL 2 Turned OFF due to POWER"); 
                          digitalWrite(fan_2, LOW);  waiting2 = false;
                          digitalWrite(Coil_2, HIGH); Coil_2_ON = false;
                          stat2 = 0;  }
/*
        if(waiting)  {digitalWrite(fan_1, LOW); waiting  = false;}
        if(waiting2) {digitalWrite(fan_2, LOW); waiting2 = false;}
*/
  }

else { // if coil_1_index == 1, 2, 3, 4, 5... 
  //kozesa coil 1 index otambuze coil 1 state ... and likewise 2 4 2
      if(coil_1_index == 0){  //switch OFF // if either we haven't started or they have reduced all the way to zero
        if(waiting) {digitalWrite(fan_1, LOW); waiting = false;}
        else {
              if(Coil_1_ON) { digitalWrite(coil_1_LED, LOW);
                              digitalWrite(fan_1, LOW);  waiting = false; 
                              digitalWrite(Coil_1, HIGH); Coil_1_ON = false;
                              stat = 0; /* Serial.println("Coil 1 turned OFF!");*/}// 
         }
          
      } 
//close if power is set to zero

else if(coil_1_index != 0) { //if toggled to a non zero integer
     if(Coil_1_ON) { 
     //   Serial.print("Coil "); Serial.print(CoilNum); Serial.print(" running for: "); 
    //    Serial.println((1000 + stat + interval - millis())/1000);
       }
 else if(!Coil_1_ON) { //check if it hasn't alreay been turned ON
            if(!waiting) { //turn it ON if it is not in waiting state
                  digitalWrite(fan_1, HIGH); digitalWrite(coil_1_LED, HIGH);
                  digitalWrite(Coil_1, LOW); Coil_1_ON = true; stat = millis(); 
               //   Serial.print("Coil "); Serial.print(CoilNum); Serial.println("FIRED !!!"); 
              //    Serial.print("At: "); Serial.println(stat/1000);
                  }
              else if(waiting){
                  //Serial.print("\tWaiting for ... "); Serial.println(((1000 + stat + interval + wait_ko) - millis())/1000);
              }
      }

        switch(coil_1_index){ 
            case 1:  interval = interval_1; running(interval);   
                  break;

            case 2: interval = interval_2; running(interval);   
                    break;

            case 3: interval = interval_3; running(interval);    
                    break;

            case 4: interval = interval_4; running(interval);   
                    break;
            
            case 5: interval = interval_5; running(interval);     
                    break;
          
          }

  }//closing if not zero
  
if(coil_2_index == 0){ //switch OFF // Serial.println("Coil 2 OFF!"); // if they have reduced to zero
   if(Coil_2_ON) {digitalWrite(coil_2_LED, LOW);
                  digitalWrite(fan_2, LOW); waiting2 = false;
                  digitalWrite(Coil_2, HIGH); Coil_2_ON = false; 
                  stat2 = 0;/*/ Serial.println("Turned OFF"); */}// 
    if(waiting2) {digitalWrite(fan_2, LOW); waiting2 = false;}
    }
//close if zero

else if(coil_2_index != 0) {  //if toggled to a non zero integer
     if(Coil_2_ON) { 
    //    Serial.print("Coil "); Serial.print(CoilNum); Serial.print(" running for: "); 
   //     Serial.println((1000 + stat2 + interval2 - millis())/1000);
       }
 else if(!Coil_2_ON) { //check if it hasn't alreay been turned ON
      if(!waiting2) { //turn it ON if it is not in waiting state
          digitalWrite(fan_2, HIGH); digitalWrite(coil_2_LED, HIGH);
          digitalWrite(Coil_2, LOW); Coil_2_ON = true; stat2 = millis();
            // Serial.print("Coil "); Serial.print(CoilNum); Serial.println("FIRED !!!"); 
            // Serial.print("At: "); Serial.println(stat2/1000);
            }
        else if(waiting2){
            //Serial.print("\tWaiting for ... "); Serial.println(((1000 + stat2 + interval2 + wait_ko) - millis())/1000);
        }
      }


  switch(coil_2_index){ 
      case 1: interval2 = interval_1; running(interval2); 
            break;

      case 2: interval2 = interval_2; running(interval2);
              break;

      case 3: interval2 = interval_3; running(interval2); 
              break;

      case 4: interval2 = interval_4; running(interval2); 
              break;
      
      case 5: interval2 = interval_5; running(interval2);   
              break;
     
      }

    }//closing if coil 2 not zero

 }  //close if power is enough

//close cooking monitor  
}

void running(long intervo){ // uint8_t COIL_NO

   // Serial.print("Running Time: "); Serial.println(intervo); 

              if((millis() - stat) < interval) { //Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON");
                    /*Serial.print(" Firing till ");  Serial.println((1+interval + stat)/1000);*/  /*if(reducing) waiting = true;*/
                  }
              else { // if has been ON for more than first interval
                  if((millis() - stat) < (interval + wait_ko)){ // --- TURN OFF & induce internal FORCED COOLING
                        if(Coil_1_ON) { waiting = true; //Turn off COIL to COOL
                              digitalWrite(Coil_1, HIGH); digitalWrite(coil_1_LED, LOW);/*digitalWrite(fan_1, LOW);*/ Coil_1_ON = false;
                          }
                          else { /*Serial.print("Waiting Till "); Serial.println((interval + stat + wait_ko)/1000); */ }
                    }
                    else { waiting = false; /*Serial.println("Waiting Time Over - !"); */} // --- WAITING TIME OVER 
               }

    

              if(millis() - stat2 < interval2) { /*Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON"); */}
                
              else { // if has been ON for more than first interval
                  if(millis() - stat2 < (interval2 + wait_ko)){ // --- TURN OFF & induce internal FORCED COOLING
                     if(Coil_2_ON) { waiting2 = true; //Turn off COIL to COOL
                        digitalWrite(Coil_2, HIGH); /*digitalWrite(fan_2, LOW);*/ Coil_2_ON = false;
                       }
                       else {/*Serial.print("Waiting Till "); Serial.println((interval2 + stat2 + wait_ko)/1000);*/ }
                    }
                    else { waiting2 = false; /*Serial.println("Waiting Time Over - !");*/ } // --- WAITING TIME OVER 
        
    }

}

void quit_cooking(){ }//turn off the correspoinding cooking zones and diable timers

unsigned long start_beeping = 0;
unsigned long sounding_duration; 

bool sound_started = false;
bool sound_locked = false; 

void beep(uint8_t times){
/*
  for(int xy=0; xy<times; xy++){

      if(!sound_locked) { start_beeping = millis();   sound_locked = true;  }

      if(sound_locked && !sound_started){
          digitalWrite(sound, HIGH);
          sound_started = true;
      }

      if((millis() - start_beeping) > 50){
        digitalWrite(sound, LOW);
        sound_started = false;
      }

  }
*/
      for(int xy=0; xy<times; xy++){
        digitalWrite(sound, HIGH);
        
        if(Battery_Level_Counter == 0) delay(100);
        else delay(50);
        
        digitalWrite(sound, LOW);
        if(times > 1) delay(50);
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
