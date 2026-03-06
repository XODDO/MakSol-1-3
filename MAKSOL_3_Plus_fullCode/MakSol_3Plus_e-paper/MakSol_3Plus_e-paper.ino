
// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>

//FONTS
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h> // FreeSerifBold24pt7b
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// ESP32 CS(SS)=5,SCL(SCK)=18,SDA(MOSI)=23,BUSY=15,RES(RST)=2,DC=17


// 1.54'' EPD Module
//GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=5*/ 5, /*DC=*/ 4, /*RES=*/ 13, /*BUSY=*/ 15)); // GDEH0154D67 200x200, SSD1681

// 2.9'' EPD Module
    GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> e_paper(GxEPD2_290_BS(/*CS=5*/ 5, /*DC=*/ 4, /*RES=*/ 13, /*BUSY=*/ 15)); // DEPG0290BS 128x296, SSD1680
//  GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> e_paper(GxEPD2_290_C90c(/*CS=5*/ 5, /*DC=*/ 17, /*RES=*/ 2, /*BUSY=*/ 15)); // GDEM029C90 128x296, SSD1680

 //4.2'' EPD Module
 // GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=5*/ 5, /*DC=*/ 17, /*RES=*/ 16, /*BUSY=*/ 4)); // 400x300, SSD1683

  void helloFullScreenPartialMode();
  void showPartialUpdate();
  void helloWorld();

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G LCD = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#include "RTClib.h"

  RTC_DS3231 real_time;


// Select I2C BUS
void LCD_BUS(uint8_t bus){ //void TCA9548A(uint8_t bus)
      Wire.beginTransmission(0x70);  // TCA9548A address
      Wire.write(1 << bus);          // send byte to select bus
      Wire.endTransmission();
      Serial.print(bus);
}
 
 uint currentScreen = 1;

uint8_t up1 = 39, down1 = 36;
uint8_t up2 = 35, down2 = 34;

void setup(){ delay(500);

  Serial.begin(115200);

  // Start I2C communication with the Multiplexer
  Wire.begin();

pinMode(up1, INPUT); pinMode(up2, INPUT); pinMode(down1, INPUT); pinMode(down2, INPUT); 

uint8_t attempts = 0;

  // Init OLED display on bus number 4
  LCD_BUS(4);
  while(!LCD.begin(i2c_Address, true)) {
      Serial.println(F("OLED 1 Adafruit_SH1106G allocation failed"));
      attempts++;
      if(attempts >= 10) break;
  } 
  // Clear the buffer
  LCD.clearDisplay();



  // Init OLED display on bus number 6
  LCD_BUS(6);
  while(!LCD.begin(i2c_Address, true)) {
      Serial.println(F("OLED 2 Adafruit_SH1106G allocation failed"));
      attempts++;
      if(attempts >= 20) break;
  } 
  // Clear the buffer
  LCD.clearDisplay();


      oled_homepage();
 


// NOW TO THE E-PAPER

  e_paper.init(115200,true,50,false);
  e_paper.setRotation(3);


  Serial.print("Starting Realtime Clock...");

    uint8_t trial = 0;
    while(!real_time.begin()) {
           Serial.println("Couldn't find RTC");
           Serial.flush();
           trial++; delay(50);
        if (trial >= 3)break;  
      } //vTaskDelay(500 / portTICK_PERIOD_MS);

      if (real_time.lostPower()) {
            Serial.println("RTC lost power, let's set the time!");
            // When time needs to be set on a new device, or after a power loss, the
            // following line sets the RTC to the date & time this sketch was compiled
            real_time.adjust(DateTime(F(__DATE__), F(__TIME__)));
            // This line sets the RTC with an explicit date & time, for example to set
            // January 21, 2014 at 3am you would call:
            // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
            Serial.println("Clock Started!");
      }

      query_rtc();

      
   update_display();

/*

    helloFullScreenPartialMode();
    delay(1000);
    if (e_paper.epd2.hasFastPartialUpdate){
            showPartialUpdate();
            delay(1000);
      }

  */  
  e_paper.hibernate();


  Serial.println("Booted!");

}


uint8_t hr = 0, mint = 0, sec = 0, day_ = 0, mth = 0, yr = 0; uint16_t mwaka = 2000; 

char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat"};
char Moonth[12][6] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};

char hr_str[7]; char min_str[7]; char sec_str[7]; char day_str[10];  char mth_str[20]; char yr_str[10]; 
char zero_holder[4] = "0"; char bucket[4]; 
char SystemTime[50] = "13:45:21"; char ShortTime[32] = "13:45"; 
char SystemDate[32] = "Thur 26 Sept, 2025"; char fullDate[50] = "";

