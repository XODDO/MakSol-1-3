  // XODDOCODE FOR MAKSOL 3+   19TH JUNE 2025
 //  DUO Zone solar-electric cooker with touch controls, buttons, OLED displays and a TFT color screen
//   Use a secondary microcontroller to cut power if the primary fails.

#include "esp_task_wdt.h"

// INTER-SYSTEM COM PROTOCOLS
#include "Wire.h"
#include "SPI.h"

//REALTIME CLOCK
#include "RTClib.h"
RTC_DS3231 real_time;

//  BIG SCREEN
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"


//SMALL SCREEN
#include <Adafruit_SH110X.h>

//FONTS
#include <Fonts/FreeSansBold24pt7b.h> // FreeSerifBold24pt7b
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define i2c_Address 0x3c //initialize with the I2C addr 0x3C 

#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G LED = Adafruit_SH1106G(128, 64, &Wire, OLED_RESET);

void LED_BUS(uint8_t bus);

// --- TOUCH INPUTS

const uint8_t menu_touch = 32;   // TOUCH 9
const uint8_t back_touch = 33;  // TOUCH 8
const uint8_t add_touch  = 27;   // TOUCH 7
const uint8_t sub_touch = 14;//  TOUCH 6

// COOKING CONTROLS
const uint8_t up_1_pin = 36;
const uint8_t down_1_pin = 39;
const uint8_t up_2_pin = 35;
const uint8_t down_2_pin = 34;


//COOKING
const uint8_t Coil_1 = 16;   
const uint8_t Coil_2 = 17;  
const uint8_t fan_1 = 2;  
const uint8_t fan_2 = 12; 


const uint8_t flashin_LED = 1;
const uint8_t sound = 26;//33;
uint8_t BatteryPin = 25;



// For the Adafruit shield, these are the default.
const uint8_t TFT_DC = 15;
const uint8_t TFT_CS = 5; //39; //5;//39;
const uint8_t RST = 13;      // 4;

const uint8_t TFT_MOSI = 23;
const uint8_t TFT_MISO = 19;
const uint8_t TFT_CLK = 18;

const uint8_t backlight_ = 4;


//  Adafruit_ILI9341 LCD = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, RST, TFT_MISO); //12X slower
    Adafruit_ILI9341 LCD(TFT_CS, TFT_DC, RST); // HARDWARE SPI

uint8_t currentScreen = 1;

uint32_t duration  = 0; float boot_duration = 0.00;
uint64_t now_now = 0, prev_prev = 0, boot_start = 0, button_check_interval = 0; 


//FUNCTION PROTOTYPING
void BootScreen();
void Sleep();
void beep(uint8_t times, uint8_t delay_setting);
void capacitive_touch();
void buttonScan(); 
void checkBatt();
void monitor_cooking();      

void initialize_RTC();        
void query_rtc();

void update_oled_display();
void update_main_display();   

uint8_t boot_level = 0;
char BootMessage[200] = ".";

void setup() {
          boot_start = esp_timer_get_time();
          Serial.begin(115200); delay(5); Serial.println("BOOTING...");
          pinMode(sound, OUTPUT); beep(1,1);
          pinMode(flashin_LED, OUTPUT); digitalWrite(flashin_LED, HIGH); delay(100); digitalWrite(flashin_LED, LOW);

            // STEP 2:
          strcpy(BootMessage, "Initializing Cooking Zones"); // Serial.println(BootMessage);
          pinMode(Coil_1, OUTPUT); digitalWrite(Coil_1, HIGH); delay(5);// put it OFF
          pinMode(Coil_2, OUTPUT); digitalWrite(Coil_2, HIGH); delay(5); // put it OFF
          strcpy(BootMessage, "Cooking Zones Successfully!"); Serial.println(BootMessage);
          delay(50);

    
        //STEP 4
          strcpy(BootMessage, "Initializing Cooling Fans"); //Serial.println(BootMessage);
          pinMode(fan_1, OUTPUT); digitalWrite(fan_1, LOW); delay(5);// KEEP IT ON
          pinMode(fan_2, OUTPUT); digitalWrite(fan_2, LOW); delay(5); // KEEP IT ON
          strcpy(BootMessage, "Cooking Zones Successfully!"); // Serial.println(BootMessage);


        
          LCD.begin();
          LCD.setRotation(3); // for the 2.4" screen
          BootScreen();

          Wire.begin();

          initialize_OLEDz();

      
          pinMode(BatteryPin, INPUT);
      
     //   Disable or Embolo-Tuusi
    //    WiFi.mode(WIFI_STA);

      
      //STEP 5 
      strcpy(BootMessage, "Initializing Realtime Clock"); // Serial.println(BootMessage);
      initialize_RTC();
      Serial.println(BootMessage); delay(1000);
      query_rtc();
          

               //STEP 6
      strcpy(BootMessage, "Initializing Input Buttons"); // Serial.println(BootMessage);
      pinMode(up_1_pin, INPUT);  pinMode(down_1_pin, INPUT);  pinMode(up_2_pin, INPUT);  pinMode(down_2_pin, INPUT);
      strcpy(BootMessage, "Input Buttons initialized!"); // Serial.println(BootMessage);


     digitalWrite(fan_1, HIGH);   // put it OFF
     digitalWrite(fan_2, HIGH);  //  put it OFF

        
   //  Serial.print("Crystal Frequency before: "); Serial.println(getXtalFrequencyMhz());
  //   Serial.print("CPU Frequency: "); Serial.println(getCpuFrequencyMhz());

      setCpuFrequencyMhz(80); delay(200);
    // ModemSleep();

   //  Serial.print("Crystal Frequency after: "); Serial.println(getXtalFrequencyMhz());
  //  Serial.print("CPU Frequency: "); Serial.println(getCpuFrequencyMhz());



  /*
  //   If the ESP32 crashes, coils/fans should not stay ON indefinitely.
  // Forcefully crash the ESP32 (*(volatile int*)0 = 0;) to verify coils turn off.
    esp_task_wdt_config_t twdt_config = {
          .timeout_ms = 5000,  // 5 seconds
          .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Both cores
          .trigger_panic = true
      };
      esp_task_wdt_init(&twdt_config);
      esp_task_wdt_add(NULL); // Add current task (loop task) to WDT

  */


      beep(3,0);


     // Serial.println("Done Booting!"); Serial.println();
      now_now = esp_timer_get_time();
      boot_duration = (now_now - boot_start)/1000000.0;
   //   Serial.print("TOTAL Boot Duration: "); Serial.print(boot_duration, 2); Serial.println(" seconds");
 
// Coils could overheat if cooling fails (e.g., fan jams).
// Fix: Add thermal sensors (e.g., NTC thermistors) and emergency cutoff:   


}

/*
    #define THERMISTOR_PIN 34
void checkTemperature() {
  int adc = analogRead(THERMISTOR_PIN);
  float tempC = // Convert ADC to °C ;
  if (tempC > 120.0) {       // Adjust for your hardware
    setCoil(Coil_1, false);
    setCoil(Coil_2, false);
    beep(5, 1);              // Alert user
  }
}
*/

//Add fan feedback (e.g., tachometer) or timeout to be sure the fan is running
/*
void monitor_fans() {
  if (coil_1_index > 0 && !fan1_ON) {
    if (millis() - fan_1_timer > 10000) { // 10 sec delay
      setCoil(Coil_1, false);             // Emergency shutdown
    }
  }
}
*/


