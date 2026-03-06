/* 16th SEPT 2023 EDITION ~900 lines: ~50% more from 1,040lines
  --- XODDOCODE 5.0 Design Pattern ---
  * VERY MINIMAL DELAY functions - 2 implementations: only in sound 
  * INTELLIGENT POWER MGT => Low Voltage AUTO Load disconnect, Charging indication & Over charge prevention
  * FAN SLIGHTLY OUT OF PHASE WITH PWM SPEED CONTROL
  * 
  * CPU speed reduced to 40MHz to reduce power consumption
  * configuration: "PSRAM=disabled,PartitionScheme=no_ota,CPUFreq=40,FlashMode=qio,FlashFreq=80,FlashSize=4M,UploadSpeed=921600,LoopCore=1,EventsCore=1,DebugLevel=none,EraseFlash=none"

  * HIGH PRECISION SCHEDULING => int64_t esp_timer_get_time(void);
  * SINCE IT OVERFLOWS AT 200k YEARS
  * BATTERY READ FREQUENCY LIMITED TO ABOUT ONCE EVERY SECOND
  * DATA & PROCESS PRESENTATION BASED ON LARGE 6-DIGIT TM1637 7seg DISP, uses pnly 2 digits
*/
#include "Arduino.h"
#include <TM1637TinyDisplay6.h>


// === #include "FS.h"
// === #include <LITTLEFS.h>
// === these work, but can wait till full integration 


const int CLK = 23;  //19 //23 //22  //18
const int DIO = 22; //21 //22 //23  //4

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
    const uint8_t Coil_1 = 26;   // 14; //26;  //13; // 14; //33
    const uint8_t Coil_2 = 14;  //  26;  
    const uint8_t fan_1 = 25;  //   33;
    const uint8_t fan_2 = 33; //    25;

    const uint8_t channel_1 = 0, channel_2 = 1;
    const uint8_t duty_cycle = 4; // 1 - 16 bits
    const uint16_t pwm_frequency = 12000; // 5kHz up to 30kHz...

    const uint8_t Speed_0 = 0;
    const uint8_t Speed_1 = 20;
    const uint8_t Speed_2 = 32;
    const uint8_t Speed_3 = 90;
    const uint8_t Speed_4 = 191;
    const uint8_t Speed_5 = 255;


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

// :: CODE OF 7 FUNCTIONS 
void beep(uint8_t times); // ~ 30 lines
void listen_to_TouchCtrl(); // ~ 30 lines
void touchProcessor(); // ~ 67 lines
void Monitor_cookingZones(uint8_t coil_1_index, uint8_t coil_2_index); // ~216 lines
void DispLY(uint8_t positions_filled, uint8_t left_num, uint8_t right_num); // ~7 lines
void running(long intervo); // ~ 46 lines
void MonitorBattery(); // ~ 220 lines
void updateTime();

// Example of manually defining a display pattern
//  ________
// |        |
//  --------
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
//ATTACK THE COILS first --- if possible, use direct port access to lower response to nanosex 
  pinMode(Coil_1, OUTPUT);  digitalWrite(Coil_1, HIGH); //ACTIVE WHEN LOW
  pinMode(Coil_2, OUTPUT);  digitalWrite(Coil_2, HIGH); //ACTIVE WHEN LOW

//DDRD |= B00000001; //make two pins as outputs in mucroseconds
//PORTD = B00001001; // write high bits on those two ports

  // THEN THE DISPLAYS
 // Serial.begin(115200); delay(100);  Serial.println("Booting..."); //Serial.flush();

  pinMode(flashin_LED, OUTPUT); digitalWrite(flashin_LED, HIGH);//INDICATOR LED

  pinMode(charge_by_Solar, OUTPUT);  digitalWrite(charge_by_Solar, HIGH);//ACTIVE HIGH

  //Screen.init();
  Screen.setBrightness(7); // SET BRIGHTNESS OF THE 7 SEGMENT
  Screen.clear();

  Screen.showString("OFF-ON");  delay(200);      //  --------- 1
  Screen.clear();

    for(int count = 0; count < 1; count++) {
          Screen.showAnimation(ANIMATION, FRAMES(ANIMATION), TIME_MS(1));
      }

  Screen.clear();
  Screen.showNumber(1234567);  delay(100); // overflow     --- 2



//  SPEED CONTROLLED FANS
    ledcAttachPin(fan_1, channel_1);    
    ledcSetup(channel_1, pwm_frequency, duty_cycle);
    
    ledcAttachPin(fan_2, channel_2);
    ledcSetup(channel_2, pwm_frequency, duty_cycle);

    ledcWrite(channel_1, Speed_5); ledcWrite(channel_2, Speed_5);


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
  while(foo>=0) { delay(5);
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


   // Serial.println("Boot Complete!"); //0n

  /*
    if(!LittleFS.begin(true)){
          Serial.println("An Error has occurred while mounting File System");
      return;
    }

  
    //File local_storage = LittleFS.open("/Cooker_Log.txt", FILE_APPEND);
    File storage = LittleFS.open("Maksol.txt");
    
    if(!storage) {Serial.print("Error Opening Storage!"); return;}
    else {
      if(storage.print("Cooking Round: ")){ Serial.print("Data Saved!");}
      else Serial.println("Data Save Failed!");
    }
  storage.close();
  */

/*
    bool setCpuFrequencyMhz(uint32_t cpu_freq_mhz);

    //  240, 160, 80    <<< For all XTAL types
    //  40, 20, 10      <<< For 40MHz XTAL
    //  26, 13          <<< For 26MHz XTAL
    //  24, 12          <<< For 24MHz XTAL

*/
/*
Serial.print("Crystal Frequency: "); Serial.println(getXtalFrequencyMhz());
Serial.print("CPU Frequency: "); Serial.println(getCpuFrequencyMhz());

    setCpuFrequencyMhz(40); delay(500);
   //ModemSleep();

Serial.print("Crystal Frequency: "); Serial.println(getXtalFrequencyMhz());
Serial.print("CPU Frequency: "); Serial.println(getCpuFrequencyMhz());
*/

//for(;;){}

 setCpuFrequencyMhz(40); delay(500);
}

