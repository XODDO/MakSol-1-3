/* 15th APRIL 2023 EDITION ~650lines
  --- XODDOCODE 5.0 Design Pattern ---
  * VERY MINIMAL DELAY functions - 2 implementations: only in sound 
  * INTELLIGENT POWER MGT => Low Voltage AUTO Load disconnect, Charging indication & Over charge prevention
  * FAN SLIGHTLY OUT OF PHASE WITH PWM SPEED CONTROL
  * 4-KEY NAVIGATION
  * HIGH PRECISION SCHEDULING => superfast (millis()) time allocation
  * BASED ON LARGE 6-DIGIT TM1637 7seg DISP, uses pnly 2 digits
*/
#include "Arduino.h"
#include <TM1637TinyDisplay6.h>


const int CLK = 23;//19  //23 //22 //18
const int DIO = 22;//21 //22 // 23//4

TM1637TinyDisplay6 Screen(CLK, DIO);

// --- UNIVERSAL INDICATORs
   const uint8_t ESP_ON = 2;
   const uint8_t flashin_LED = 12;
   const uint8_t sound = 32;//33;

// --- POWER
    const uint8_t BatteryPin = 34; //39;//34;  //  26;
    const uint8_t readSolar = 36;
    const uint8_t charge_by_Solar = 13; //26; //14; //17-Dec-2022


// --- PWM OUTPUTS
    const uint8_t Coil_1 = 26;//14; //26;  //13; // 14; //33
    const uint8_t Coil_2 = 14;//26;  
    const uint8_t fan_1 = 25;//33;
    const uint8_t fan_2 = 33;//25;

    const uint8_t channel_1 = 0, channel_2 = 1;
    const uint8_t duty_cycle = 8; // 1 - 16 bits
    const uint16_t pwm_frequency = 12000; // 5kHz up to 30kHz...

    const uint8_t Speed_0 = 0;
    const uint8_t Speed_1 = 20;
    const uint8_t Speed_2 = 32;
    const uint8_t Speed_3 = 90;
    const uint8_t Speed_4 = 255;


   //uint8_t coil_1_LED = 2; uint8_t coil_2_LED = 15;
  //these are backlooped direct to the power board
 //  
// --- TOUCH INPUTS
    const uint8_t touch_1 = 4;//5;   // up 1
    const uint8_t touch_2 = 21;//19;//23;//36; //27; //15; // down 1
    const uint8_t touch_3 = 5;//4; //13; // up 2
    const uint8_t touch_4 = 19;//21;//22;//39; //16;// down 2

    const uint8_t reset = 0; //17; //const int powerLED = 34;
// --- LED BATTERY LEVEL INDICATORS
    const uint8_t level0 = 15;
    const uint8_t level1 = 17;//32; //RX2
    const uint8_t level2 = 18;
    const uint8_t level3 = 16; //19; //TX2

// HIGHLY VOLATILE STATES
    bool LED0_ON = false;
    bool LED1_ON = false; 
    bool LED2_ON = false;
    bool LED3_ON = false;

uint8_t power_drain_counter = 0; //memory to store for how long battery has cycled between ON & OFF
bool just_dropped = false;

void beep(uint8_t times);

// Example of manually defining a display pattern
//  ------
// |      |
//  ------
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

  // THEN THE DISPLAYS
  Serial.begin(115200); delay(100); Serial.flush(); Serial.println("Booting...");

  pinMode(flashin_LED, OUTPUT); digitalWrite(flashin_LED, HIGH);//INDICATOR LED

  pinMode(charge_by_Solar, OUTPUT);  digitalWrite(charge_by_Solar, HIGH);//ACTIVE HIGH

  //Screen.init();
  Screen.setBrightness(7); // SET BRIGHTNESS OF THE 7 SEGMENT
  Screen.clear();

  Screen.showString("OFF-ON");  delay(400);      //  --------- 1
  Screen.clear();

    for(int count = 0; count < 1; count++) {
          Screen.showAnimation(ANIMATION, FRAMES(ANIMATION), TIME_MS(10));
      }

  Screen.clear();
  Screen.showNumber(1234567);  delay(100); // overflow     --- 2