uint8_t up1 = 0, up2 = 0; //const int threshold_up = 30; //for touch abilities
uint8_t down1 = 0, down2 = 0; //const int threshod_dn = 40; // for touch abilities


bool lock_press = false, passed_yet = false;
unsigned long start_counting = 0;


unsigned long long time_of_last_activity = 0;



uint8_t currentMode = 1;

bool locked_up = false;   bool locked_down = false;
bool locked_up2 = false;  bool locked_down2 = false;


bool cooking_started = false, cooking_started2 = false;
bool cooking_screen_set = false;
bool cooking_screen_2_set = false;

 uint8_t coil_1_index = 0;
 uint8_t coil_2_index = 0;

char cooking_clock[50] = "xx:xx:xx"; char cooking_clock2[50] = "XX:XX:XX";

uint8_t time_stamp = 5; bool time_set = false;
unsigned long refresh_interval = 10*1000UL; // if not cooking, limit this to about 10 seconds, otherwise, < 1 second

char SystemTime[50] = "13:45:21"; char ShortTime[32] = "13:45"; 

void loop() {
   now_now = esp_timer_get_time()/1000ULL; // conveerted to milli_seconds

    if(((uint64_t)now_now - (uint64_t)button_check_interval) >= 50) { // 20X each second --- //read buttons
          buttonScan(); 
          capacitive_touch();
          button_check_interval = now_now; 
      }
   
   if(((uint64_t)now_now - (uint64_t)prev_prev) >= refresh_interval){ //once every 10 seconds  if not cooking, and once per second if cooking
        
      //  checkBatt(); // ignore because of Voltage Configuration of the LiPo vs Lead Batts
        monitor_cooking(); 
        monitor_cooling(); // see if the fans are working   
        checkTemperature(); // of the MOSFETs or Coil  
        
        query_rtc();

        update_oled_display();
        update_main_display();   
        
      prev_prev = now_now;
   }

//Enforce maximum runtime (e.g., 3 hours of cooking or 3 hours of being idle): (now_now - full_coil_1_started) > 3*60*60*1000) 
if(cooking_started || cooking_started2){ 
      // prevent prolonged idle cooking:  6 or 7 or 8 or 10 hours
   if((now_now - time_of_last_activity) >= (3*60*60*1000)){ ///if has been ON for 3 hours, take it OFF and flush the system
              beep(5, 2);
              coil_1_index = 0; coil_2_index = 0;
              monitor_cooking(); // STOP COOKING
              
              locked_up = false; locked_up2 = false;
              time_of_last_activity = now_now;
           } 
      } // 
esp_task_wdt_reset();  
}

void monitor_cooling(){
  
}

void checkTemperature(){


}

bool clock_failed = false;

void initialize_RTC(){
  uint8_t trial = 0;
  while(!real_time.begin()) {
         strcpy(BootMessage, "Couldn't find RTC");   Serial.flush();
         trial++; delay(50);
      if (trial >= 3)break;  

      clock_failed = true;
    } //vTaskDelay(500 / portTICK_PERIOD_MS);

    if (real_time.lostPower()) {
              strcpy(BootMessage, "RTC lost power, let's set the time!"); // When time needs to be set on a new device, or after a power loss, the
              
              real_time.adjust(DateTime(F(__DATE__), F(__TIME__)));  // This line sets the RTC with an explicit date & time, for example to set // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
              
              strcpy(BootMessage, "Clock Started and time set!");
    }
}



uint8_t Battery_Level_Counter = 3;
 
char voltage_string[10] = ""; 
double voltage = 0.00;

void checkBatt(){
 
 uint16_t readBattery = 0;
 float summation = 0.00;

  for(int i=0; i<10; i++){ ////also make the reading less jumpy
      readBattery = analogRead(BatteryPin);
      summation += float(readBattery);
  }
    summation /= 10.0f;
    //voltage = 15.00 * (summation/3095.0);
    //voltage = 14.50 * (summation/3095.0);
      voltage = 2 * 14.868 * (summation/3095.0f);

      dtostrf((voltage), -4, 3, voltage_string);
      strcat(voltage_string, "V");

      
      if(voltage <= 21.0){ // battery volktage is miserable: 3.5V on each cell
        Battery_Level_Counter = 1;
      }

      else if(voltage <= 23.0){ // battery voltage is okayish but low (3.8V on each cell)
         Battery_Level_Counter = 2;
      }

      else if(voltage <= 25.0){ // battery voltage is Good .. beyond 4V on each cell
          Battery_Level_Counter = 3;
      }

      else{ // battery voltage excellent!!! beyond 4.2V on each Li ion Cell 
        Battery_Level_Counter = 4;
      }
      /*
// PRINT READINGS //
      Serial.print("Raw Reading: "); Serial.println(readBattery);
      Serial.print("Voltage: "); Serial.println(voltage, 3);
      

   //  dtostrf(voltage, -5, 2, voltage_string);
 
  Serial.print("Voltage String 1: "); Serial.println(voltage_string);
*/


 

}





void buttonScan(){
    up1 = 0; down1 = 0;
    up2 = 0; down2 = 0;
    lock_press = false;

    //if(!critical_power_locked){

    //lock max at 5 and min at 0 ... no press beyond those limits
    if(!locked_up) {    up1 = digitalRead(up_1_pin); }
    if(!locked_down){ down1 = digitalRead(down_1_pin); }
    if(!locked_up2) {   up2 = digitalRead(up_2_pin); }
    if(!locked_down2){down2 = digitalRead(down_2_pin); }
    //}

  //universal tactile feeedback
  if(up1 || up2 || down1 || down2){ lock_press = true;  } 
  else start_counting = now_now; // // --- 1000; // --- to prevent overflow when statted at 4.2Bn ---//

      if(lock_press && !passed_yet) {start_counting = now_now; passed_yet = true;}
        if(passed_yet){
              if((now_now - start_counting) >= 50 && (now_now - start_counting) < 100) { digitalWrite(flashin_LED, HIGH);
                                                                                       beep(1,0);
                                                                                       digitalWrite(flashin_LED, LOW);
                                                                                       time_of_last_activity = now_now; //wake up from idle
                                                                                  /*   
                                                                                     Serial.println("\tButton Pressed: ");
                                                                                     Serial.print("\tup 1: ");  Serial.println(up1);
                                                                                     Serial.print("\tdown 1: "); Serial.println(down1);
                                                                                     Serial.print("\tup 2: ");   Serial.println(up2);
                                                                                     Serial.print("\tdown 2: "); Serial.println(down2); 
                                                                                   */
                                                                                     cooking_button_Processor(); // process input

                                                                                  }
       /*EXTRA SETTINGS MODE*/    //e.g. TIMER, VOLTAGE DISP, 
            if((now_now - start_counting) >= 1200 && (now_now - start_counting) < 1250) { beep(2,0); Serial.println("Long Press!"); }  
      }
}




