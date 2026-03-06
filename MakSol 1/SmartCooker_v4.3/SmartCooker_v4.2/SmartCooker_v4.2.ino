//ESP32
//uses touch sensrs as buttons
//runs 2 cooker indicative LEDs
//and a 7 seg clock display
//and a battery level indicative display

#include "TM1637.h"
#define CLK 18 //18
#define DIO 4 //4

TM1637 Screen(CLK, DIO);

// --- UNIVERSAL INDICATOR
const int LED2 = 2;
const int LED  = 33;//23

// --- ANALOG INPUTS
const int readBattery = 34;
const int readSolar = 35;

// --- PWM OUTPUTS
const int Coil_1 = 14;//33
const int Coil_2 = 26;
const int fan_1 = 25;
const int fan_2 = 27;
const int SolarCharge = 32;

const int duration[] = {2, 10, 15, 20, 25, 30, 40, 50, 60}; // in minutes
const int OFF = 5; // 5 minutes

// --- TOUCH INPUTS
const int touch_1 = 13;   // up 1
const int touch_2 = 0; //27; //15; // down 1
const int touch_3 = 15; //13; // up 2
const int touch_4 = 4;// down 2

// --- LED INDICATORS
const int right[3] = {5, 17, 16};
const int left[3] = {1, 3, 21};
const int batt[4] = {};

// --- ALARM
const int TONE_OUTPUT_PIN = 22;

// The ESP32 has 16 channels which can generate 16 independent waveforms
// We'll just choose PWM channel 0 here
// We'll just choose PWM channel 0 here
const int TONE_PWM_CHANNEL = 0; 
const int sound = 22;

void setup() {
  Serial.begin(115200);

  Screen.init();
  Screen.set(5); // SET BRIGHTNESS OF THE 7 SEGMENT

pinMode(SolarCharge, OUTPUT); digitalWrite(SolarCharge, HIGH);//ACTIVE LOW
pinMode(Coil_1, OUTPUT);  digitalWrite(Coil_1, HIGH); //ACTIVE WHEN LOW
pinMode(Coil_2, OUTPUT);  digitalWrite(Coil_2, HIGH); //ACTIVE WHEN LOW
pinMode(fan_1, OUTPUT); pinMode(fan_2, OUTPUT); // ACTIVE HIGH

  pinMode(LED, OUTPUT);  digitalWrite(LED, HIGH); //POWER LED
  pinMode(LED2, OUTPUT); digitalWrite(LED2, LOW);//INDICATOR LED

  delay(100); //soma time to launch the serial monitor
  
pinMode(sound, OUTPUT); Serial.println("Booting...");

digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50);//Screen.display(1, 8); 
digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50);//Screen.display(2, 8); delay(50);
digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50);  

delay(500);

Screen.display(0, 0); Screen.display(3, 0); 
  
}


unsigned long now, then, stat; 

int currentMode = 1;
uint8_t heat_1_level = 0;
uint8_t heat_2_level = 0;


int up1=0; int up2=0; const int threshold_up = 30;

int down1=0; int down2=0; const int threshod_dn = 40;
 
void loop() {

now = millis();
//if(now - then == 1000){}

  Serial.print("UP1 = "); Serial.println(up1);
  Serial.print("UP2 = "); Serial.println(up2);
  Serial.print("DOWN1 = "); Serial.println(down1);
  Serial.print("DOWN2 = "); Serial.println(down2);

touchReader();
touchProcessor();

Serial.println(now);

// close loop()
}

void touchReader(){

int up1_1, up1_2, up1_3, up1_4;
int up2_1, up2_2, up2_3, up2_4;
int dn1_1, dn1_2, dn1_3, dn1_4;
int dn2_1, dn2_2, dn2_3, dn2_4;
  
                        up1_1 = digitalRead(touch_1);delay(20); 
                        up1_2 = digitalRead(touch_1); delay(20);
                        up1_3 = digitalRead(touch_1); delay(20); 
                        up1_4 = digitalRead(touch_1); delay(20);
  up1 = (up1_1 + up1_2 + up1_3 + up1_4)/4; 

                          up2_1 = digitalRead(touch_3); 
               delay(20); up2_2 = digitalRead(touch_3); 
               delay(20); up2_3 = digitalRead(touch_3); 
               delay(20); up2_4 = digitalRead(touch_3); delay(20);
  up2 = (up2_1 + up2_2 + up2_3 + up2_4)/4;

  down1 = digitalRead(touch_2); delay(50);


  down2 = digitalRead(touch_4); delay(50);

}

void touchProcessor(){
  
  if(up1 == 1) { Screen.display(0, heat_1_level); if(heat_1_level < 9) heat_1_level++; else heat_1_level = 9; }
  if(up2 == 1) { Screen.display(3, heat_2_level); if(heat_2_level < 9) heat_2_level++; else heat_2_level = 9; }
  
  if(down1 == 1) { Screen.display(0, heat_1_level); if(heat_1_level > 0) heat_1_level--; else heat_1_level = 0;}
  if(down2 == 1) { Screen.display(3, heat_2_level); if(heat_2_level > 0) heat_2_level--; else heat_2_level = 0;}

if(up1 == 1 || up2 == 1 || down1 == 1 || down2 == 1){ digitalWrite(LED, HIGH); digitalWrite(sound, 1);  delay(50); digitalWrite(sound, 0);}

if(up1 == 0 || up2 == 0 || down1 == 0 || down2 == 0) { digitalWrite(LED, LOW); digitalWrite(LED2, LOW); }

Serial.println();
Serial.print("Coil 1 Level: "); Serial.println(heat_1_level);
Serial.print("Coil 2 Level: "); Serial.println(heat_2_level);
Serial.println();

heatingFactor(heat_1_level, heat_2_level);

  //return 0;
}

bool Coil_1_ON = false, Coil_2_ON = false;
unsigned long wait_ko = 5*60*1000; // 5 minutes waiting time \\ 300,000
bool waiting = false;

void heatingFactor(int coil_1_index, int coil_2_index){
  //kozesa coil 1 index otambuze coil 1 state ... and likewise 2 4 2
  if(coil_1_index == 0){
     if(Coil_1_ON) Coil_1_ON = false;
        Serial.println("Coil 1 now OFF...");
  }//close if zero

else { //open if not zero (1-9) 
    if(!Coil_1_ON) {//tuen it on if it is not in waiting state
      if(!waiting) {Coil_1_ON = true; stat = millis();
           Serial.print("Coil 1 fired ON at: "); Serial.print(stat);
            }}

  switch(coil_1_index){
      case 1: if(now - stat <= 5000) { Serial.println("COIL 1 still firing..."); }
              else { // if more than first interval
                  if(now - stat <= (2*10000)){ waiting = true; Coil_1_ON = false; Serial.println("Waiting Now...");}//wating time
                  else {waiting = false; Coil_1_ON = true; Serial.println("Waiting Time Over"); } //toggled back ON
                    } 
            break;

      case 2:  break;

      case 3:  break;

    }

  }//closing if not zero
  
  statusTraker();
}


void statusTraker() {
  if(Coil_1_ON) { //ACTIVE WHEN LOW
      digitalWrite(Coil_1, LOW);
      digitalWrite(fan_1, HIGH);} 

  if(Coil_1_ON == false) {//ACTIVE LOW
      digitalWrite(Coil_1, HIGH);
      digitalWrite(fan_1, HIGH);}



}