char FullDate[50] = "Wednesday 26th March, 2025";
char ShortDate[24] = "Thur 26 Sept, 2025"; // long short characters
char FullTime[32] = "13:28:55";

//char SystemDate[32] = "Wed 17th Sept, 2025";; char fullDate[50] = "Wednesday 17th November, 2025";





uint32_t running_ticks = 0;

void query_rtc(){ //Serial.println("Time Check!"); Serial.println();
  running_ticks++;

  if(running_ticks%10 == 0){ //query for time once every 10 seconds

            DateTime time_now = real_time.now();

              hr = time_now.hour(); mint = time_now.minute();  sec = time_now.second();
              day_ = time_now.day(); mth = time_now.month();  mwaka = time_now.year();

            if(hr > 24){ 
                // DateTime now = real_time.now();
              // while(recursive_counter < 5){ query_rtc(); recursive_counter++;}

                hr = 24; mint = 59; sec = 59; /*beep(2);*/ Serial.println("Time Chip Failed!");

              }
            
                itoa(hr, hr_str, 10);    itoa(mint, min_str, 10); 
                itoa(sec, sec_str, 10);  
                itoa(day_, day_str, 10); strcpy(mth_str, Moonth[mth-1]);  // ... strcpy(day_str, daysOfTheWeek[day]); ... //
                itoa(mwaka, yr_str, 10);

                  
              //construct the time sequences
        if(hr<=9){  strcpy(ShortTime, zero_holder); strcat(ShortTime, hr_str);} else strcpy(ShortTime, hr_str); strcat(ShortTime, ":"); 
        if(mint<=9){strcat(ShortTime, zero_holder);} strcat(ShortTime, min_str); // HH:MM
                    strcpy(SystemTime, ShortTime);  strcat(SystemTime, ":"); strcat(SystemTime, sec_str); // HH:MM:SS

          //construct the date sequence
        //      strcpy(SystemDate, day_str); strcat(SystemDate, " "); strcat(SystemDate, mth_str); strcat(SystemDate, ", "); strcat(SystemDate, yr_str);
         strcpy(SystemDate, day_str); strcat(SystemDate, " "); strcat(SystemDate, mth_str); strcat(SystemDate, ", "); strcat(SystemDate, yr_str);

                  Serial.println(); 
                  Serial.print("Short Time: ");  Serial.println(ShortTime);
                  Serial.print("Full System Time: "); Serial.print(SystemTime); 
                  Serial.print("\tSystem Date: "); Serial.println(SystemDate); Serial.println();
                  Serial.println();
        }



}



void oled_homepage(){
      LCD_BUS(4);
      LCD.clearDisplay();
      LCD.setTextSize(1);
      LCD.setTextColor(SH110X_WHITE); 

            // HEADER
      LCD.setCursor(25, 3); LCD.print("Cooking Power"); 

            //BODY
      LCD.setTextSize(1);
      LCD.setTextColor(SH110X_WHITE);
      LCD.setFont(&FreeSansBold24pt7b);
      LCD.setCursor(45, 55);
      LCD.println("4");
      LCD.display(); 

      // Write to OLED on bus number 3
      LCD_BUS(6);
      LCD.clearDisplay();
      LCD.setTextSize(1);
      LCD.setTextColor(SH110X_WHITE); 

            // HEADER
       LCD.setFont();
      LCD.setCursor(25, 3); LCD.print("Cooking Power"); 

            //BODY
      LCD.setTextSize(1.5);
      LCD.setTextColor(SH110X_WHITE);
      LCD.setCursor(45, 55);
      LCD.setFont(&FreeSansBold24pt7b);
      LCD.println("2");
      LCD.display(); 
}


const char HelloWorld[] = "MakSol Cooker";
const char HelloWeACtStudio[] = "IntelliSys";

const char Progress[20] = "Cooking Progress";



uint16_t counter = 0;

void loop() {

//  buttonScan();

   delay(100);

  if(counter%30 == 0) { query_rtc(); currentScreen = 2;   update_display();  Serial.print("Current Screen"); Serial.println(currentScreen); } // 2 SECONDS

   counter++;

}



void buttonScan(){
  int up_1 = digitalRead(up1);
  int up_2 = digitalRead(up2);
  int dwn_1 = digitalRead(down1);
  int dwn_2 = digitalRead(down2);

  Serial.print("UP  1: "); Serial.println(up_1);
  Serial.print("DWN 1: "); Serial.println(dwn_1);
  Serial.print("UP  2: "); Serial.println(up_2);
  Serial.print("DWN 2: "); Serial.println(dwn_2); 
  Serial.println();
  
}


char cooking_1_str[5] = "4";
char cooking_2_str[5] = "2";