void cooking_button_Processor(){ // LEVEL SHIFTER WITH SEMI-DYNAMIC POWER MANAGEMENT INTEGRATED
    
    if(Battery_Level_Counter < 1) {  // low-voltage cutoff: enforce shutdown if Battery_Level_Counter < 1.
         if(up1 || up2) beep(2,2); // // prevent deep discharge ..NO.. Screen.display(1, 'n'); Screen.display(2, 'o');
     }

  //COOKING ZONE 1
         if(up1 == 1 && down1 == 0) {  // only up
              if(coil_1_index < 5) { cooking_screen_set = false;  locked_up=false; locked_down=false; //free_run
                  if(Battery_Level_Counter >= 3 && coil_1_index < 5)  coil_1_index++;  //maxes out at 5:: FULL BLAST
                  if(Battery_Level_Counter == 2 && coil_1_index < 4) {coil_1_index++; if(coil_1_index == 4) locked_up = true;} //maxes out at 4
                  if(Battery_Level_Counter == 1 && coil_1_index < 3) {coil_1_index++; if(coil_1_index == 3) locked_up = true;} //maxes out at 3
                  // coil_1_index++; locked_up=false; locked_down=false;   
                }//free_run up and down
              else { // coil_1_index = 5; locked_up = true; locked_down = false; 
                    locked_up = true; locked_down=false;
                        if(Battery_Level_Counter == 1) coil_1_index = 3;  
                        if(Battery_Level_Counter == 2) coil_1_index = 4;  
                        if(Battery_Level_Counter >= 3) coil_1_index = 5;       
                        
                  }  //maxes out at 5:: FULL BLAST
              
              cooking_started = true; 
            }

            
        if(down1 == 1 && up1 == 0) {  // only down
            if(coil_1_index > 0) {coil_1_index--;  locked_down=false; locked_up=false;  cooking_screen_set = false;} 
            else { coil_1_index = 0; locked_down = true; locked_up=false; } 
            //reducing = true; 
         }


    //COOKING ZONE 2
         if(up2 == 1 && down2 == 0) {  // only up
          if(coil_2_index < 5) { coil_2_index++; locked_up2=false; locked_down2=false;  cooking_screen_2_set = false;  }//free_run up and down
          else { coil_2_index = 5; locked_up2 = true; locked_down2 = false; }  //maxes out at 5:: FULL BLAST
            
            cooking_started2 = true;
        }

        
    if(down2 == 1 && up2 == 0) {  // only down
        //  if(coil_2_index < 5) { coil_2_index--; locked_down2=false; locked_up2=false;  cooking_screen_2_set = false;  }//free_run up and down
        if(coil_2_index > 0) {coil_2_index--;  locked_down2=false; locked_up2=false;   cooking_screen_2_set = false; } 
        else { coil_2_index = 0; locked_down2 = true; locked_up2=false; } 
      //  reducing = true; 
     }
          
   
            

       update_oled_display(); 
       update_main_display();
     

    

}






bool lock_touch = false, touch_sense_passed = false;
unsigned long start_timing = 0;



const uint8_t touch_trigger = 10; //32;
// variable for storing the touch pin value 
int touchValue = 0;

 int menu_=0, back_=0, plus_=0, minus_=0;
 
void capacitive_touch(){
   menu_ = touchRead(menu_touch); delay(5);
   back_ = touchRead(back_touch); delay(5);
   plus_ = touchRead(add_touch); delay(5);
   minus_ = touchRead(sub_touch); delay(5);


  //universal tactile feeedback
  if(menu_ <= touch_trigger ||  back_ <= touch_trigger || plus_ <= touch_trigger ||  minus_ <= touch_trigger){ lock_touch = true;  } 
  else start_timing = now_now; // // --- 1000; // --- to prevent overflow when statted at 4.2Bn ---//

      if(lock_touch && !touch_sense_passed) {start_timing = now_now; touch_sense_passed = true;}

        if(touch_sense_passed){
              if((now_now - start_timing) >= 50 && (now_now - start_timing) < 100) { 
                                                  
                                                  digitalWrite(flashin_LED, HIGH); /*beep(1,0);*/ digitalWrite(flashin_LED, LOW);
                                                   time_of_last_activity = now_now; //wake up from idle
                                                                                     
                                                                                                                                                                          
                                                                           /*
                                                                                 Serial.println("----Touch Values---");
                                                                                 Serial.print("\tmenu: "); Serial.println(menu_);
                                                                                 Serial.print("\tback: "); Serial.println(back_);
                                                                                 Serial.print("\tplus: "); Serial.println(plus_);
                                                                                 Serial.print("\tminus: "); Serial.println(minus_);
                                                                             */
                                                                                                                                                                        
                                                            touchProcessor(); // process input
              }

       /*FOR EXTRA SETTINGS MODE*/    //e.g. TIMER, VOLTAGE DISP, 
            if((now_now - start_counting) >= 1200 && (now_now - start_counting) < 1250) { beep(2,0); /* Serial.println("Long Press!"); */ }  
         
    }


}

bool now_home = false;
bool setCooking_called = false;
bool cookingZone1_called = false;
bool cookingZone2_called = false;

uint8_t highlight = 1;

void touchProcessor(){ //check where we are then know where to go
  
    switch(currentScreen){
        case 1: {
            if(menu_ <= touch_trigger) currentScreen = 2; // list of cooking zones wiz Highlight
            if(back_ <= touch_trigger) { now_home = false; currentScreen = 1; } // gg back home
            break;
        }

        case 2:{
            if(menu_ <= touch_trigger) { // timer selection for CZ 1
                  if(highlight == 1) { cookingZone1_called = false; currentScreen = 3; }
                  if(highlight == 2) { cookingZone2_called = false; currentScreen = 4; }
            }
            if(back_ <= touch_trigger) { now_home = false; currentScreen = 1; } // gg back home
            if(plus_ <= touch_trigger) { setCooking_called = false; highlight = 2; }
            if(minus_ <= touch_trigger){ setCooking_called = false; highlight = 1; }
            break;
        }
        case 3: {
            if(menu_ <= touch_trigger) {  currentScreen = 4; }// timer selection for CZ 1
            if(back_ <= touch_trigger) currentScreen = 2; // go back to list of cooking zones wiz Highl
            break;
        }
       case 4: {
            if(menu_ <= touch_trigger) currentScreen = 5; // timer selection for CZ 2
            if(back_ <= touch_trigger) currentScreen = 2; // go back to list of cooking zones wiz Highl
            break;
        }

   //default: do nothing!
      
    }

    update_main_display();
}


  void update_main_display(){  
        if(currentScreen == 1) homeScreen(); 
   else if(currentScreen == 2) setCooking();      
   else if(currentScreen == 3) cookingZone1();  
   else if(currentScreen == 4) cookingZone2();  
   else if(currentScreen == 5) cookingData_all();  

  //Serial.print("Current Screen: "); Serial.println(currentScreen);
}


void setCooking(){
  if(!setCooking_called){
     //HEADER
      header();

      sub_header();
              
    //BODY     

     //select what zone to set for

    //COOKING ZONE 1
    block_for_zone_1(coil_1_index);

        //COOKING ZONE 2
    block_for_zone_2(coil_2_index);

    if(highlight == 1){ //LCD.fillRoundRect(20, 100, 120, 80, 3, ILI9341_NAVY);
       LCD.drawRoundRect(10, 65, 150, 132, 3, ILI9341_GREENYELLOW);
       LCD.drawRect(11, 66, 148, 130, ILI9341_GREENYELLOW);
       LCD.drawRoundRect(12, 67, 146, 128, 3, ILI9341_GREENYELLOW);
    }
    else if(highlight == 2){ //LCD.fillRoundRect(180, 100, 120, 75, 3, ILI9341_LIGHTGREY);
       LCD.drawRoundRect(170, 65, 150, 132, 3, ILI9341_GREENYELLOW);
       LCD.drawRect(171, 66, 148, 130, ILI9341_GREENYELLOW);
       LCD.drawRoundRect(172, 67, 146, 128, 5, ILI9341_GREENYELLOW);
   
    }

     //BOTTOM
     footer();
   

    setCooking_called = true;
  }

  
  
}