/*
It turned out to be 0.125MHz (or 125KHz) as you can see. 
This was nearly expected because in the previous test the CPU clock rate was 240MHz and the max GPIO switching rate was 3MHz. 
Now, the CPU clock is reduced to 10MHz, so the max GPIO pin switching is expected to fall down to 1/24th of its previous value.
*/


uint8_t currentMode = 1;
uint8_t heat_1_level = 0;
uint8_t heat_2_level = 0;


  bool locked_up = false;   bool locked_down = false;
  bool locked_up2 = false;  bool locked_down2 = false;


/* ONLY 4 SUPERLOOP TASKS:
 * Read battery level
 * Listen to touch interrupts
 * Update cooking coils manager
 * Shutdown when pressed or when idle (running & unloaded) for 30 minutes 
 */
unsigned long batt_runs = 0; // max out at 4.29B

bool Coil_1_ON = false, Coil_2_ON = false;


unsigned long long now_now = 0;
unsigned long long time_of_last_activity = 0;
unsigned long long auto_turn_off = 18000000; // 5 hours //18 million milliseconds

uint64_t cooker_clock = 0; uint8_t second = 0; uint8_t minute = 0; uint8_t hour = 0;
uint64_t previous = 0;

uint32_t listens_to_user = 0; // maxes out at 4Billion

void loop() { //to max out the 6 digit display:
/* 
    1. Listen for user control 10-100X each second
    2. Read changes in battery voltage once every second
    3. Monitor Cooking if it is cooking, every single time
    4. When 49 days have passed, do not allow cooking to start until either a restart or overflow...
    5. Do a free run when not reading battery, or listening to inputs or monitoring running time
*/
     now_now = esp_timer_get_time()/1000ULL;

     cooker_clock = now_now;

  if(cooker_clock - previous >= 1000){ // things we can do once every second
      second++; //updateTime();
      MonitorBattery();
      previous = cooker_clock;
  }

  if(listens_to_user%100 == 0) listen_to_TouchCtrl(); // listen for user control 100X every second

 
 

    Monitor_cookingZones(heat_1_level, heat_2_level); 
    
      
       listens_to_user++;

   // prevent prolonged idle cooking:  6 or 7 or 8 or 10 hours
   if((now_now - time_of_last_activity) >= (6*60*60*1000)){ //6|7|8 x 60 x 60 x 1000ULL //if has been ON for six hours, take it OFF and flush the system
        //if(heat_1_level != 0) heat_1_level = 0; 
        //if(heat_2_level != 0) heat_2_level = 0;
          heat_1_level = 0;  heat_2_level = 0;
          Monitor_cookingZones(heat_1_level, heat_2_level);
          DispLY(2, heat_1_level, heat_2_level);
          beep(1);
          locked_up = false; locked_up2 = false;
          time_of_last_activity = now_now;
          
   } 

 //if((now_now - time_of_last_activity) >= ) esp_light_sleep_start(); //Mode: The CPU is paused. Any wake-up events (MAC, host, RTC timer, or external interrupts) will wake up the chip.
 //if(wake_up_call){}

   /*
if(coil_1_started  > 4294960000) {   coil_1_index = 0; return; } // prevent superoverflow for last minute ...4,294,967,295 || on d 49th day 16th hour
if(coil_1_started  > auto_turn_off) {coil_1_index = 0; return; } //if the cooker has been ON NONSTOP for 5 hours...

if(coil_2_started > 4294960000) {   coil_2_index = 0; return; } // prevent superoverflow for last minute
if(coil_2_started > auto_turn_off) {coil_2_index = 0; return; } //if the cooker has been ON NONSTOP for 5 hours...
*/



/* different read frequencies... depending on whether loaded or not
  if(!Coil_1_ON && !Coil_2_ON) {if(batt_runs%80000 == 0) MonitorBattery();  }// 100k loop cycles for each second at 240MHz 
      
  else { //if either coil 1 is on or 2 is ON or BOTH
                                if(batt_runs%40000 == 0) MonitorBattery(); // Query the battery once every second since it doesn't change that rapidly
    }
     batt_runs++;
*/
}

void DispLY(uint8_t positions_filled, uint8_t left_num, uint8_t right_num){ // bool animate = false, float center = 0.00

  if(positions_filled == 2){
      Screen.showNumber(right_num, false); 
      Screen.showNumber(left_num, false, 1, 0);
  }

}

void quit_cooking(){ }//turn off the correspoinding cooking zones and diable timers

