// MAKSOL 3+ CENTRAL UI
// After a long iterative battle from Mid April 2025 to mid-June 2025
// TFT LCD as Central Screen to Log cooking info
//3 or 4 BUTTONS to control up to only 5 screens

#include "RTClib.h"
RTC_DS3231 real_time;

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"


// --- TOUCH INPUTS

const uint8_t menu_touch = 32;   // TOUCH 9
const uint8_t back_touch = 33;  // TOUCH 8
const uint8_t add_touch  = 27;   // TOUCH 7
const uint8_t sub_touch = 14;//  TOUCH 6


const uint8_t up_1 = 36;//5;   // up 1
const uint8_t down_1 = 39;//19;//23;//36; //27; //15; // down 1
const uint8_t up_2 = 35;//4; //13; // up 2
const uint8_t down_2 = 34;//21;//22;//39; //16;// down 2



int vibrator = 17;
int buzzer   = 25; // 16;




// For the Adafruit shield, these are the default.
const uint8_t TFT_DC = 15;
const uint8_t TFT_CS = 5; //39; //5;//39;
const uint8_t RST = 13;      // 4;

const uint8_t TFT_MOSI = 23;
const uint8_t TFT_MISO = 19;
const uint8_t TFT_CLK = 18;

const uint8_t backlight_ = 4;


 //  If using the breakout, change pins as desired
 //  Adafruit_ILI9341 LCD = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, RST, TFT_MISO); //12X slower
     Adafruit_ILI9341 LCD(TFT_CS, TFT_DC, RST); // HARDWARE SPI

uint8_t currentScreen = 1;

uint32_t duration  = 0; float boot_duration = 0.00;
int64_t now_now = 0, prev_prev = 0, boot_start = 0; 


//FUNCTION PROTOTYPING
void BootScreen();
void Sleep();

uint8_t boot_level = 0;
char BootMessage[200] = ".";

void setup() {
          boot_start = esp_timer_get_time();
          Serial.begin(115200);

  
          LCD.begin();
          LCD.setRotation(3); // for the 2.4" screen
          BootScreen();
      
      
     // Initilize WiFi or Embolo Tuusi
  //      WiFi.mode(WIFI_STA);

      
      //STEP 5 
      strcpy(BootMessage, "Initializing Realtime Clock"); Serial.println(BootMessage);
      initialize_RTC();
      Serial.println(BootMessage); delay(1000);
      query_rtc();
          
      Serial.println("Done Booting!"); Serial.println();
      now_now = esp_timer_get_time();
      boot_duration = (now_now - boot_start)/1000000.0;
      Serial.print("TOTAL Boot Duration: "); Serial.print(boot_duration, 2); Serial.println(" seconds");
 

        
}

void initialize_RTC(){
  uint8_t trial = 0;
  while(!real_time.begin()) {
         strcpy(BootMessage, "Couldn't find RTC");   Serial.flush();
         trial++; delay(50);
      if (trial >= 3)break;  
    } //vTaskDelay(500 / portTICK_PERIOD_MS);

    if (real_time.lostPower()) {
              strcpy(BootMessage, "RTC lost power, let's set the time!"); // When time needs to be set on a new device, or after a power loss, the
              
              real_time.adjust(DateTime(F(__DATE__), F(__TIME__)));  // This line sets the RTC with an explicit date & time, for example to set // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
              
              strcpy(BootMessage, "Clock Started and time set!");
    }
}

 uint8_t heat_1_index = 5;
 uint8_t heat_2_index = 0;

uint8_t ticker_timer = 0, time_stamp = 5; bool time_set = false;

char SystemTime[50] = "13:45:21"; char ShortTime[32] = "13:45"; 

void loop() {
   now_now = esp_timer_get_time()/1000ULL;
   
   if(now_now - prev_prev >= 200){ //10X each second
      //read buttons
        capacitive_touch();
        ticker_timer++;

      if(ticker_timer >= 10){ // every second
        updateDisplay();
        
        ticker_timer = 0;
        
        time_stamp++;
       if(time_stamp >= 5){ // every 5 seconds
          query_rtc();
          time_stamp = 0;
        }       
      }



      prev_prev = now_now;
   }

}

const int touch_trigger = 32;
// variable for storing the touch pin value 
int touchValue = 0;

 int menu_=0, back_=0, plus_=0, minus_=0;
 