void cookingZone1(){
    if(!cookingZone1_called){
     //HEADER
      header();

      sub_header();
              
    //BODY   


     //BOTTOM
     footer();
   
      cookingZone1_called = true;
    }
}

void cookingZone2(){
   if(!cookingZone2_called){
     //HEADER
      header();

      sub_header();
              
    //BODY   


     //BOTTOM
     footer();
   
      cookingZone2_called = true;
    }
}

void cookingData_all(){
  
}

bool minute_count_passed = false;

char pad_timer[15] = ".";
void homeScreen(){

  if(!now_home){ 
    //HEADER
      header();

      sub_header();
              
    //BODY     

     //which zone is cooking and for how long 

    //COOKING ZONE 1
    block_for_zone_1(coil_1_index);

   //COOKING ZONE 2
   block_for_zone_2(coil_2_index);

     //BOTTOM
     footer();
   
    now_home = true;  
    }

//DYNAMIC DATA

//header dynamics
if(!minute_count_passed){
    running_timer();  //Serial.println(pad_timer);
    minute_count_passed = true;
}

 //body dynamics      

    if(!cooking_screen_set) {block_for_zone_1(coil_1_index);  cooking_screen_set = true;}
    if(!cooking_screen_2_set) { block_for_zone_2(coil_2_index); cooking_screen_2_set = true;}


}

void block_for_zone_1(uint8_t index){
      LCD.setCursor(45, 75);   
    LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);LCD.println("Zone 1"); 
    
    if(index <= 0){
        //LCD.fillRoundRect(20, 120, 120, 75, 3, ILI9341_GREEN); FANTASTIC GREEN
        //LCD.setCursor(40, 190); LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);  
        LCD.fillRoundRect(20, 100, 120, 80, 3, ILI9341_LIGHTGREY);

        // cooking level 1
        LCD.setCursor(60, 120); LCD.setTextSize(6); LCD.setTextColor(ILI9341_DARKGREY);  
        LCD.println(index); // coil_1_index
    }

    else {
       LCD.fillRoundRect(20, 100, 120, 80, 3, ILI9341_NAVY);
       LCD.setTextColor(ILI9341_YELLOW);  
      /*
       LCD.setCursor(50, 100); LCD.setTextSize(1);
       LCD.print(cooking_clock);
       */
        // cooking level 1
       LCD.setCursor(60, 120); LCD.setTextSize(6); 
       LCD.println(index); // coil_1_index

    }

}

void block_for_zone_2(uint8_t indeks){
     
    //LCD.setCursor(210, 190); LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);  
    LCD.setCursor(205, 80); LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);  
    LCD.println("Zone 2"); // LCD.println("Cooking Zone 2");

    if(indeks <= 0){
        LCD.fillRoundRect(180, 100, 120, 75, 3, ILI9341_LIGHTGREY);
        // cooking level 2
        LCD.setCursor(220, 120); LCD.setTextSize(6); LCD.setTextColor(ILI9341_DARKGREY);  
        LCD.println(coil_2_index);
    }

    else {
        LCD.fillRoundRect(180, 100, 120, 75, 3, ILI9341_NAVY);
        // cooking level 2
        LCD.setCursor(220, 120); LCD.setTextSize(6); LCD.setTextColor(ILI9341_YELLOW);  
        LCD.println(coil_2_index);

    }
}

uint8_t active_menu = 1;

void footer(){

    LCD.fillRect(0, 200, 320, 40, ILI9341_RED);

    if(currentScreen == 1){
        
        LCD.fillRoundRect(20, 205, 80, 30, 3, ILI9341_WHITE);
        LCD.fillRoundRect(120, 205, 80, 30, 3, ILI9341_DARKGREY);
        LCD.fillRoundRect(230, 205, 80, 30, 3, ILI9341_DARKGREY);

          //ACTIVE MENU
         LCD.setTextSize(2); 
         LCD.setTextColor(ILI9341_NAVY); LCD.setCursor(37, 215); LCD.print("Home");
         
         //THE REST OF US
         LCD.setTextColor(ILI9341_LIGHTGREY); 
         LCD.setCursor(140, 215); LCD.print("Set");
         LCD.setCursor(245, 215); LCD.print("Logs");
        
    }
  
    else if(currentScreen == 2 || currentScreen == 3 || currentScreen == 4 || currentScreen == 5){ // all these are modes for setting
        LCD.fillRoundRect(20, 212, 80, 26, 3, ILI9341_DARKGREY);
        LCD.fillRoundRect(120, 212, 80, 26, 3, ILI9341_WHITE);
        LCD.fillRoundRect(230, 212, 80, 26, 3, ILI9341_DARKGREY);

          //ACTIVE MENU
         LCD.setTextSize(2); 
         LCD.setTextColor(ILI9341_NAVY); 
         LCD.setCursor(140, 220); LCD.print("Set");
         
         
         //THE REST OF US
         LCD.setTextColor(ILI9341_LIGHTGREY); LCD.setCursor(37, 220); LCD.print("Home");
         LCD.setCursor(245, 220); LCD.print("Logs");
        
    }
  
}

int sec_ = 0, min_ = 0, hr_ = 0;

void running_timer(){
      char temp_holding[9] = "";  char temp_sec[4] = ""; char temp_min[4] = "00"; char temp_hr[4] = "00";
      
     // ltoa(now_now/1000, temp_holding, 10);

   // if(real_time not initialized) {
      if(!time_set){ //!set
      if(sec_ >= 60) { sec_ = 0; min_++; }
      if(min_ >= 60) { hr_++; }

      itoa(sec_, temp_sec, 10); itoa(min_, temp_min, 10); if(hr_ > 0) { itoa(hr_, temp_hr, 10);}


      strcpy(pad_timer, temp_hr); strcat(pad_timer, ":"); 
      if(min_ < 10) strcat(pad_timer, "0");  strcat(pad_timer, temp_min); strcat(pad_timer, ":"); 
      if(sec_ < 10) strcat(pad_timer, "0");  strcat(pad_timer, temp_sec);

}
      else strcpy(pad_timer, ShortTime);

   // Serial.print("Short Time: "); Serial.println(SystemTime);

    for(int i=260; i<320; i+=5){
        LCD.fillRect(250, 0, i, 25, ILI9341_RED); // 320p x 240p display
    }
    
    LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);
    LCD.setCursor(255, 6);   LCD.print(pad_timer);


}

void header(){ //
            // IntelliSys Logo
            //drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h);
              LCD.fillRect(0, 0, 320, 25, ILI9341_RED); // 320p x 240p display
              LCD.setTextColor(ILI9341_WHITE);  LCD.setTextSize(1);
              LCD.setCursor(2, 10);        LCD.print(currentScreen); 
             // LCD.setCursor(100, 6); LCD.setTextSize(2); LCD.print("MAKSOL 3+");
              LCD.setCursor(15, 6); LCD.setTextSize(2); LCD.print("MAKSOL 3+"); 
              LCD.setCursor(270, 210); LCD.setTextSize(1); LCD.print(voltage_string);
} 