void update_display(){
  if(currentScreen == 1) ep_homePage();
  else if(currentScreen == 2) dataScreen();
}


uint16_t box_width = 120;
uint16_t box_height = 80;


void ep_homePage(){ Serial.println("E-PAPER SUMMONED!");
   e_paper.setFont(&FreeMonoBold9pt7b);
   e_paper.setTextColor(GxEPD_BLACK);
   e_paper.fillScreen(GxEPD_WHITE);

   e_paper.setFont(&FreeMonoBold9pt7b); e_paper.setTextColor(GxEPD_BLACK);
   int16_t tbx=0, tby=0; uint16_t tbw=0, tbh=0;
   e_paper.getTextBounds(Progress, 0, 0, &tbx, &tby, &tbw, &tbh);

  uint16_t xx = ((e_paper.width() - tbw) / 2) - tbx;

uint32_t e_paper_cycle = 0;


  Serial.println("ENTERING LOOP...");
  e_paper.firstPage();
    do{ 
      Serial.print(e_paper_cycle);
  
   //HEADER
     e_paper.fillRect(0, 0, 296, 15, GxEPD_BLACK);
     e_paper.setTextColor(GxEPD_WHITE);
       
   //DATE
     e_paper.setFont(&FreeSans9pt7b);
     e_paper.setCursor(2, 12);  e_paper.print(SystemDate); 
   //TIME
   //e_paper.setFont(&FreeSans12pt7b); //FreeMono12pt7b
     e_paper.setFont(&FreeMonoBold9pt7b);
     e_paper.setCursor(230, 12); e_paper.print(ShortTime);


    //BODY
    //BODY TITLE
        e_paper.setFont(&FreeMonoBold12pt7b); e_paper.setTextColor(GxEPD_BLACK);
        e_paper.setCursor(30, 35);    e_paper.print(Progress);

    //COOKING ZONE 1
        e_paper.drawRoundRect(10, 45, box_width, box_height, 10, GxEPD_RED);

        e_paper.fillRoundRect(10, 45, box_width, 20, 10, GxEPD_RED); //BLACK RIBBON
        e_paper.fillRect(10, 55, box_width, 12, GxEPD_RED);

         e_paper.setTextColor(GxEPD_WHITE); e_paper.setFont(&FreeMonoBold9pt7b);
         e_paper.setCursor(35, 60); e_paper.print("Zone 2"); //AREA HEADING


    //COOKING ZONE 2
        e_paper.drawRoundRect((30+box_width), 45, box_width, box_height, 10, GxEPD_RED);
        e_paper.fillRoundRect((30+box_width), 45, box_width, 20, 10, GxEPD_RED); //BLACK RIBBON
        e_paper.fillRect((30+box_width), 55, box_width, 12, GxEPD_RED);

        e_paper.setTextColor(GxEPD_WHITE); e_paper.setFont(&FreeMonoBold9pt7b);
        e_paper.setCursor(25+(30+box_width), 60); e_paper.print("Zone 1"); //AREA HEADING


         LCD.setTextColor(GxEPD_RED);
         LCD.setFont(&FreeSansBold24pt7b); 
         LCD.setCursor(35, 60);    LCD.print(cooking_1_str);

         //LCD.setFont(&FreeMono9pt7b); //FreeMono9pt7b
         //LCD.setCursor(90, 130); LCD.print(WindSpeed_units);
  
    e_paper_cycle++;

  }while(e_paper.nextPage());

  Serial.println("LOOP EXITED!");
  Serial.println(e_paper_cycle);


}


void dataScreen(){ int e_paper_cycle = 0;

    e_paper.setPartialWindow(0, 0, e_paper.width(), 15);
      Serial.println("ENTERING LOOP...");

    e_paper.firstPage();
    do{ 
          //HEADER
            e_paper.fillRect(0, 0, 296, 15, GxEPD_BLACK);
            e_paper.setTextColor(GxEPD_WHITE);
              
          //DATE
            e_paper.setFont(&FreeSans9pt7b);
            e_paper.setCursor(2, 12);  e_paper.print(SystemDate); 
          
          //TIME
          //e_paper.setFont(&FreeSans12pt7b); //FreeMono12pt7b
            e_paper.setFont(&FreeMonoBold9pt7b);
            e_paper.setCursor(230, 12); e_paper.print(ShortTime);

        e_paper_cycle++;

     }while(e_paper.nextPage());


}