void capacitive_touch(){
   menu_ = touchRead(menu_touch); delay(5);
   back_ = touchRead(back_touch); delay(5);
   plus_ = touchRead(add_touch); delay(5);
   minus_ = touchRead(sub_touch); delay(5);

/*
  Serial.println("----Touch Values---");
    Serial.print("\tmenu: "); Serial.println(menu_);
    Serial.print("\tback: "); Serial.println(back_);
    Serial.print("\tplus: "); Serial.println(plus_);
    Serial.print("\tminus: "); Serial.println(minus_);
*/

  if(menu_ <= touch_trigger ||  back_ <= touch_trigger ||
     plus_ <= touch_trigger ||  minus_ <= touch_trigger){
         touchProcessor();
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

    updateDisplay();
}


  void updateDisplay(){  
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
    block_for_zone_1(heat_1_index);

        //COOKING ZONE 2
    block_for_zone_2(heat_2_index);

    if(highlight == 1){ //LCD.fillRoundRect(20, 100, 120, 80, 3, ILI9341_NAVY);
       LCD.drawRoundRect(10, 65, 150, 140, 3, ILI9341_GREENYELLOW);
       LCD.drawRect(11, 66, 148, 138, ILI9341_GREENYELLOW);
       LCD.drawRoundRect(12, 67, 146, 136, 3, ILI9341_GREENYELLOW);
    }
    else if(highlight == 2){ //LCD.fillRoundRect(180, 100, 120, 75, 3, ILI9341_LIGHTGREY);
       LCD.drawRoundRect(170, 65, 150, 140, 3, ILI9341_GREENYELLOW);
       LCD.drawRect(171, 66, 148, 138, ILI9341_GREENYELLOW);
       LCD.drawRoundRect(172, 67, 146, 136, 5, ILI9341_GREENYELLOW);
   
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


char pad_timer[15] = ".";
void homeScreen(){

  if(!now_home){ 
    //HEADER
      header();

      sub_header();
              
    //BODY     

     //which zone is cooking and for how long 

    //COOKING ZONE 1
    block_for_zone_1(heat_1_index);

   //COOKING ZONE 2
   block_for_zone_2(heat_2_index);

     //BOTTOM
     footer();
   
    now_home = true;  
    }

//DYNAMIC DATA

//header dynamics
    running_timer();  //Serial.println(pad_timer);
    LCD.fillRect(260, 0, 320, 25, ILI9341_RED); // 320p x 240p display
    LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);
    LCD.setCursor(250, 6);   LCD.print(pad_timer);

 //body dynamics      




}

void block_for_zone_1(uint8_t index){
      LCD.setCursor(45, 75);   
    LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);LCD.println("Zone 1"); 
    
    //LCD.fillRoundRect(20, 120, 120, 75, 3, ILI9341_GREEN); FANTASTIC GREEN
    //LCD.setCursor(40, 190); LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);  
    LCD.fillRoundRect(20, 100, 120, 80, 3, ILI9341_NAVY);

    // cooking level 1
    LCD.setCursor(60, 120); LCD.setTextSize(6); LCD.setTextColor(ILI9341_YELLOW);  
    LCD.println(index); // heat_1_index

}

void block_for_zone_2(uint8_t indeks){
      LCD.fillRoundRect(180, 100, 120, 75, 3, ILI9341_LIGHTGREY);
    //LCD.setCursor(210, 190); LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);  
    LCD.setCursor(205, 80); LCD.setTextSize(2); LCD.setTextColor(ILI9341_WHITE);  
    LCD.println("Zone 2"); // LCD.println("Cooking Zone 2");

     // cooking level 2
    LCD.setCursor(220, 120); LCD.setTextSize(6); LCD.setTextColor(ILI9341_DARKGREY);  
    LCD.println(heat_2_index);
}

uint8_t active_menu = 1;

void footer(){

    if(currentScreen == 1){
        LCD.fillRoundRect(0, 212, 80, 26, 3, ILI9341_WHITE);
        LCD.fillRoundRect(110, 212, 80, 26, 3, ILI9341_DARKGREY);
        LCD.fillRoundRect(220, 212, 80, 26, 3, ILI9341_DARKGREY);

          //ACTIVE MENU
         LCD.setTextSize(2); 
         LCD.setTextColor(ILI9341_NAVY); LCD.setCursor(17, 220); LCD.print("Home");
         
         //THE REST OF US
         LCD.setTextColor(ILI9341_LIGHTGREY); 
         LCD.setCursor(130, 220); LCD.print("Set");
         LCD.setCursor(240, 220); LCD.print("Logs");
        
    }
  
    else if(currentScreen == 2){
        LCD.fillRoundRect(0, 212, 80, 26, 3, ILI9341_DARKGREY);
        LCD.fillRoundRect(110, 212, 80, 26, 3, ILI9341_WHITE);
        LCD.fillRoundRect(220, 212, 80, 26, 3, ILI9341_DARKGREY);

          //ACTIVE MENU
         LCD.setTextSize(2); 
         LCD.setTextColor(ILI9341_NAVY); 
         LCD.setCursor(130, 220); LCD.print("Set");
         
         
         //THE REST OF US
         LCD.setTextColor(ILI9341_LIGHTGREY); LCD.setCursor(17, 220); LCD.print("Home");
         LCD.setCursor(240, 220); LCD.print("Logs");
        
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

}

void header(){ //
            // IntelliSys Logo
            //drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h);
              LCD.fillRect(0, 0, 320, 25, ILI9341_RED); // 320p x 240p display
              LCD.setTextColor(ILI9341_WHITE);  LCD.setTextSize(1);
              LCD.setCursor(2, 10);        LCD.print(currentScreen); 
             // LCD.setCursor(100, 6); LCD.setTextSize(2); LCD.print("MAKSOL 3+");
              LCD.setCursor(15, 6); LCD.setTextSize(2); LCD.print("MAKSOL 3+"); 
          //    LCD.setCursor(270, 10); LCD.setTextSize(1); LCD.print(now_now/1000);
} 

void  sub_header(){      
            //LCD.fillRect(0, 20, 480, 239, ILI9341_DARKGREEN);
              LCD.fillRect(0, 25, 320, 80, ILI9341_WHITE); //UPPER MIDDLE CLASS
              LCD.fillRect(0, 60, 320, 170, ILI9341_BLACK);//MIDDLE CLASS
              LCD.fillRect(0, 210, 320, 30, ILI9341_RED);
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

void BootScreen(){
      LCD.fillScreen(ILI9341_BLACK);
      LCD.setTextColor(ILI9341_WHITE); 
      LCD.setCursor(10, 90); LCD.setTextSize(5); LCD.print("IntelliSys");

      // BACK LIGHT
         pinMode(backlight_, OUTPUT); digitalWrite(backlight_, HIGH);

      LCD.setTextColor(ILI9341_LIGHTGREY); 
      LCD.setCursor(140, 210); LCD.setTextSize(2); LCD.print("2025");

      delay(1000);
      
      LCD.fillScreen(ILI9341_NAVY);
         
      LCD.setTextColor(ILI9341_WHITE);  
      LCD.setCursor(90, 10); LCD.setTextSize(3); LCD.print("MAKSOL 3+"); 
      delay(1000);


      LCD.setTextSize(2);
     LCD.setCursor(80, 60); LCD.print("Zero-Emission");
     LCD.setCursor(38, 90); LCD.print("Solar Electric Cooker");
     LCD.setCursor(75, 140);LCD.print("Made in Africa");

      delay(200);

     for(int i=0; i<=320; i+=2){
        LCD.fillRect(0, 160, i, 30, ILI9341_BLACK);
       } delay(100);

    for(int i=0; i<=320; i+=2){
      LCD.fillRect(0, 190, i, 30, ILI9341_YELLOW);
    }  delay(100);
    
    for(int i=0; i<=320; i+=2){
      LCD.fillRect(0, 220, i, 30, ILI9341_RED);
    } delay(500);
    

       
      LCD.setTextSize(1); LCD.setTextColor(ILI9341_WHITE); 
      LCD.setCursor(1, 225); LCD.print(origin);
      delay(1000);
/*
      for(int i=0; i<100; i++){
        LCD.print(origin[i]); //delay(5);
      } delay(2000);
  
*/
    
//green for climate action
      for(int y=0; y<=240; y+=4){ // fill screen
          LCD.fillRect(0, 40, 320, y, ILI9341_GREEN);
      }

      LCD.setTextColor(ILI9341_BLACK);  
      LCD.setCursor(25, 55); LCD.setTextSize(3); LCD.print("GUMBI COMMUNITY"); 
      delay(1000);

    //LCD.setTextColor(ILI9341_WHITE); 
      LCD.setTextColor(ILI9341_NAVY);  
      LCD.setCursor(4, 110); LCD.setTextSize(2); LCD.print("Alton Climate Action Netwk"); 
      delay(1000);
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
    
       for(int i=0; i<=320; i++){
          LCD.fillRect(0, 220, i, 20, ILI9341_DARKGREEN);
         } 
    
  
      delay(2000);

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


uint32_t running_ticks = 0;

void query_rtc(){ Serial.println("Time Check!"); Serial.println();
  running_ticks++;

  //if(running_ticks%10 == 0){ //query for time once every 10 seconds

            DateTime time_now = real_time.now();

              hr = time_now.hour(); mint = time_now.minute();  sec = time_now.second();
              day_ = time_now.day(); mth = time_now.month();  mwaka = time_now.year();
        
             if(hr || mint || sec || day_) time_set = true;
        
            if(hr > 24){ 
                // DateTime now = real_time.now();
              // while(recursive_counter < 5){ query_rtc(); recursive_counter++;}

                hr = 24; mint = 59; sec = 59; /*beep(2);*/ Serial.println("Time Chip Failed!");

              }
            
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
                  Serial.println(); 
                  Serial.print("Short Time: ");  Serial.println(ShortTime);
                  Serial.print("Full System Time: "); Serial.print(SystemTime); 
                  Serial.print("\tSystem Date: "); Serial.println(SystemDate); Serial.println();
                  Serial.println();
      //  } once every 10 secs



}



   
   