void  sub_header(){      
            //LCD.fillRect(0, 20, 480, 239, ILI9341_DARKGREEN);
              LCD.fillRect(0, 25, 320, 80, ILI9341_WHITE); //UPPER MIDDLE CLASS
              LCD.fillRect(0, 60, 320, 170, ILI9341_BLACK);//MIDDLE CLASS
              
              LCD.setTextColor(ILI9341_BLACK);

              
              
            //  LCD.setTextColor(ILI9341_GREENYELLOW);
              
              
       if(currentScreen == 1) {
                           LCD.setTextSize(3); //LCD.setFont(ArialBlack_8_data);
                           LCD.setCursor(100, 35); LCD.print("MONITOR");
         }
        else if(currentScreen == 2){ 
                           LCD.setTextSize(2);
                           LCD.setCursor(60, 35); LCD.print("Set Your Cooking");
                                  }
                                  
        else if(currentScreen == 3){ 
                           LCD.setTextSize(2);
                           LCD.setCursor(60, 35); LCD.print("Cooking Zone 1");
                                  }

        else if(currentScreen == 4){ 
                           LCD.setTextSize(2);
                           LCD.setCursor(60, 35); LCD.print("Cooking Zone 2");
                                  }

}

char origin[100] = "This Appliance is Designed and Manufactured in Uganda";

uint32_t screen_boot_dur = 0, screen_boot_start = 0;

char boot_dur[20] = "";

void BootScreen(){ screen_boot_start = millis();
      LCD.fillScreen(ILI9341_BLACK);
      LCD.setTextColor(ILI9341_WHITE); 
      LCD.setCursor(10, 90); LCD.setTextSize(5); LCD.print("IntelliSys");

      // BACK LIGHT
      pinMode(backlight_, OUTPUT); digitalWrite(backlight_, HIGH);

      LCD.setTextColor(ILI9341_LIGHTGREY); 
      LCD.setCursor(140, 210); LCD.setTextSize(2); LCD.print("2025");

      delay(2000);
      
      LCD.fillScreen(ILI9341_NAVY);
         
      LCD.setTextColor(ILI9341_WHITE);  
      LCD.setCursor(90, 10); LCD.setTextSize(3); LCD.print("MAKSOL 3+"); 
      delay(500);


      LCD.setTextSize(2);
      LCD.setCursor(80, 60); LCD.print("Zero-Emission");
      LCD.setCursor(38, 90); LCD.print("Solar Electric Cooker");
      LCD.setCursor(75, 140);LCD.print("Made in Africa");

      delay(200);

     for(int i=0; i<=320; i+=4){
         LCD.fillRect(0, 160, i, 30, ILI9341_BLACK);
       } delay(50);

    for(int i=0; i<=320; i+=4){
       LCD.fillRect(0, 190, i, 30, ILI9341_YELLOW);
    }  delay(50);
    
    for(int i=0; i<=320; i+=4){
       LCD.fillRect(0, 220, i, 30, ILI9341_RED);
    } delay(50);
    

       
      LCD.setTextSize(1); LCD.setTextColor(ILI9341_WHITE); 
      LCD.setCursor(1, 225); LCD.print(origin);
      delay(1000);
/*
      for(int i=0; i<100; i++){
        LCD.print(origin[i]); //delay(5);
      } delay(2000);
  
*/
    /*
//green for climate action
      for(int y=0; y<=240; y+=8){ // fill screen
          LCD.fillRect(0, 40, 320, y, ILI9341_GREEN);
      }

      LCD.setTextColor(ILI9341_BLACK);  
      LCD.setCursor(25, 55); LCD.setTextSize(3); LCD.print("GUMBI COMMUNITY"); 
      delay(500);

    //LCD.setTextColor(ILI9341_WHITE); 
      LCD.setTextColor(ILI9341_NAVY);  
      LCD.setCursor(4, 110); LCD.setTextSize(2); LCD.print("Alton Climate Action Netwk"); 
      delay(500);
      LCD.setCursor(15, 155); LCD.setTextSize(2); LCD.print("For the People of Malawi"); 
      delay(1000);
      //Malawian Flag ==> black, red, green
        for(int i=0; i<=320; i+=2){
           LCD.fillRect(0, 180, i, 20, ILI9341_BLACK);
         } delay(100);

        for(int i=0; i<=320; i+=2){ //delay(50);
           LCD.fillRect(0, 200, i, 20, ILI9341_RED);
           if(i == 160+10){
            LCD.fillCircle(160, 200, 12, ILI9341_RED);
           }
           if(i == (160+20)){
            LCD.drawFastHLine(100, 199, 100, ILI9341_BLACK);
           }
         }  delay(100);
    
       for(int i=0; i<=320; i+=2){
          LCD.fillRect(0, 220, i, 20, ILI9341_DARKGREEN);
         } 

    */
  
        delay(1000);

        screen_boot_dur = (millis() - screen_boot_start)/1000;
        ltoa(screen_boot_dur, boot_dur, 10); strcat(boot_dur, " seconds");

        LCD.setTextSize(1); LCD.setTextColor(ILI9341_WHITE); 
        LCD.setCursor(20, 225); LCD.print(boot_dur);
 
/*-*/
    //  Serial.print("TFT Boot Duration: "); Serial.print(screen_boot_dur); Serial.println(" seconds");

      //  while(true){}

}


uint8_t hr = 0, mint = 0, sec = 0, day_ = 0, mth = 0, yr = 0; uint16_t mwaka = 2000; 

char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat"};
char Moonth[12][6] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};

char hr_str[7]; char min_str[7]; char sec_str[7]; char day_str[10];  char mth_str[20]; char yr_str[10]; 
char zero_holder[4] = "0"; char bucket[4]; 
char SystemDate[32] = "26 April, 2025"; char fullDate[50] = "";

char FullDate[50] = "Wednesday 26th March, 2025";
char ShortDate[24] = "1/1/25"; // long short characters
char FullTime[32] = "13:28:55";

//char SystemDate[32] = "Wed 17th Sept, 2025";; char fullDate[50] = "Wednesday 17th November, 2025";

uint8_t minute_then = 0;

uint32_t running_ticks = 0;