/*
// d ACTIVE HIGHz
    pinMode(fan_1, OUTPUT);   digitalWrite(fan_1, HIGH); delay(50);
    pinMode(fan_2, OUTPUT);   digitalWrite(fan_2, HIGH); delay(50);

    */
//  SPEED CONTROLLED FANS
    ledcAttachPin(fan_1, channel_1);    
    ledcSetup(channel_1, pwm_frequency, duty_cycle);
    
    ledcAttachPin(fan_2, channel_2);
    ledcSetup(channel_2, pwm_frequency, duty_cycle);

    ledcWrite(channel_1, Speed_4); ledcWrite(channel_2, Speed_4);


    pinMode(level0, OUTPUT);    digitalWrite(level0, LOW);   LED0_ON = false;
    pinMode(level1, OUTPUT);    digitalWrite(level1, HIGH);  LED1_ON = true; 
    pinMode(level2, OUTPUT);    digitalWrite(level2, HIGH);  LED2_ON = true; 
    pinMode(level3, OUTPUT);    digitalWrite(level3, HIGH);  LED3_ON = true; 
  


  Screen.clear();
  // All segments on
  Screen.showNumber(9, false);  //             -----9 
  Screen.showNumber(9, false, 1, 0);  //       9-----

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
 do{ digitalWrite(sound, HIGH); digitalWrite(flashin_LED, LOW); delay(50); digitalWrite(sound, LOW); digitalWrite(flashin_LED, HIGH); delay(50); beepu++; } while(beepu < 2);


    delay(100);

        
    pinMode(touch_1, INPUT); pinMode(touch_2, INPUT);
    pinMode(touch_3, INPUT); pinMode(touch_4, INPUT);

  
    ledcWrite(channel_1, Speed_0); ledcWrite(channel_2, Speed_0);

    digitalWrite(charge_by_Solar, LOW);
  
   pinMode(ESP_ON, OUTPUT); digitalWrite(ESP_ON, HIGH); // TURN DEM ON

  //   REG_WRITE (GPIO_ENABLE_REG, BIT2); //Define GPIO2 as output
 //    REG_WRITE (GPIO_OUT_W1TS_REG, BIT2); //GPIO2 HIGH (set)


    Serial.println("Boot Complete!"); //0n
    

}


uint8_t currentMode = 1;
uint8_t heat_1_level = 0;
uint8_t heat_2_level = 0;

/* ONLY 4 SUPERLOOP TASKS:
 * Read battery level
 * Listen to touch interrupts
 * Update cooking coils manager
 * Shutdown when pressed or when idle (running & unloaded) for 30 minutes 
 */
unsigned int batt_runs = 1;

void loop() { //to max out the 6 digit display:
/* 6-DIGIT 7-SEGMENT
  1. Present cooking power on either side...
  2. flash voltage reading [12.75] if need be
  3. flash countdown timer [08:27] if need be
*/
  if(batt_runs%10 == 0) MonitorBattery(); 
  listen_to_TouchCtrl(); 
  Monitor_cookingZones(heat_1_level, heat_2_level);
  //listen_to_timer_set()
   batt_runs++;
}


  bool isPower_enough = false; bool isCharging = false;
  uint8_t Battery_Level_Counter; 
//float voltage = 0.00;

  bool is_critical = false; bool processes_ended = false;
  float voltage = 0.00;

  uint16_t charge_blinker = 0; // 65k
  bool toggled = false;


  bool locked_up = false;   bool locked_down = false;
  bool locked_up2 = false;  bool locked_down2 = false;

  bool volt_disp_invoked = false; bool screen_cleared = false;