void helloWorld(){

  e_paper.setRotation(3);
  e_paper.setFont(&FreeMonoBold9pt7b);
  e_paper.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  e_paper.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x = ((e_paper.width() - tbw) / 2) - tbx;
  uint16_t y = ((e_paper.height() - tbh) / 2) - tby;
  e_paper.setFullWindow();
  e_paper.firstPage();
  do
  {
    e_paper.fillScreen(GxEPD_WHITE);

    e_paper.setCursor(x, y-tbh);
    e_paper.print(HelloWorld);
    
    e_paper.setTextColor(e_paper.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
    e_paper.getTextBounds(HelloWeACtStudio, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((e_paper.width() - tbw) / 2) - tbx;
    e_paper.setCursor(x, y+tbh);
    
    e_paper.print(HelloWeACtStudio);
  }
  while (e_paper.nextPage());
}


void helloFullScreenPartialMode(){

  //Serial.println("helloFullScreenPartialMode");
  const char fullscreen[] = "full screen update";
  const char fpm[] = "fast partial mode";
  const char spm[] = "slow partial mode";
  const char npm[] = "no partial mode";
  e_paper.setPartialWindow(0, 0, e_paper.width(), e_paper.height());
  e_paper.setRotation(1);

  e_paper.setFont(&FreeMonoBold9pt7b);
  if (e_paper.epd2.WIDTH < 104) e_paper.setFont(0);
      e_paper.setTextColor(GxEPD_BLACK);

  const char* updatemode;
  if (e_paper.epd2.hasFastPartialUpdate)
  {
    updatemode = fpm;
  }
  else if (e_paper.epd2.hasPartialUpdate)
  {
    updatemode = spm;
  }
  else
  {
    updatemode = npm;
  }
  // do this outside of the loop
  int16_t tbx, tby; uint16_t tbw, tbh;
  // center update text
  e_paper.getTextBounds(fullscreen, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t utx = ((e_paper.width() - tbw) / 2) - tbx;
  uint16_t uty = ((e_paper.height() / 4) - tbh / 2) - tby;
  // center update mode
  e_paper.getTextBounds(updatemode, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t umx = ((e_paper.width() - tbw) / 2) - tbx;
  uint16_t umy = ((e_paper.height() * 3 / 4) - tbh / 2) - tby;
  // center HelloWorld
  e_paper.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t hwx = ((e_paper.width() - tbw) / 2) - tbx;
  uint16_t hwy = ((e_paper.height() - tbh) / 2) - tby;
  e_paper.firstPage();
  do
  {
    e_paper.fillScreen(GxEPD_WHITE);
    e_paper.setCursor(hwx, hwy);
    e_paper.print(HelloWorld);
    e_paper.setCursor(utx, uty);
    e_paper.print(fullscreen);
    e_paper.setCursor(umx, umy);
    e_paper.print(updatemode);
  }
  while (e_paper.nextPage());
  //Serial.println("helloFullScreenPartialMode done");
}

void showPartialUpdate(){

  // some useful background
  helloWorld();
  // use asymmetric values for test
  uint16_t box_x = 10;
  uint16_t box_y = 15;
  uint16_t box_w = 70;
  uint16_t box_h = 20;
  uint16_t cursor_y = box_y + box_h - 6;

  if (e_paper.epd2.WIDTH < 104) cursor_y = box_y + 6;
    float value = 13.95;
    uint16_t incr = e_paper.epd2.hasFastPartialUpdate ? 1 : 3;
    e_paper.setFont(&FreeMonoBold9pt7b);
    
  if (e_paper.epd2.WIDTH < 104) e_paper.setFont(0);
    e_paper.setTextColor(GxEPD_BLACK);
  // show where the update box is
  for (uint16_t r = 0; r < 4; r++)
  {
    e_paper.setRotation(r);
    e_paper.setPartialWindow(box_x, box_y, box_w, box_h);
    e_paper.firstPage();
    do
    {
      e_paper.fillRect(box_x, box_y, box_w, box_h, GxEPD_BLACK);
      //e_paper.fillScreen(GxEPD_BLACK);
    }
    while (e_paper.nextPage());
    delay(2000);
    e_paper.firstPage();
    do
    {
      e_paper.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (e_paper.nextPage());
    delay(1000);
  }
  //return;
  // show updates in the update box
  for (uint16_t r = 0; r < 4; r++)
  {
    e_paper.setRotation(r);
    e_paper.setPartialWindow(box_x, box_y, box_w, box_h);
    for (uint16_t i = 1; i <= 10; i += incr)
    {
      e_paper.firstPage();
      do
      {
        e_paper.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
        e_paper.setCursor(box_x, cursor_y);
        e_paper.print(value * i, 2);
      }
      while (e_paper.nextPage());
      delay(500);
    }
    delay(1000);
    e_paper.firstPage();
    do
    {
      e_paper.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (e_paper.nextPage());
    delay(1000);
  }
}



 