void query_rtc(){ // Serial.println("Time Check!"); //Serial.println();
  running_ticks++;

    if (!real_time.begin()) {
        hr = 0; mint = 0; sec = 0; // Safe default
        strcpy(SystemTime, "ERR:RTC");
        return;
      }
  //if(running_ticks%10 == 0){ //query for time once every 10 seconds

            DateTime time_now = real_time.now();

              hr = time_now.hour(); mint = time_now.minute();  sec = time_now.second();
              day_ = time_now.day(); mth = time_now.month();  mwaka = time_now.year();
        
             if(hr || mint || sec || day_) time_set = true;
        
            if(hr > 24){  // fail-safe for RTC failure: If the RTC fails, TIME DEFAULTS TO hr = 23:59:59
                // DateTime now = real_time.now();
              // while(recursive_counter < 5){ query_rtc(); recursive_counter++;}

                hr = 23; mint = 59; sec = 59; /*beep(2);*/ // Serial.println("Time Chip Failed!");

              }

              if(minute_then != mint){    minute_count_passed = false;  minute_then = mint; }
                        
            
                itoa(hr, hr_str, 10);    itoa(mint, min_str, 10); 
                itoa(sec, sec_str, 10);  
                itoa(day_, day_str, 10); strcpy(mth_str, Moonth[mth-1]);  // ... strcpy(day_str, daysOfTheWeek[day]); ... //
                itoa(mwaka, yr_str, 10); 

                char short_hand_year[5] = "22";
                itoa(mwaka%1000, short_hand_year, 10);

                  
              //construct the time sequences
        if(hr<=9){  strcpy(ShortTime, zero_holder); strcat(ShortTime, hr_str);} else strcpy(ShortTime, hr_str); strcat(ShortTime, ":"); 
        if(mint<=9){strcat(ShortTime, zero_holder);} strcat(ShortTime, min_str); // HH:MM
                    strcpy(SystemTime, ShortTime);  strcat(SystemTime, ":"); strcat(SystemTime, sec_str); // HH:MM:SS
         /*
          if(hr<12) {strcat(ShortTime, "AM");}
          else strcat(ShortTime, "PM");
          */
          //construct the date sequence
        //      strcpy(SystemDate, day_str); strcat(SystemDate, " "); strcat(SystemDate, mth_str); strcat(SystemDate, ", "); strcat(SystemDate, yr_str);
         strcpy(SystemDate, day_str); strcat(SystemDate, " "); strcat(SystemDate, mth_str); strcat(SystemDate, ", "); strcat(SystemDate, yr_str);
         strcpy(ShortDate, day_str);  strcat(ShortDate, " "); strcat(ShortDate, mth_str); strcat(ShortDate, ", "); strcat(ShortDate, short_hand_year);
//daysOfTheWeek[0]
/*
                  Serial.println(); 
                  Serial.print("Short Time: ");  Serial.println(ShortTime);
                  Serial.print("Full System Time: "); Serial.print(SystemTime); 
                  Serial.print("\tSystem Date: "); Serial.println(SystemDate); Serial.println();
 */                
      //  } once every 10 secs



}

char cooking_hr[4] = ""; char cooking_min[4] = ""; char cooking_sec[4] = "";
char cooking_hr2[4] = ""; char cooking_min2[4] = ""; char cooking_sec2[4] = "";

uint64_t ticker_timer = 0;
uint8_t run_time_secs = 0, run_time_mins = 0, run_time_hrs = 0;
uint8_t run_time_secs2 = 0, run_time_mins2 = 0, run_time_hrs2 = 0;



void beep(uint8_t times, uint8_t delay_setting){
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
          
//          if(Battery_Level_Counter <= 1) delay(100);
  //         else delay(50);

          if(delay_setting == 0)  delay(50); 
          else if(delay_setting == 1) delay(100);
          else if(delay_setting == 2) delay(500);
          
          digitalWrite(sound, LOW);
          if(times > 1) delay(50);
        }
    
   }
  

// Select I2C BUS
void LED_BUS(uint8_t bus){ //void TCA9548A(uint8_t bus)
    Wire.beginTransmission(0x70);  // TCA9548A address
    Wire.write(1 << bus);          // send byte to select bus
    Wire.endTransmission();
   // Serial.print(bus);
}


void initialize_OLEDz(){
  uint8_t attempts = 0;

  // Init OLED display on bus number 4
  LED_BUS(4);
  while(!LED.begin(i2c_Address, true)) {
      strcpy(BootMessage, "OLED 1 Adafruit_SH1106G allocation failed on OLED 1");
      attempts++;
      if(attempts >= 10) break;
  } 
  // Clear the buffer
         
      LED.setTextSize(1); LED.setTextColor(SH110X_WHITE); LED.setFont(&FreeSansBold24pt7b);

        for(int i = 5; i>=0; i--){
              LED.clearDisplay();
              LED.setCursor(50, 60); 
              LED.print(i);
              LED.display();
              delay(100);
          }   // delay(1000);

  // Init OLED display on bus number 6
  LED_BUS(6);
  if(!LED.begin(i2c_Address, true)) {
      strcpy(BootMessage, "OLED 2 Adafruit_SH1106G allocation failed on OLED 2");
     
      while(attempts < 20) {LED.begin(i2c_Address, true);  attempts++; } //break;
      
  } 
  else  strcpy(BootMessage, "OLED 2 Adafruit_SH1106G allocation successful on OLED 1");
  //  Clear the buffer
      LED.clearDisplay();

      LED.setTextSize(1); LED.setTextColor(SH110X_WHITE); LED.setFont(&FreeSansBold24pt7b);

      for(int i = 5; i>=0; i--){
            LED.clearDisplay();
            LED.setCursor(50, 60); 
            LED.print(i);
            LED.display();
            delay(100);
        }



}



const uint8_t screen_1 = 4;
const uint8_t screen_2 = 6;



uint8_t current_screen_OLED_1 = 1;
uint8_t current_screen_OLED_2 = 1;

void update_oled_display(){

  switch(current_screen_OLED_1){

        case 1: oled_homepage(1); break;
        case 2: oled_cooking_disp(); break;


  }
    
  
  switch(current_screen_OLED_2){

      case 1: oled_homepage(2); break;
      case 2: oled_cooking_disp(); break;


  }

   
  


}

void oled_cooking_disp(){

}




void oled_homepage(uint8_t which_screen){
  if(which_screen == 1){
        LED_BUS(screen_1);
 
        // HEADER
        //LED.setCursor(25, 3); LED.print("Cooking Power"); 

        LED.clearDisplay();
        LED.setTextSize(1);     LED.setTextColor(SH110X_WHITE);  
       

        if(cooking_started){
          LED.setFont(); // LED.setFont(&FreeSans9pt7b);
          LED.setCursor(40, 4); LED.print(cooking_clock);
        }

        else {
          LED.setFont();
          LED.setCursor(2, 1); LED.print(SystemDate); // LED.print(ShortDate);  
          LED.setCursor(94, 1); LED.print(ShortTime); 

        }

        //BODY
        LED.setTextSize(1); LED.setTextColor(SH110X_WHITE); LED.setFont(&FreeSansBold24pt7b);

        LED.setCursor(50, 60);        LED.println(coil_1_index);
        LED.display(); 
  }

  else if(which_screen == 2){
  // Write to OLED on bus number 3
        LED_BUS(screen_2);
        LED.clearDisplay();

              // HEADER
              if(cooking_started2){
                LED.setFont(); // LED.setFont(&FreeSans9pt7b);
                LED.setCursor(40, 4); LED.print(cooking_clock2);
              }

              else {
                LED.setFont();
                LED.setCursor(2, 1); LED.print(SystemDate); // LED.print(ShortDate);  
                LED.setCursor(94, 1); LED.print(ShortTime); 

              }

        //LED.setCursor(25, 3); LED.print("Cooking Power"); 
      
        //BODY
        LED.setTextSize(1); LED.setTextColor(SH110X_WHITE); LED.setFont(&FreeSansBold24pt7b);
        
        
        LED.setCursor(50, 60); LED.println(coil_2_index);
        
        
        LED.display(); 
      }
}




bool Coil_1_ON = false, Coil_2_ON = false;
//uint8_t coil_1_index = 0, coil_2_index = 0;
uint8_t heat_1_level = 0, heat_2_level = 0;



long interval = 0; long interval2 = 0;