void MonitorBattery(){ //Power Manage(battery, solar)
 
/*
  * 2 VOLTAGES:
  * 1 during charging - charging voltage
  * 1 not charging - nominal voltage
  * when heavily loaded
*/

//Battery_Level_Counter = 0;
uint16_t readBattery = 0;
float summation = 0.00;

  for(int i=0; i<10; i++){ //7 samples
      readBattery = analogRead(BatteryPin);
      summation += float(readBattery);
  }
    summation /= 10.0;
    //voltage = 15.00 * (summation/3095.0);
    //voltage = 14.50 * (summation/3095.0);
      voltage = 14.868 * (summation/3095.0);

      //also make the reading less jumpy

// PRINT READINGS //
      //Serial.print("Raw Reading: "); Serial.println(readBattery);
    //  Serial.print("Voltage: "); Serial.println(voltage, 3);
      
/*
  char voltage_string[7];   //  dtostrf(voltage, -5, 2, voltage_string);
  dtostrf((voltage), -4, 3, voltage_string);
  Serial.print("Voltage String 1: "); Serial.println(voltage_string);
*/


  if(voltage < 10.20) { //too low battery power
     Battery_Level_Counter = 0;
  //   isPower_enough = false; 
       if(!is_critical) {Serial.println("\t\tPower Too Low!!!"); is_critical = true; }
       //Screen.clear();
     
     if(!LED0_ON){  digitalWrite(level0, HIGH);LED0_ON = true;  }
     if(LED1_ON) {  digitalWrite(level1, LOW); LED1_ON = false; }
     if(LED2_ON) {  digitalWrite(level2, LOW); LED2_ON = false; }
     if(LED3_ON) {  digitalWrite(level3, LOW); LED3_ON = false; }
         
     if(!isCharging) { digitalWrite(charge_by_Solar, HIGH); isCharging = true; beep(1); } // ALWAYS PREVENT DEEP DISCHARGE when there is sunshining...
  
  
     if(heat_1_level > 0) { // display for the left most cooking zone                      
                            for(int index = heat_1_level; index>=0; index--){ Screen.showNumber(index, false, 1, 0);  delay(100); }
                            heat_1_level = 0;  
                            
                            locked_down = true; locked_up=false; 
                            Serial.println("COIL 1 Turned OFF by Power Manager due to undervoltage"); 
                            Serial.println("COIL 1 Turned OFF DUE TO POWER"); 
                            
                          }
      if(heat_2_level > 0) { // display for the left most cooking zone                      
                            for(int index = heat_2_level; index>=0; index--){ Screen.showNumber(index, false);  Screen.showNumber(heat_1_level, false, 1, 0); delay(50); }
                            heat_2_level = 0;
                            //Monitor_cookingZones(heat_1_level, heat_2_level); //since it is auto-summoned by calling function
                            
                            locked_down2 = true; locked_up2=false; 
                            Serial.println("COIL 2 Turned OFF by Power Manager due to undervoltage"); 
                            
                          }
    //    if(heat_1_level == 0 || heat_2_level == 0) {Screen.showNumber(heat_2_level, false); Screen.showNumber(heat_1_level, false, 1, 0); }

  } 

else{ //if voltage is above 10.2 atleast
        if(is_critical) {Serial.println("\t\tPower Sufficient!!!"); is_critical = false; }

if(volt_disp_invoked){

//split the voltage: 12.868 into 12 & 86
int part_1 = int(voltage); // 12.868 = 12
int part_2 = int(10*(voltage - int(voltage+0.05))); // 12.868 - 12 = (0.868+.05 x 10) = 9.1 = 9
int part_3 = int(100*(voltage - int(voltage))); // 12.868 - 12 = 0.868 x 100 = 86

   // int workin_volt = voltage*100; char voltage_str2[5]; //12.86 => 1286
      int workin_volt = (part_1*10) + part_2; char voltage_str2[5]; //12.86 => 1286
      itoa(workin_volt, voltage_str2, 10); 

        Serial.print("Actual Voltage: "); Serial.println(voltage, 3);
  //    Serial.print("Rounded Voltage: "); Serial.println(workin_volt);
  //    Serial.print("Voltage String 2: "); Serial.println(voltage_str2);
        Screen.showString(voltage_str2, 3, 1, 0b01000000); // (float, int dots, bool leading zeroz, int length, int position)
      
}

else {
      if(!screen_cleared) { Screen.showNumber(heat_2_level, false);  Screen.showNumber(heat_1_level, false, 1, 0);  screen_cleared = true;}
}

  //LED indicators --- battery bars
  if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
  if(!LED1_ON) {  digitalWrite(level1, HIGH); LED1_ON = true; }//ON THE LOWER MOST BAR [common denominator]

//between 10.9V to 11.8V
    if(voltage >= 10.20 && voltage < 11.80) {Battery_Level_Counter = 1; isPower_enough = true; processes_ended = false;
        if(LED2_ON)  {  digitalWrite(level2, LOW); LED2_ON = false; } 
        if(LED3_ON)  {  digitalWrite(level3, LOW); LED3_ON = false; }
        
        if(!isCharging) { digitalWrite(charge_by_Solar, HIGH); isCharging = true; beep(1); } //PWM THE CHARGING VOLTAGE
    } 
        
 else if(voltage >= 11.80 && voltage < 12.55) {Battery_Level_Counter = 2; isPower_enough = true;
      if(!LED2_ON) {  digitalWrite(level2, HIGH); LED2_ON = true; }
      if(LED3_ON)  {  digitalWrite(level3, LOW);  LED3_ON = false;}
      
       if(!isCharging) { digitalWrite(charge_by_Solar, HIGH); isCharging = true; beep(1); }
    } //pulse width modulate the charging voltage

 else if(voltage >= 12.55) {Battery_Level_Counter = 3; isPower_enough = true;
      if(!LED3_ON) {  digitalWrite(level3, HIGH); LED3_ON = true; }
      if(!LED2_ON) {  digitalWrite(level2, HIGH); LED2_ON = true; }
      
      if(voltage >= 14.38) if(isCharging) { digitalWrite(charge_by_Solar, LOW); isCharging = false; beep(3); } // PREVENT OVERCHARGE
    }

}

//blink power button when charginh
if(isCharging){ 
        // --- charge_blinker++; Serial.print("Charging Indicator: "); Serial.println(charge_blinker); --- //
             if(charge_blinker <= 30){ if(!toggled) { digitalWrite(flashin_LED, HIGH); toggled = true;}}
        else if(charge_blinker > 30 && charge_blinker <= 100) { if(toggled){digitalWrite(flashin_LED, LOW); toggled = false;} }
        else if(charge_blinker > 100) {charge_blinker = 0;}

}


else {charge_blinker = 0;}

       
}


  bool Coil_1_ON = false, Coil_2_ON = false;


  uint8_t up1 = 0, up2 = 0; //const int threshold_up = 30; //for touch abilities
  uint8_t down1 = 0, down2 = 0; //const int threshod_dn = 40; // for touch abilities


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
          if((millis() - start_counting) >= 50 && (millis() - start_counting) < 100) { digitalWrite(flashin_LED, HIGH);
                                                                                       beep(1);
                                                                                       digitalWrite(flashin_LED, LOW);
                                                                                       touchProcessor();
                                                                                   }
         /*EXTRA SETTINGS MODE*/    //e.g. TIMER, VOLTAGE DISP, 
          if((millis() - start_counting) >= 1200 && (millis() - start_counting) < 1250) { beep(3); volt_disp_invoked = !volt_disp_invoked; screen_cleared = false;}  

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
    if(up1 == 1 && down1 == 0) {  
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

if(down1 == 1 && up1 == 0) {  if(heat_1_level > 0) {locked_down=false; locked_up=false; heat_1_level--; } 
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
bool fan1_ON = false; bool fan2_ON = false;
unsigned long fan_1_sat  = 0, fan_2_sat  = 0;
unsigned long fan_1_stop = 0, fan_2_stop = 0;

bool fan1_speed_2_engaged = false,  fan1_speed_4_engaged = false;
bool fan2_speed_2_engaged = false,  fan2_speed_4_engaged = false;

bool fan_1_modulated = false;

//revised intervals:: HALVED
const   long interval_1 = 300000; //  5 minutes
const   long interval_2 = 600000; // 10 minutes
const   long interval_3 = 900000; // 15 minutes
const   long interval_4 = 1200000; // 20 minutes
const   long interval_5 = 1500000; // 25 minutes
const   long interval_6 = 1800000; // 30 minutes

unsigned long runtime_tracker = 0;

void Monitor_cookingZones(uint8_t coil_1_index, uint8_t coil_2_index){ //COIL INDEX IS THE MASTER CONTROLLER 

if(stat  > 4294960000) {stat  = 0; return; } // prevent superoverflow for last minute ...4,294,967,295
if(stat2 > 4294960000) {stat2 = 0; return; } // prevent superoverflow for last minute

 //WHENEVER POWER IS ALARMINGLY LOW < 11.4V auto stop cooking
      if(coil_1_index == 0) { 
                    if(Coil_1_ON) {
                        digitalWrite(Coil_1, HIGH); Coil_1_ON = false; 
                        stat = 0;
                        fan_1_stop = millis();
                      }
                    if(fan1_ON){ //whether it was in the wait or NOT, take it OFF after 5 seconds
                          if(millis() - fan_1_stop >= 4000) { // forced cooling at shutdown
                                ledcWrite(channel_1, Speed_0);
                                 waiting = false; fan1_ON = false;
                                fan_1_stop = 0; 
                                Serial.println("Fan 1 OFF!");
                                fan1_speed_2_engaged = false; fan1_speed_4_engaged = false;
                            }
                        }
                  }

      if(coil_2_index == 0) { 
                        if(Coil_2_ON) {
                            digitalWrite(Coil_2, HIGH); Coil_2_ON = false;
                            stat2 = 0;  
                            fan_2_stop = millis();
                        }
                      if(fan2_ON){ //whether it was in the wait or NOT, take it OFF after 5 seconds
                          if(millis() - fan_2_stop >= 4000) { // forced cooling at shutdown
                                ledcWrite(channel_2, Speed_0);  
                                waiting2 = false; fan2_ON = false;
                                fan_2_stop = 0;
                                Serial.println("Fan 2 OFF!");
                           }
                     }
                    
               }

//close if zero


//close if power is set to zero

   if(coil_1_index != 0) { //if toggled to 1, 2, 3, 4, 5
         if(Coil_1_ON) { /*
               runtime_tracker = (1000 + stat + interval - millis());
            long run_time_mins = runtime_tracker/60000; long run_time_secs = (runtime_tracker%60000);
    
            Serial.print("Coil "); Serial.print(CoilNum); Serial.print(" running for: "); 
            
            Serial.print(run_time_mins); Serial.print(":");  Serial.print(run_time_secs);Serial.print(" Minutes");
            
            Serial.print(" (Total: "); Serial.print(runtime_tracker/1000); Serial.println(" Seconds)"); Serial.println();
    */
            if(!waiting){//turn it on instantly but very silently... for the first 30 seconds
                          if(!fan1_ON) { ledcWrite(channel_1, Speed_1); fan1_ON = true; Serial.print("Fan 1 Speed: "); Serial.println(Speed_1); fan1_speed_2_engaged = false; fan1_speed_4_engaged = false; }
                          if(fan1_ON){
                            if(((millis() - stat) >= 20000) && (millis() - stat) < 30000){ //if(!fan1_speed_2_engaged) {
                                ledcWrite(channel_1, Speed_2); Serial.print("Fan 1 Speed: "); Serial.println(Speed_2); fan1_speed_2_engaged = true; }
                            if(((millis() - stat) >= 30000) && (millis() - stat) < 60000){ //if(!fan1_speed_4_engaged){
                              ledcWrite(channel_1, Speed_4); Serial.print("Fan 1 Speed: "); Serial.println(Speed_4); fan1_speed_4_engaged = true; } //&& to avoid repititions
                            
                          }
                        }
       }
 else if(!Coil_1_ON) { //check if it hasn't alreay been turned ON
            if(!waiting) { //turn it ON if it is not in waiting state

                  digitalWrite(Coil_1, LOW); Coil_1_ON = true; stat = millis(); 
                  
                  Serial.print("Coil "); Serial.print(CoilNum); Serial.println("FIRED !!!"); 
                  Serial.print("At: "); Serial.println(stat/1000);
                  }
              else if(waiting){
                  Serial.print("\tWaiting for ... "); Serial.print(((1000 + stat + interval + wait_ko) - millis())/1000); Serial.println(" seconds");
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

  }//closing if coil 1 is not zero
  

if(coil_2_index != 0) {  //if toggled to a non zero integer
     
      if(Coil_2_ON) { //check if it hasn't alreay been turned ON
    
            if(!waiting2){//turn it on instantly but very silently... for the first FULL minute
                          if(!fan2_ON) { ledcWrite(channel_2, Speed_1); fan2_ON = true; Serial.print("Fan 2 Speed: "); Serial.println(Speed_1); }
                          if(fan2_ON){
                            if(((millis() - stat2) >= 20000) && (millis() - stat2) < 30000){ ledcWrite(channel_2, Speed_2); Serial.print("Fan 2 Speed: "); Serial.println(Speed_2);}
                            if(((millis() - stat2) >= 30000) && (millis() - stat2) < 40000){ ledcWrite(channel_2, Speed_3); Serial.print("Fan 2 Speed: "); Serial.println(Speed_3);} 
                            
                            if(((millis() - stat2) >= 40000) && (millis() - stat2) < 240000){ ledcWrite(channel_2, Speed_4); Serial.print("Fan 2 Speed: "); Serial.println(Speed_4);} 
                
                            if(((millis() - stat2) >= 240000) && (millis() - stat2) < 250000){ ledcWrite(channel_2, Speed_2); Serial.print("Fan 2 Speed: "); Serial.println(Speed_2);} 
                            if(((millis() - stat2) >= 250000) && (millis() - stat2) < 270000){ ledcWrite(channel_2, Speed_3); Serial.print("Fan 2 Speed: "); Serial.println(Speed_3);} 
                            
                            if(((millis() - stat2) >= 270000) && (millis() - stat2) < 271000){ ledcWrite(channel_2, Speed_4); Serial.print("Fan 2 Speed: "); Serial.println(Speed_4);} //&& to avoid repititions
                            
                          }
                        }
              else if(waiting){
                
              }

            
          } //if coil 2 is already on and running...
          if(!Coil_2_ON) { 

            if(waiting2){ //thrown into WAIT LOOP
                  Serial.print("\tThrown into wait loop ... "); Serial.print(((1000 + stat + interval + wait_ko) - millis())/1000); Serial.println(" seconds");
              }
          
  
            else if(!waiting) { //turn it ON if it is not in waiting state

                  digitalWrite(Coil_2, LOW); Coil_2_ON = true; stat2 = millis(); 
                  
                  Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" FIRED !!!"); 
                  Serial.print("At: "); Serial.println(stat/1000);
                  }
              
          }
 

//


//
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

 //}  //close if power is enough

//close cooking monitor  
}

bool is_togod = false;
uint16_t blink_counta = 0; // RUNTIME INDICATOR...

void running(long intervo){ // uint8_t COIL_NO

   // Serial.print("Running Time: "); Serial.println(intervo); 

              if((millis() - stat) < interval) { //Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON");
                    /*Serial.print(" Firing till ");  Serial.println((1+interval + stat)/1000);*/  /*if(reducing) waiting = true;*/
                           if(blink_counta <= 300){ if(!is_togod) { digitalWrite(flashin_LED, HIGH); is_togod = true;}}
                      else if(blink_counta > 300 && blink_counta <= 500) { if(is_togod){digitalWrite(flashin_LED, LOW); is_togod = false;} }
                      else if(blink_counta > 550) {blink_counta = 0;}

                        blink_counta++;
                                
                  }
              else { // if has been ON for more than first interval
                  if((millis() - stat) < (interval + wait_ko)){ // --- TURN OFF & induce internal FORCED COOLING
                        if(Coil_1_ON) { waiting = true; //Turn off COIL to COOL
                              digitalWrite(Coil_1, HIGH); Coil_1_ON = false;
                          }
                          else { /*Serial.print("Waiting Till "); Serial.println((interval + stat + wait_ko)/1000); */ }
                    }
                    else { waiting = false; /*Serial.println("Waiting Time Over - !"); */} // --- WAITING TIME OVER 
               }

    

              if(millis() - stat2 < interval2) { /*Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON"); */}
                
              else { // if has been ON for more than first interval
                  if(millis() - stat2 < (interval2 + wait_ko)){ // --- TURN OFF & induce internal FORCED COOLING
                     if(Coil_2_ON) { waiting2 = true; //Turn off COIL to COOL
                        digitalWrite(Coil_2, HIGH); Coil_2_ON = false;
                       }
                       else {Serial.print("Waiting Till "); Serial.println((interval2 + stat2 + wait_ko)/1000); }
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