void updateTime(){
        if(second >= 60){
           second = 0; minute++;
        }
        if(minute >= 60){
           minute = 0; hour++;
         }
        if(hour >= 24){
           hour = 0; //day++;
        }

}
  bool isPower_enough = false; bool isCharging = false;
  uint8_t Battery_Level_Counter; 
//float voltage = 0.00;

  bool is_critical = false; bool processes_ended = false;
  float voltage = 0.00;

  bool toggled = false; //for the Power LED
  bool blinking_battery = false;


  bool volt_disp_invoked = false; bool screen_cleared = false;
  bool critical_power_locked = false;

void MonitorBattery(){ //Power Manage(battery, solar)
 //Serial.print("Battery Read Count: "); Serial.println(batt_runs);
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
    summation /= 10.0f;
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


  if(voltage < 10.20) { //when a Power CCT PD of 10.20V represents 11.0V terminal EMF
     Battery_Level_Counter = 0; 
      if(!LED0_ON)  {  digitalWrite(level0, HIGH);  LED0_ON = true; } 
  //   isPower_enough = false; 
       if(!is_critical) { /* Serial.println("\t\tPower Too Low!!!"); */ is_critical = true; }
       //Screen.clear();
    
     // ALWAYS PREVENT DEEP DISCHARGE 
     
if(Battery_Level_Counter == 0){ 

      if(heat_1_level > 0) { // display for the left most cooking zone                      
                            for(int index1 = heat_1_level; index1>=0; index1--){ Screen.showNumber(index1, false, 1, 0);  delay(50); }
                                heat_1_level = 0;  
                            
                            locked_down = true; locked_up=false; 
                            critical_power_locked = true;
                      //      Serial.println("COIL 1 Turned OFF"); 
                        //    Serial.println("COIL 1 Turned OFF DUE TO POWER"); 
                            //volt_disp_invoked = true;
                          }
      if(heat_2_level > 0) { // display for the left most cooking zone                      
                            for(int index2 = heat_2_level; index2>=0; index2--){ Screen.showNumber(index2, false);  Screen.showNumber(heat_1_level, false, 1, 0); delay(50); }
                            heat_2_level = 0;
                            //Monitor_cookingZones(heat_1_level, coil_2_index); //since it is auto-summoned by calling function
                            
                            locked_down2 = true; locked_up2=false; 
                            critical_power_locked = true;
                    //        Serial.println("COIL 2 Turned OFF by Power Manager due to undervoltage"); 
                    //        Serial.println("ADDRESS POWER!");
                            
                          }
  }
     // if(!isCharging) { digitalWrite(charge_by_Solar, HIGH); isCharging = true; beep(1); } 
     
  } 

else{ //if voltage is  10.20 at the very least

    // Battery_Level_Counter = 1;

        if(is_critical) {/*Serial.println("\t\tPower Sufficient!!!");*/ is_critical = false; }

      if(volt_disp_invoked){ // 2dp or 3dp: 12.87 or 12.9

      //split the voltage: 12.868 into 12 & 86
      uint16_t part_1 = int(voltage); // int(12.868) = 12
      uint16_t part_2 = int(10*(voltage - int(voltage+0.05))); // 10 x (12.868 - 12) = int(9.18) = 9
      uint16_t part_3 = int(100*(voltage - int(voltage))); // 100 x (12.868 - 12) = int(86.8) = 86

        // int workin_volt = voltage*100; char voltage_str[6]; //12.86 => 1286
            uint16_t workin_volt = (part_1*10) + part_2; char voltage_str2[5]; //12.9 => 129
            itoa(workin_volt, voltage_str2, 10); 

          //   Serial.print("Actual Voltage: "); Serial.println(voltage, 3);
         //    Serial.print("Rounded Voltage: "); Serial.println(workin_volt);
        //     Serial.print("Voltage String 2: "); Serial.println(voltage_str2);
              Screen.showString(voltage_str2, 3, 1, 0b01000000); // (float, int dots, bool leading zeroz, int length, int position)
            
      }

    else {
        if(!screen_cleared) { Screen.showNumber(heat_2_level, false);  Screen.showNumber(heat_1_level, false, 1, 0);  screen_cleared = true;}
      }



if(!isCharging){ //if charging voltage has been autodetected

  

//between 10.10V to 11.10V // repsentative of 11.0V to 12.0V
    if(voltage >= 10.20 && voltage < 11.10) {// ONE BAR
            Battery_Level_Counter = 1; isPower_enough = true; processes_ended = false;
        //LED indicators --- battery bars
        if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
        if(!LED1_ON) {  digitalWrite(level1, HIGH); LED1_ON = true; }//ON THE LOWER MOST BAR [common denominator]

        if(LED2_ON)  {  digitalWrite(level2, LOW); LED2_ON = false; } 
        if(LED3_ON)  {  digitalWrite(level3, LOW); LED3_ON = false; }
        
         digitalWrite(charge_by_Solar, HIGH); isCharging = true; beep(1); //PWM THE CHARGING VOLTAGE
    } 

//between 11.10V to 12.30 // repsentative of 12.0V to 12.55V        
 else if(voltage >= 11.10 && voltage < 12.30) { //TWO BARS
          Battery_Level_Counter = 2; isPower_enough = true; critical_power_locked = false;
        if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
        if(!LED1_ON) {  digitalWrite(level1, HIGH); LED1_ON = true; }//ON THE LOWER MOST BAR [common denominator]
        if(!LED2_ON) {  digitalWrite(level2, HIGH); LED2_ON = true; }
        if(LED3_ON)  {  digitalWrite(level3, LOW);  LED3_ON = false;}
      
        digitalWrite(charge_by_Solar, HIGH); isCharging = true; beep(1); 
    } // START CHARGING...ASAP //pulse width modulate the charging voltage

 else if(voltage >= 12.30) { //THREE BARS at only 12.3V
          Battery_Level_Counter = 3; isPower_enough = true; critical_power_locked = false;
       if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
       if(!LED1_ON) {  digitalWrite(level1, HIGH); LED1_ON = true; }//ON THE LOWER MOST BAR [common denominator]
       if(!LED3_ON) {  digitalWrite(level3, HIGH); LED3_ON = true; } //on the second bar
       if(!LED2_ON) {  digitalWrite(level2, HIGH); LED2_ON = true; } // on the third bar

      if(voltage < 12.60){ //anything below 12.6V should auto toggle charging relay
             if(!isCharging) {  digitalWrite(charge_by_Solar, HIGH); isCharging = true; beep(1);  } 
      } // do nothing if 12.6 - 14.40V 
      else if(voltage > 14.40){ if(isCharging) { // redundant but required for safety... just in case
            digitalWrite(charge_by_Solar, LOW); isCharging = false; beep(3);}
        }

      
    }

// Serial.println("Cooker not charging..."); Serial.println();

 }




else if(isCharging){  //adjust the indicators to signal full or half with the appropriate voltages!

        if(voltage < 11.01){ //CRITICAL POWER INDICATOR
                  if(!LED0_ON)  {  digitalWrite(level0, HIGH);  LED0_ON = true; }  
                  if(LED1_ON)  {  digitalWrite(level1, LOW);  LED1_ON = false; } 
                  if(LED2_ON)  {  digitalWrite(level2, LOW);  LED2_ON = false; }
                  if(LED3_ON)  {  digitalWrite(level3, LOW);  LED3_ON = false; }
                  
                  if(!blinking_battery) {  digitalWrite(level1, HIGH); LED1_ON = true; }//BLINK THE LOWER MOST BAR 
                  if(blinking_battery)  {  digitalWrite(level1, LOW);  LED1_ON = false;}

          }
   else  if(voltage >= 11.01 && voltage <= 12.20){ //only one bar (blinking) till 12.20V
              Battery_Level_Counter = 1;
                  if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
                  if(LED2_ON)  {  digitalWrite(level2, LOW);  LED2_ON = false; }
                  if(LED3_ON)  {  digitalWrite(level3, LOW);  LED3_ON = false; }
                  
                  if(!blinking_battery) {  digitalWrite(level1, HIGH); LED1_ON = true; }//BLINK THE LOWER MOST BAR 
                  if(blinking_battery)  {  digitalWrite(level1, LOW);  LED1_ON = false;}

        }
    else if(voltage > 12.20 && voltage <= 13.30){ //two bars bar lock in at 12.21V, middle blinking
            Battery_Level_Counter = 2;
                    if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
                    if(!LED1_ON) {  digitalWrite(level1, HIGH); LED1_ON = true; }// KEEP THIS LOWER BAR LIT
                    if(LED3_ON)  {  digitalWrite(level3, LOW);  LED3_ON = false;}
                    
                    if(!blinking_battery) {  digitalWrite(level2, HIGH);  LED2_ON = true;} //BLINK THE MIDDLE BAR
                    if(blinking_battery) {   digitalWrite(level2, LOW);  LED2_ON = false;}
                    

        }
    else if(voltage > 13.30){ //three bars bar lock in at 13.3V .... blink the top most
              Battery_Level_Counter = 3;
                             //if is starting to go beyond 13.5V (maximum fully charged voltage) present... all three bars
                    if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
                    if(!LED1_ON) {  digitalWrite(level1, HIGH); LED1_ON = true; } //KEEP LOWER BAR LIT
                    if(!LED2_ON) {  digitalWrite(level2, HIGH); LED2_ON = true; } // KEEP MIDDLE BAR LIT
                    if(!blinking_battery) {   digitalWrite(level3, HIGH);  LED3_ON = true;} //BLINK THE TOP BAR
                    if(blinking_battery) {    digitalWrite(level3, LOW);  LED3_ON = false;}
    
                    if(voltage > 14.40)  { // make all bars static: no bar blinks... just like 12.8V for not charging
                                if(LED0_ON)  {  digitalWrite(level0, LOW); LED0_ON = false; } //OFF CRITICAL POWER INDICATOR
                                if(!LED1_ON) {  digitalWrite(level1, HIGH); LED1_ON = true; } //KEEP LOWER BAR LIT
                                if(!LED2_ON) {  digitalWrite(level2, HIGH); LED2_ON = true; } // KEEP MIDDLE BAR LIT
                                if(!LED3_ON) {  digitalWrite(level3, HIGH); LED3_ON = true; } // KEEP THE TOP BAR ON
                  
                             digitalWrite(charge_by_Solar, LOW); isCharging = false; beep(3);  
                        }// PREVENT OVERCHARGE
        }

//blink power button when charginh
         
        //if(!toggled){digitalWrite(flashin_LED, HIGH); toggled = true; }
        //else {       digitalWrite(flashin_LED, LOW);  toggled = false;}
        //blinking_battery = toggled;
         blinking_battery = !blinking_battery;
      //  Serial.println("Cooker charging..."); Serial.println();

        }
    }


}




  uint8_t up1 = 0, up2 = 0; //const int threshold_up = 30; //for touch abilities
  uint8_t down1 = 0, down2 = 0; //const int threshod_dn = 40; // for touch abilities


  bool lock_press = false, passed_yet = false;
  unsigned long start_counting = 0;

