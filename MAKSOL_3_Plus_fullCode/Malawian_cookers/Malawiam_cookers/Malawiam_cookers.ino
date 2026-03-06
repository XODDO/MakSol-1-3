/*  XODDOCODE 2025 
    30th APRIL 2025 EDITION ---- EXPORTABLE COOKERS
 -- MINIMAL POWER MANAGEMENT
 -- ACCESSIBLE COOKING TIMERS
 -- DATA STORAGE 
*/

#include "Arduino.h"


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#include "RTClib.h"

RTC_DS3231 real_time;


//FONTS
#include <Fonts/FreeSansBold24pt7b.h> // FreeSerifBold24pt7b
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 LCD(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);




uint8_t up1 = 39, down1 = 36;
uint8_t up2 = 35, down2 = 34;

// --- PWM OUTPUTS
const uint8_t Coil_1 = 26;   // 14; //26;  //13; // 14; //33
const uint8_t Coil_2 = 14;  //  26;  
const uint8_t fan_1 = 25;  //   33;
const uint8_t fan_2 = 33; //    25;

void initialize_RTC();
void LCD_BUS(uint8_t bus);
void initialize_OLEDz();



uint8_t boot_level = 0;
char BootMessage[200] = "";

void setup(){
  delay(50);
  Serial.begin(115200); delay(50);

  // STEP 1:
  strcpy(BootMessage, "Booting MakSol 3+"); Serial.println(BootMessage); 
  Wire.begin();


  // STEP 2:
  strcpy(BootMessage, "Initializing Cooking Zones"); Serial.println(BootMessage);
  pinMode(Coil_1, OUTPUT); digitalWrite(Coil_1, HIGH); delay(5);// put it OFF
  pinMode(Coil_2, OUTPUT); digitalWrite(Coil_2, HIGH); delay(5); // put it OFF
  strcpy(BootMessage, "Cooking Zones Successfully!"); Serial.println(BootMessage);
    delay(1000);

  //STEP 3:
  strcpy(BootMessage, "Initializing Cooling Fans"); Serial.println(BootMessage);
  pinMode(fan_1, OUTPUT); digitalWrite(fan_1, HIGH); delay(5);// KEEP IT ON
  pinMode(fan_2, OUTPUT); digitalWrite(fan_2, HIGH); delay(5); // KEEP IT ON
  strcpy(BootMessage, "Cooking Zones Successfully!"); Serial.println(BootMessage);

//200W*6panels = (1,200Wp * 5hrs) = 6kVA 

  //STEP 4
   strcpy(BootMessage, "Initializing Realtime Clock"); Serial.println(BootMessage);
   intialize_RTC();
   Serial.println(BootMessage); delay(100);


  //STEP 5 
  strcpy(BootMessage, "Initializing OLED Displays"); Serial.println(BootMessage);
  initialize_OLEDz();
  Serial.println(BootMessage); delay(100);
 

  //STEP 6
  strcpy(BootMessage, "Initializing Input Buttons"); Serial.println(BootMessage);
  pinMode(up1, INPUT);  pinMode(up2, INPUT);  pinMode(down1, INPUT);  pinMode(down2, INPUT);






}







void loop(){

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

// Select I2C BUS
void LCD_BUS(uint8_t bus){ //void TCA9548A(uint8_t bus)
  Wire.beginTransmission(0x70);  // TCA9548A address
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();
  Serial.print(bus);
}



void initialize_OLEDz(){
  uint8_t attempts = 0;

  // Init OLED display on bus number 4
  LCD_BUS(4);
  while(!LCD.begin(i2c_Address, true)) {
    strcpy(BootMessage, "OLED 1 Adafruit_SH1106G allocation failed on OLED 1");
      attempts++;
      if(attempts >= 10) break;
  } 
  // Clear the buffer
  LCD.clearDisplay();
 
 

  // Init OLED display on bus number 6
  LCD_BUS(6);
  if(!LCD.begin(i2c_Address, true)) {
      strcpy(BootMessage, "OLED 2 Adafruit_SH1106G allocation failed on OLED 2");
     
      while(attempts < 20) {LCD.begin(i2c_Address, true);  attempts++; } break;
      
  } 
  else  strcpy(BootMessage, "OLED 2 Adafruit_SH1106G allocation successful on OLED 1");
  //  Clear the buffer
      LCD.clearDisplay();
}