uint64_t  coil_1_started = 0, coil_2_started = 0;  //up to year 200+
uint64_t full_coil_1_started = 0, full_coil_2_started = 0;
bool first_started_1 = false, first_started_2 = false;
uint8_t whichCoil = 0;

bool waiting = false; bool auto_cooling_1 = false;
bool waiting2 = false; bool auto_cooling_2 = false;

const   long wait_ko = 30000; // 30 sec constant Cooling time \\ 300,000
bool fan1_ON = false; bool fan2_ON = false;
unsigned long long fan_1_sat  = 0, fan_2_sat  = 0;
unsigned long long fan_1_stop = 0, fan_2_stop = 0;

bool fan1_speed_1_engaged = false, fan1_speed_3_engaged = false, fan1_speed_5_engaged = false; // only 3 gears for the smaller Cooking Zone

bool fan2_speed_1_engaged = false, fan2_speed_2_engaged = false, fan2_speed_3_engaged = false, fan2_speed_4_engaged = false, fan2_speed_5_engaged = false;


//revised intervals:: HALVED
const  unsigned long long interval_1 = 300000ULL; //  5 minutes (5*60*1000)
const  unsigned long long interval_2 = 600000ULL; // 10 minutes (10*60*1000)
const  unsigned long long interval_3 = 900000ULL; // 15 minutes
const  unsigned long long interval_4 = 1200000ULL; // 20 minutes
const  unsigned long long interval_5 = 1500000ULL; // 25 minutes
const  unsigned long long interval_6 = 1800000ULL; // 30 minutes


unsigned long runtime_tracker = 0;
unsigned long long fan_1_timer = 0;
unsigned long long fan_2_timer = 0;
//checking interval span is 65,000 // 65 seconds

uint32_t total_cooking_seconds = 0;


float timer_calculator = 0.00;
uint32_t zone_1_cooking_secs = 0, zone_1_cooking_mins = 0, zone_1_cooking_hrs = 0; 
char zone_1_cooking_dur[20] = "";