void listen_to_TouchCtrl(){
  up1 = 0; down1 = 0;
  up2 = 0; down2 = 0;
  lock_press = false;
  
  //if(!critical_power_locked){

//lock max at 5 and min at 0 ... no press beyond those limits
if(!locked_up) {    up1 = digitalRead(touch_1); }
if(!locked_down){ down1 = digitalRead(touch_2); }
if(!locked_up2) {   up2 = digitalRead(touch_3); }
if(!locked_down2){down2 = digitalRead(touch_4); }
//}

//universal tactile feeedback
if(up1 || up2 || down1 || down2){ lock_press = true;  } 
else start_counting = now_now; // // --- 1000; // --- to prevent overflow when statted at 4.2Bn ---//

      if(lock_press && !passed_yet) {start_counting = now_now; passed_yet = true;}
        if(passed_yet){
              if((now_now - start_counting) >= 50 && (now_now - start_counting) < 100) { digitalWrite(flashin_LED, HIGH);
                                                                                       beep(1);
                                                                                       digitalWrite(flashin_LED, LOW);
                                                                                       time_of_last_activity = now_now; //wake up from idle
                                                                                       touchProcessor(); // process input
                                                                                       }
         /*EXTRA SETTINGS MODE*/    //e.g. TIMER, VOLTAGE DISP, 
              if((now_now - start_counting) >= 1200 && (now_now - start_counting) < 1250) { beep(2); volt_disp_invoked = !volt_disp_invoked; screen_cleared = false;}  
        }
}


uint8_t CoilNum = 0; uint8_t whichCoil = 0;

bool reducing = false;
//Power Mgt Activated: ONCLICK ---- basic Power Saving mode
void touchProcessor(){ // LEVEL SHIFTER WITH SEMI-DYNAMIC POWER MANAGEMENT INTEGRATED
    
   //  MonitorBattery(); //if input detected from user, first check how much is on battery
      if(Battery_Level_Counter < 1) { //only react to LEVEL LOWERING when in ultra low power mode
    
          if(up1 || up2) beep(2); // // prevent deep discharge ..NO.. Screen.display(1, 'n'); Screen.display(2, 'o');

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

if(down1 == 1 && up1 == 0) {  
              if(heat_1_level > 0) {locked_down=false; locked_up=false; heat_1_level--; } 
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
/*
      Serial.println();
      Serial.print("Coil 1 Level: "); Serial.println(heat_1_level);
      Serial.print("Coil 2 Level: "); Serial.println(heat_2_level);
      Serial.println();
      //vTask.execute();
*/
        //return 0;
}





long interval = 0; long interval2 = 0;

unsigned long long coil_1_started = 0, coil_2_started = 0;  //up to year 200+

bool waiting = false; bool auto_cooling_1 = false;
bool waiting2 = false; bool auto_cooling_2 = false;

const   long wait_ko = 30000; // 30 sec constant Cooling time \\ 300,000
bool fan1_ON = false; bool fan2_ON = false;
unsigned long long fan_1_sat  = 0, fan_2_sat  = 0;
unsigned long long fan_1_stop = 0, fan_2_stop = 0;

bool fan1_speed_1_engaged = false, fan1_speed_3_engaged = false, fan1_speed_5_engaged = false; // only 3 gears for the smaller Cooking Zone

bool fan2_speed_1_engaged = false, fan2_speed_2_engaged = false, fan2_speed_3_engaged = false, fan2_speed_4_engaged = false, fan2_speed_5_engaged = false;


//revised intervals:: HALVED
const   long interval_1 = 300000; //  5 minutes
const   long interval_2 = 600000; // 10 minutes
const   long interval_3 = 900000; // 15 minutes
const   long interval_4 = 1200000; // 20 minutes
const   long interval_5 = 1500000; // 25 minutes
const   long interval_6 = 1800000; // 30 minutes


unsigned long runtime_tracker = 0;
unsigned long long fan_1_timer = 0;
unsigned long long fan_2_timer = 0;

// === esp_timer_get_time(); ... this gives microsecond resolution for up to 290,000 years

void Monitor_cookingZones(uint8_t coil_1_index, uint8_t coil_2_index){ //COIL INDEX IS THE MASTER CONTROLLER 
 //WHENEVER POWER IS ALARMINGLY LOW < 11.4V auto stop cooking
      if(coil_1_index == 0) { 
                    if(Coil_1_ON) {
                        digitalWrite(Coil_1, HIGH); Coil_1_ON = false; 
                        coil_1_started = 0;
                        fan_1_stop = now_now; 
                      }
                    if(fan1_ON){ //whether it was in the wait or NOT, take it OFF after 5 seconds
                          if(now_now - fan_1_stop >= 5000) { // forced cooling at shutdown
                                ledcWrite(channel_1, Speed_0);
                                beep(1);
                                fan1_speed_1_engaged = false; fan1_speed_3_engaged = false;  fan1_speed_5_engaged = false;
                                waiting = false; fan1_ON = false;
                                fan_1_stop = 0; 
                               // Serial.println("Fan 1 OFF!");
                            }
                        }
                  }

      if(coil_2_index == 0) { 
                        if(Coil_2_ON) {
                            digitalWrite(Coil_2, HIGH); Coil_2_ON = false;
                            coil_2_started = 0;  
                            fan_2_stop = now_now;  
                        }
                      if(fan2_ON){ //whether it was in the wait or NOT, take it OFF after 5 seconds
                          if(now_now - fan_2_stop >= 5000) { // ((now - fan_2_stop) >= 4000) forced cooling at shutdown
                                ledcWrite(channel_2, Speed_0);  
                                beep(1);
                                 fan2_speed_1_engaged = false; fan2_speed_2_engaged = false; fan2_speed_3_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false;

                                waiting2 = false; fan2_ON = false;
                                fan_2_stop = 0;
                             //   Serial.println("Fan 2 OFF!");
                           }
                     }
                    
               }

//close if zero


//close if power is set to zero

if(coil_1_index != 0) { //if toggled to 1, 2, 3, 4, 5
         if(Coil_1_ON) { /*
               runtime_tracker = (1000 + coil_1_started + interval - millis());
            long run_time_mins = runtime_tracker/60000; long run_time_secs = (runtime_tracker%60000);
    
            Serial.print("Coil "); Serial.print(CoilNum); Serial.print(" running for: "); 
            
            Serial.print(run_time_mins); Serial.print(":");  Serial.print(run_time_secs);Serial.print(" Minutes");
            
            Serial.print(" (Total: "); Serial.print(runtime_tracker/1000); Serial.println(" Seconds)"); Serial.println();
    */
            if(!waiting){//turn it on instantly but very silently... for the first 60 seconds

      /*GEAR 1*/          if(!fan1_ON) { // 30 seconds
                              ledcWrite(channel_1, Speed_1); fan1_ON = true;  fan1_speed_1_engaged = true; fan1_speed_3_engaged = false; fan1_speed_5_engaged = false; /* Serial.print("Fan 1 Speed: "); Serial.println(Speed_1); */ }
                          if(fan1_ON)  { fan_1_timer = (long)(now_now - coil_1_started); 

      /*GEAR 3*/             if(fan_1_timer >= 30000 && fan_1_timer < 90000){ // for ... 1 minute: 60 seconds || till 1:30
                                if(!fan1_speed_3_engaged){ledcWrite(channel_1, Speed_3);  fan1_speed_3_engaged = true; fan1_speed_1_engaged = false; fan1_speed_5_engaged = false;} } /*  Serial.print("Fan 1 Speed: "); Serial.println(Speed_4);  */

      /*GEAR 5*/             if(fan_1_timer >= 90000 && fan_1_timer < 270000){ // for ... 3minutes: 180 seconds || till 4:30
                                if(!fan1_speed_5_engaged){ledcWrite(channel_1, Speed_5);  fan1_speed_5_engaged = true; fan1_speed_3_engaged = false;  fan1_speed_1_engaged = false;} } /*  Serial.print("Fan 1 Speed: "); Serial.println(Speed_4);  */

      /*GEAR 3*/             if(fan_1_timer >= 270000 && fan_1_timer < 330000){  // for... 1 minute: 60 seconds || till 5:30
                                if(!fan1_speed_3_engaged){ledcWrite(channel_1, Speed_3); fan1_speed_3_engaged = true; fan1_speed_1_engaged = false; fan1_speed_5_engaged = false;} }  /*  Serial.print("Fan 1 Speed: "); Serial.println(Speed_4);  */

      /*GEAR 1*/             if(fan_1_timer >= 330000 && fan_1_timer < 360000){ // for... 60 seconds || till 6:30
                                if(!fan1_speed_1_engaged){ledcWrite(channel_1, Speed_1); fan1_speed_1_engaged = true; fan1_speed_3_engaged = false; fan1_speed_5_engaged = false; } }  /* Serial.print("Fan 1 Speed: "); Serial.println(Speed_2);  */ 

      /*GEAR 3*/             if(fan_1_timer >= 360000 && fan_1_timer < 450000){ // for... 90 seconds || till 7:30
                                if(!fan1_speed_3_engaged){ledcWrite(channel_1, Speed_3); fan1_speed_3_engaged = true; fan1_speed_1_engaged = false; fan1_speed_5_engaged = false;} }  /* Serial.print("Fan 1 Speed: "); Serial.println(Speed_2);  */ 

      /*GEAR 5*/             if(fan_1_timer >= 450000){ //after 7.5 minutes, keep fan running at full blast || till inf
                                if(!fan1_speed_5_engaged){ledcWrite(channel_1, Speed_5); fan1_speed_5_engaged = true; fan1_speed_3_engaged = false; fan1_speed_1_engaged = false; } }  //to avoid repititions /* Serial.print("Fan 1 Speed: "); Serial.println(Speed_2);  */ 
                

                          } //if(fan1_ON)
                        } //if(!waiting)
            // else if(waiting){}
       }


    else if(!Coil_1_ON) { //check if it hasn't alreay been turned ON
            if(!waiting) { //turn it ON if it is not in waiting state

                  digitalWrite(Coil_1, LOW); Coil_1_ON = true; coil_1_started = now_now; 
                  auto_cooling_1 = false;
                  
             //     Serial.print("Coil "); Serial.print(CoilNum); Serial.println("FIRED !!!"); 
              //    Serial.print("At: "); Serial.println(coil_1_started/1000);
                  }
              else if(waiting){
                if(!auto_cooling_1) {beep(1); auto_cooling_1 = true;}
                //  Serial.print("\tWaiting for ... "); Serial.print(((1000 + coil_1_started + interval + wait_ko) - now)/1000); Serial.println(" seconds");
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
    
            if(!waiting2){//turn it on instantly but very silently... for the first FULL minute || till 1:00
      /*GEAR 1*/          if(!fan2_ON) {   /*Serial.print(F("Fan 2 Speed: ")); /*Serial.println(F(Speed_1));*/ //
                                        ledcWrite(channel_2, Speed_1); fan2_ON = true;  fan2_speed_1_engaged = true; fan2_speed_2_engaged = false; fan2_speed_3_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false; // Serial.print("Fan 2 Speed: "); Serial.println(Speed_1);  
                                        }
                          if(fan2_ON){ //only one JAB
                                fan_2_timer = (long)(now_now - coil_2_started); // fan_2_timer = now - coil_2_started;

      /*GEAR 2*/             if(fan_2_timer >= 60000 && fan_2_timer < 75000){ // for ... 15 seconds || till 1:15
                                if(!fan2_speed_2_engaged) {ledcWrite(channel_2, Speed_2);  fan2_speed_2_engaged = true;  fan2_speed_1_engaged = false; fan2_speed_3_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false;;   /*  Serial.print("Fan 2 Speed: "); Serial.println(Speed_2); */   } } 

      /*GEAR 3*/             if(fan_2_timer >= 75000 && fan_2_timer < 90000){ // for ... 15 seconds || till 1:30
                                if(!fan2_speed_3_engaged){ledcWrite(channel_2, Speed_3);  fan2_speed_3_engaged = true; fan2_speed_1_engaged = false; fan2_speed_2_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false;;   /*  Serial.print("Fan 2 Speed: "); Serial.println(Speed_3); */  } }  

      /*GEAR 4*/             if(fan_2_timer >= 90000 && fan_2_timer < 180000){ // for ... 90 seconds || till 2:30
                                if(!fan2_speed_4_engaged){ledcWrite(channel_2, Speed_4);  fan2_speed_4_engaged = true; fan2_speed_1_engaged = false; fan2_speed_2_engaged = false; fan2_speed_3_engaged = false; fan2_speed_5_engaged = false;;    /*  Serial.print("Fan 2 Speed: "); Serial.println(Speed_4); */ } } //MACHINE STATES to avoid repititions 

      /*GEAR 5*/             if(fan_2_timer >= 180000 && fan_2_timer < 390000){ // for ... 210 seconds || till 6:00
                                if(!fan2_speed_5_engaged){ledcWrite(channel_2, Speed_5);  fan2_speed_5_engaged = true; fan2_speed_1_engaged = false; fan2_speed_2_engaged = false; fan2_speed_3_engaged = false; fan2_speed_4_engaged = false;;    /*  Serial.print("Fan 2 Speed: "); Serial.println(Speed_5); */ } } 

      /*GEAR 3*/             if(fan_2_timer >= 390000 && fan_2_timer < 420000){  // for... 30 seconds  || till 7:00
                                if(!fan2_speed_3_engaged){ledcWrite(channel_2, Speed_3); fan2_speed_3_engaged = true; fan2_speed_1_engaged = false; fan2_speed_2_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false;;  /* Serial.print("Fan 2 Speed: "); Serial.println(Speed_3);*/ } } 
      
      /*GEAR 2*/             if(fan_2_timer >= 420000 && fan_2_timer < 450000){ // for... 30 seconds || till 7:30
                                if(!fan2_speed_2_engaged){ledcWrite(channel_2, Speed_2); fan2_speed_2_engaged = true; fan2_speed_1_engaged = false; fan2_speed_3_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false;;  /* Serial.print("Fan 2 Speed: "); Serial.println(Speed_2);*/ } }  


      /*GEAR 1*/             if(fan_2_timer >= 450000 && fan_2_timer < 480000){ // for... 30 seconds || till 8:00
                                if(!fan2_speed_1_engaged){ledcWrite(channel_2, Speed_1); fan2_speed_1_engaged = true; fan2_speed_2_engaged = false; fan2_speed_3_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false;; /* Serial.print("Fan 2 Speed: "); Serial.println(Speed_1);*/} }  

      /*GEAR 3*/             if(fan_2_timer >= 480000 && fan_2_timer < 510000){ // for... 30 seconds || till 8:30
                                if(!fan2_speed_3_engaged){ledcWrite(channel_2, Speed_3); fan2_speed_3_engaged = true; fan2_speed_1_engaged = false; fan2_speed_2_engaged = false; fan2_speed_4_engaged = false; fan2_speed_5_engaged = false;; /* Serial.print("Fan 2 Speed: "); Serial.println(Speed_3);  */} }   

      /*GEAR 5*/             if(fan_2_timer >= 510000){ //after 9 minutes, keep fan running at full blast || beyond 8:30
                                if(!fan2_speed_5_engaged){ledcWrite(channel_2, Speed_5); fan2_speed_5_engaged = true; fan2_speed_1_engaged = false; fan2_speed_2_engaged = false; fan2_speed_3_engaged = false; fan2_speed_4_engaged = false;; /* Serial.print("Fan 2 Speed: "); Serial.println(Speed_5);  */ } }  
                
                          } //if(fan2_ON)
                        } //if(!waiting2)
              else if(waiting2){ }
            
          } //if(Coil_2 coil 2 is already on and running...


         else if(!Coil_2_ON) { 

            if(waiting2){ //thrown into WAIT LOOP
                  //Serial.print("\tThrown into wait loop ... "); Serial.print(((1000 + coil_1_started + interval + wait_ko) - now)/1000); Serial.println(" seconds");
                if(!auto_cooling_2) {beep(1); auto_cooling_2 = true;}
              }
          
  
            else if(!waiting) { //turn it ON if it is not in waiting state

                    digitalWrite(Coil_2, LOW); Coil_2_ON = true;  coil_2_started = now_now;  
                    auto_cooling_2 = false;
              //    Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" FIRED !!!"); 
             //     Serial.print("At: "); Serial.println(coil_1_started/1000);
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

              if((now_now - coil_1_started) < interval) {//((now - coil_1_started) < interval) //Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON");
                    /*Serial.print(" Firing till ");  Serial.println((1+interval + coil_1_started)/1000);*/  /*if(reducing) waiting = true;*/
                           if(blink_counta <= 16000){ if(!is_togod) { digitalWrite(flashin_LED, HIGH); is_togod = true;}}
                      else if(blink_counta > 16000 && blink_counta <= 30000) { if(is_togod){digitalWrite(flashin_LED, LOW); is_togod = false;} }
                      else if(blink_counta > 30000) {blink_counta = 0; is_togod = false; }

                        blink_counta++;
                                
                  }
              else { // if has been ON for more than first interval
                  if((now_now - coil_1_started) < (interval + wait_ko)){ //now - coil_1_started // --- TURN OFF & induce internal FORCED COOLING
                        if(Coil_1_ON) { waiting = true; //Turn off COIL to COOL
                              digitalWrite(Coil_1, HIGH); Coil_1_ON = false;
                          }
                          else { /*Serial.print("Waiting Till "); Serial.println((interval + coil_1_started + wait_ko)/1000); */ }
                    }
                    else { waiting = false; /*Serial.println("Waiting Time Over - !"); */} // --- WAITING TIME OVER 
               }

    

              if(now_now - coil_2_started < interval2) { // (now - coil_2_started < interval2) /*Serial.print("Coil "); Serial.print(CoilNum); Serial.println(" ON"); */
                          if(blink_counta <= 8000){ if(!is_togod) { digitalWrite(flashin_LED, HIGH); is_togod = true;}}
                      else if(blink_counta > 8000 && blink_counta <= 15000) { if(is_togod){digitalWrite(flashin_LED, LOW); is_togod = false;} }
                      else if(blink_counta > 15000) {blink_counta = 0; is_togod = false; }

                        blink_counta++;
              }
                
              else { // if has been ON for more than first interval
                  if(now_now - coil_2_started < (interval2 + wait_ko)){// now - coil_2_started // --- TURN OFF & induce internal FORCED COOLING
                     if(Coil_2_ON) { waiting2 = true; //Turn off COIL to COOL
                        digitalWrite(Coil_2, HIGH); Coil_2_ON = false;
                       }
                       else {  /*Serial.print("Waiting For: "); Serial.println(((interval2 + coil_2_started + wait_ko) - millis())/1000);*/
                        //if(!auto_cooling_2){ beep(1); auto_cooling_2 = true;}
                        }
                    }
                    else { waiting2 = false; /*Serial.println("Waiting Time Over - !");*/ } // --- WAITING TIME OVER 
        
    }


}


unsigned long long start_beeping = 0;
unsigned long long sounding_duration; 

bool sound_started = false;
bool sound_locked = false; 

void beep(uint8_t times){
/*
  for(int xy=0; xy<times; xy++){

      if(!sound_locked) { start_beeping = now_now;   sound_locked = true;  }

      if(sound_locked && !sound_started){
          digitalWrite(sound, HIGH);
          sound_started = true;
      }

      if((now_now - start_beeping) > 50){
        digitalWrite(sound, LOW);Battery_Level_Counter
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

/*
void ModemSleep(){
  WiFi.begin("ESP_WIFI", "1234");
  WiFi.setSleep(true);
    if (!setCpuFrequencyMhz(40)){
        Serial.println("Not valid frequency!");
    }
   
    //esp_bluedroid_disable(), esp_bt_controller_disable(), esp_wifi_stop();

 /*
    Since everything is always active in this mode (especially the WiFi module, processing core, and Bluetooth module), the chip consumes about 240 mA of power. 
    It has also been observed that the chip draws more than 790 mA at times, particularly when both WiFi and Bluetooth are used simultaneously.
    In order to save power, you must disable features that are not in use by switching to another power mode.

In modem sleep mode, everything is active except for the WiFi, Bluetooth, and the radio. The CPU remains active, and the clock is configurable.
 
 

}
/*
void disableWiFi(){
        adc_power_off();
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
}

void disableBluetooth(){
    btStop();
}
*/