void monitor_cooking(){
        if(coil_1_index) { cooking_started = true; whichCoil = 1; }
        else cooking_started = false;

        if(coil_2_index) { cooking_started2 = true; whichCoil = 2; }
        else cooking_started2 = false;

        if(coil_1_index == true && coil_2_index == true) { whichCoil = 3; }
      

      if(cooking_started || cooking_started2) refresh_interval = 1000; // 1 second or less
      else refresh_interval = 10000; // 10 seconds



  if(Battery_Level_Counter < 2){ // if (voltage <= 21.0) turn_off_coils();
      if(coil_1_index) { coil_1_index = 0; beep(4, 2);}
      if(coil_2_index) { coil_2_index = 0; beep(4, 2); }

      
  }


      if(coil_1_index == 0) { 
            if(Coil_1_ON) { // turn OFF coil
                digitalWrite(Coil_1, HIGH); Coil_1_ON = false; 
                coil_1_started = 0; full_coil_1_started = 0;  first_started_1 = false;
                fan_1_stop = now_now; 
              }
            if(fan1_ON){ // whether it was in the wait or NOT, take it OFF after 5 seconds
                  if(now_now - fan_1_stop >= 5000) { // maintain forced cooling for 5-sex after shutdown
                        digitalWrite(fan_1, HIGH); 
                        beep(1,1);
                        
                        waiting = false; fan1_ON = false;
                        fan_1_stop = 0; 
                      // Serial.println("Fan 1 OFF!");
                    }
                }
      }

      if(coil_2_index == 0) { 
            if(Coil_2_ON) { // turn OFF coil
                digitalWrite(Coil_2, HIGH); Coil_2_ON = false;
                coil_2_started = 0;  full_coil_2_started = 0;  first_started_2 = false;
                fan_2_stop = now_now;  
            }
          if(fan2_ON){ // maintain forced cooling for0 5-sex after shutdown
              if(now_now - fan_2_stop >= 5000) {     
                digitalWrite(fan_2, HIGH);   
                    beep(1,1);

                    waiting2 = false; fan2_ON = false;
                    fan_2_stop = 0;
                //   Serial.println("Fan 2 OFF!");
              }
        }
        
   }



   
  if(coil_1_index != 0) { //if coil 1 toggled to 1, 2, 3, 4, 5
     if(Coil_1_ON) { 
        //    runtime_tracker = (1000 + coil_1_started + interval - now_now);
      //  uint32_t run_time_mins = runtime_tracker/60000; uint32_t run_time_secs = (runtime_tracker%60000000);

         //uint64_t ticker_timer =  (now_now - coil_1_started)/1000;
         total_cooking_seconds = (now_now - full_coil_1_started)/1000;
        
        if(total_cooking_seconds <= 60) run_time_secs = total_cooking_seconds; 
        else { run_time_secs = total_cooking_seconds%60;
       // 60, 61, ... 70, ... 90, 120, ... 1501, ...
      
          if(total_cooking_seconds > 60 && total_cooking_seconds < 3600){
              if(total_cooking_seconds%60 == 0){ // 60, 120, 180, 240 ... 3600
                run_time_mins = (total_cooking_seconds/60);
              }
          }
     /*
          if(total_cooking_seconds >= (60*60) && total_cooking_seconds <= (5*60*60)){ // 1 hr to 5 hrs
            run_time_mins = (total_cooking_seconds/60);
            run_time_hrs = run_time_mins/60;
          }
    */        
        }

    /*
        total_cooking_seconds++;
        if(total_cooking_seconds >= 60){
          total_cooking_seconds = 0;
          run_time_mins = 0;
        }
    */   
      //  if(run_time_secs >= 59) { run_time_mins++; }
        if(run_time_mins >= 60) { run_time_mins = 0; run_time_hrs++; }

         if(run_time_hrs>0) itoa(run_time_hrs, cooking_hr, 10); 
         else strcpy(cooking_hr, "00");

          itoa(run_time_mins, cooking_min, 10); 
          itoa(run_time_secs, cooking_sec, 10);
        
        strcpy(cooking_clock, cooking_hr); strcat(cooking_clock, ":"); 
        if(run_time_mins < 10) strcat(cooking_clock, "0"); strcat(cooking_clock, cooking_min); strcat(cooking_clock, ":"); 
        if(run_time_secs < 10) strcat(cooking_clock, "0"); strcat(cooking_clock, cooking_sec);

  //      Serial.println(); Serial.print("Coil "); Serial.print(whichCoil); Serial.print(" running for: "); 
        
      //  Serial.print(run_time_mins); Serial.print(":");  Serial.print(run_time_secs);Serial.print(" Minutes");
     //     Serial.println(cooking_clock);

   //     Serial.print(" (Total: "); Serial.print(total_cooking_seconds); Serial.println(" Seconds)"); Serial.println();


/*
      uint64_t zone_1_cooking_interval = interval_1;
      zone_1_cooking_secs = zone_1_cooking_interval / (1000ULL); Serial.print("Cooking Secs: "); Serial.println(zone_1_cooking_secs);
      zone_1_cooking_mins = zone_1_cooking_interval / (60*1000ULL); Serial.print("Cooking Mins: "); Serial.println(zone_1_cooking_mins);
      uint16_t count_down_mins = (now_now / (60*1000ULL)) - zone_1_cooking_mins; 
      uint16_t count_down_secs = (now_now / 1000) - zone_1_cooking_secs; Serial.print("Count down Secs: "); Serial.println(zone_1_cooking_secs);

      char temp_buck_wild[5] = "";


      itoa(count_down_mins, temp_buck_wild, 10); strcpy(zone_1_cooking_dur, temp_buck_wild); // minutes
      strcat(zone_1_cooking_dur, ":");
      itoa(count_down_secs, temp_buck_wild, 10); strcat(zone_1_cooking_dur, temp_buck_wild);

      Serial.print("Cooking Timer: "); Serial.println(zone_1_cooking_dur);
*/

        if(!waiting){//turn it on instantly but very silently... for the first 60 seconds

 /*FAN TOGOLA */   if(!fan1_ON) { // 30 seconds
                              digitalWrite(fan_1, LOW); fan1_ON = true; //  Serial.print("Fan 1 Now ON at: "); Serial.println(now_now);  
                             }
                   if(fan1_ON)  { fan_1_timer = (uint64_t)(now_now - coil_1_started); 
                          //    Serial.print("\tFan 1 Timer: "); Serial.println(fan_1_timer/1000); 
                        } //if(fan1_ON)
                  } //if(!waiting)
       // else if(waiting){}
    }


  else if(!Coil_1_ON) { //check if it hasn't alreay been turned ON
       if(!waiting) { //turn it ON if it is not in waiting state

           digitalWrite(Coil_1, LOW); Coil_1_ON = true; coil_1_started = now_now;  ticker_timer = coil_1_started;
           auto_cooling_1 = false; if(!first_started_1)  { first_started_1 = true; full_coil_1_started = now_now; }
           
      //     Serial.print("Coil "); Serial.print(whichCoil); Serial.println("FIRED !!!"); 
       //    Serial.print("At: "); Serial.println(coil_1_started/1000);
           }
       else if(waiting){ //full_coil_1_started = now_now-wait_ko;
         if(!auto_cooling_1) {beep(1,0); auto_cooling_1 = true;}
         //  Serial.print("\tWaiting for ... "); Serial.print(((1000 + coil_1_started + interval + wait_ko) - now)/1000); Serial.println(" seconds");
       }
  }

 switch(coil_1_index){ 
     case 1: { interval = interval_1; running(interval);   }
           break;

     case 2: { interval = interval_2; running(interval);  } 
             break;

     case 3: { interval = interval_3; running(interval);  }  
             break;

     case 4: { interval = interval_4; running(interval);  } 
             break;
     
     case 5: { interval = interval_5; running(interval);  }    
             break;
   
   }

}//closing if coil 1 is not zero


if(coil_2_index != 0) {  //if toggled to a non zero integer

if(Coil_2_ON) { //check if it hasn't alreay been turned ON

        uint32_t total_cooking_seconds2 = (now_now - full_coil_2_started)/1000;
              
        if(total_cooking_seconds2 <= 60) run_time_secs2 = total_cooking_seconds2; 
        else { run_time_secs2 = total_cooking_seconds2%60;
      // 60, 61, ... 70, ... 90, 120, ... 1501, ...

          if(total_cooking_seconds2 > 60 && total_cooking_seconds2 < 3600){
              if(total_cooking_seconds2%60 == 0){ // 60, 120, 180, 240 ... 3600
                run_time_mins2 = (total_cooking_seconds2/60);
              }
          }
      /*
          if(total_cooking_seconds >= (3600) && total_cooking_seconds <= (6*3600)){ // 1 hr to 6 hrs (21,600secs)
            run_time_mins = (total_cooking_seconds/60);
            run_time_hrs = run_time_mins/60;
          }
      */        
        }
      
      //  if(run_time_secs >= 59) { run_time_mins++; }
        if(run_time_mins2 >= 60) { run_time_mins2 = 0; run_time_hrs2++; }

        if(run_time_hrs2>0) itoa(run_time_hrs2, cooking_hr2, 10); 
        else strcpy(cooking_hr2, "00");

          itoa(run_time_mins2, cooking_min2, 10); 
          itoa(run_time_secs2, cooking_sec2, 10);
        
        strcpy(cooking_clock2, cooking_hr2); strcat(cooking_clock2, ":"); 
        if(run_time_mins2 < 10) strcat(cooking_clock2, "0"); strcat(cooking_clock2, cooking_min); strcat(cooking_clock2, ":"); 
        if(run_time_secs2 < 10) strcat(cooking_clock2, "0"); strcat(cooking_clock2, cooking_sec2);

     //   Serial.println(); Serial.print("Coil "); Serial.print(whichCoil); Serial.print(" running for: "); 
        
      //  Serial.print(run_time_mins); Serial.print(":");  Serial.print(run_time_secs);Serial.print(" Minutes");
       //   Serial.println(cooking_clock2);

    //    Serial.print(" (Total: "); Serial.print(total_cooking_seconds2); Serial.println(" Seconds)"); Serial.println();


     if(!waiting2){//turn it on instantly but very silently... for the first FULL minute || till 1:00
/*GEAR 1*/          if(!fan2_ON) {   /*Serial.print(F("Fan 2 Speed: ")); /*Serial.println(F(Speed_1));*/ //
                                 digitalWrite(fan_2, LOW); fan2_ON = true;  //  Serial.print("Fan 2 on AT: "); Serial.println(now_now);  
                                 }
                   if(fan2_ON){ //only one JAB
                         fan_2_timer = (uint64_t)(now_now - coil_2_started); // fan_2_timer = now - coil_2_started;
                     //    Serial.print("\tFan 2 Timer: "); Serial.println(fan_2_timer/1000); 
                   } //if(fan2_ON)
                 } //if(!waiting2)
       else if(waiting2){ }
     
   } //if(Coil_2 coil 2 is already on and running...


  else if(!Coil_2_ON) { 

     if(waiting2){ //thrown into WAIT LOOP
           //Serial.print("\tThrown into wait loop ... "); Serial.print(((1000 + coil_1_started + interval + wait_ko) - now)/1000); Serial.println(" seconds");
         if(!auto_cooling_2) {beep(1,0); auto_cooling_2 = true;
        
        }
       }
   

     else if(!waiting) { //turn it ON if it is not in waiting state

             digitalWrite(Coil_2, LOW); Coil_2_ON = true;  coil_2_started = now_now;  
             auto_cooling_2 = false;  if(!first_started_2)  { first_started_2 = true; full_coil_2_started = now_now; }
       //    Serial.print("Coil "); Serial.print(whichCoil); Serial.println(" FIRED !!!"); 
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




}


bool is_togod = false;
uint16_t blink_counta = 0; // RUNTIME INDICATOR...

void running(uint64_t intervo){ // uint8_t COIL_NO

   // Serial.print("Running Time: "); Serial.println(intervo); 

              if((now_now - coil_1_started) < interval) {//((now - coil_1_started) < interval) //Serial.print("Coil "); Serial.print(whichCoil); Serial.println(" ON");
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

    

              if(now_now - coil_2_started < interval2) { // (now - coil_2_started < interval2) /*Serial.print("Coil "); Serial.print(whichCoil); Serial.println(" ON"); */
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
                       else {  /*Serial.print("Waiting For: "); Serial.println(((interval2 + coil_2_started + wait_ko) - now_now)/1000);*/
                        //if(!auto_cooling_2){ beep(1); auto_cooling_2 = true;}
                        }
                    }
                    else { waiting2 = false; /*Serial.println("Waiting Time Over - !");*/ } // --- WAITING TIME OVER 
        
    }


}



   
   