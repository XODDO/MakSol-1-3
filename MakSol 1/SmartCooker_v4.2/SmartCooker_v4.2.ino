//ESP32
//uses touch sensrs as buttons
//runs 2 cooker indicative LEDs
//and a 7 seg clock display
//and a battery level indicative display

#include "TM1637.h"
#define CLK 22 //18
#define DIO 23 //4

TM1637 Screen(CLK, DIO);

// --- UNIVERSAL INDICATOR
const int LED = 2;
const int LED2  = 21;//23

// --- ANALOG INPUTS
const int readBattery = 26;
//const int readSolar = 35;

// --- PWM OUTPUTS
const int Coil_1 = 25; // 14;//33
const int Coil_2 = 13;
const int fan_1 = 27;
const int fan_2 = 12;
const int SolarCharge = 14;

const int duration[] = {2, 10, 15, 20, 25, 30, 40, 50, 60}; // in minutes
const int OFF = 5; // 5 minutes

// --- TOUCH INPUTS
const int touch_1 = 15;   // up 1
const int touch_2 = 36; //27; //15; // down 1
const int touch_3 = 4; //13; // up 2
const int touch_4 = 16;// down 2

// --- LED INDICATORS
const int batt[3] = {5, 18, 19};

// --- ALARM
const int TONE_OUTPUT_PIN = 22;
const int TONE_PWM_CHANNEL = 0; 

// The ESP32 has 16 channels which can generate 16 independent waveforms
// We'll just choose PWM channel 0 here
const int sound = 33;
float voltage = 0.0;

void setup() {  
//ATTACK THE COILS first
  delay(10);
  pinMode(Coil_1, OUTPUT);  digitalWrite(Coil_1, HIGH); //ACTIVE WHEN LOW
  pinMode(Coil_2, OUTPUT);  digitalWrite(Coil_2, HIGH); //ACTIVE WHEN LOW

  pinMode(fan_1, OUTPUT); pinMode(fan_2, OUTPUT); // ACTIVE HIGH

// THEN THE DISPLAYS
  Serial.begin(115200); Serial.println("Booting...");


  Screen.init();
  Screen.set(5); // SET BRIGHTNESS OF THE 7 SEGMENT
  delay(100); //time to launch the serial monitor
  
  for(int i=9; i>=0; i--) { delay(70);
      Screen.display(0, i);
      Screen.display(3, i);
    }


  pinMode(LED, OUTPUT);  digitalWrite(LED, HIGH); //POWER LED
  pinMode(LED2, OUTPUT); digitalWrite(LED2, HIGH);//INDICATOR LED

  for(int i=0; i<3; i++){ delay(100);
    pinMode(batt[i], OUTPUT);
    digitalWrite(batt[i], 1);
  }
  
pinMode(sound, OUTPUT); 
digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50);  
digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50); 
digitalWrite(sound, HIGH); delay(50); digitalWrite(sound, LOW); delay(50); 

delay(500);

  pinMode(SolarCharge, OUTPUT); digitalWrite(SolarCharge, LOW);//ACTIVE HIGH

    for(int i=0; i<3; i++) digitalWrite(batt[i], 0);
  int reading = analogRead(readBattery);
  Serial.print("Reading: "); Serial.println(reading);
  voltage = 15.00 * float(reading)/3095.0; Serial.println();
  Serial.print("Voltage: "); Serial.println(voltage);



 //while(!power)
}


unsigned long now, then, stat; 

int currentMode = 1;
uint8_t heat_1_level = 0;
uint8_t heat_2_level = 0;


int up1=0; int up2=0; //const int threshold_up = 30; //for touch abilities

int down1=0; int down2=0; //const int threshod_dn = 40; // for touch abilities
 
void loop() {

now = millis();
//if(now - then == 1000){}

  
touchReader();
touchProcessor();

Serial.print("Time: "); Serial.println(now);

Serial.print("UP1 = "); Serial.println(up1);
Serial.print("UP2 = "); Serial.println(up2);
Serial.print("DOWN1 = "); Serial.println(down1);
Serial.print("DOWN2 = "); Serial.println(down2);

// close loop()
}


void touchReader(){

  bool toggled = false;
  
    up1 = digitalRead(touch_1);
    up2 = digitalRead(touch_3);
  down1 = digitalRead(touch_2);
  down2 = digitalRead(touch_4);

  if(up1 || up2 || down1 || down2) delay(150);
}

void touchReader2(){

  int up1_1, up1_2, up1_3, up1_4;
  int up2_1, up2_2, up2_3, up2_4;
  int dn1_1, dn1_2, dn1_3, dn1_4;
  int dn2_1, dn2_2, dn2_3, dn2_4;
  
                        up1_1 = digitalRead(touch_1); delay(10); 
                        up1_2 = digitalRead(touch_1); delay(10);
                        up1_3 = digitalRead(touch_1); delay(10); 
                        up1_4 = digitalRead(touch_1); delay(10);
  up1 = (up1_1 + up1_2 + up1_3 + up1_4)/4; 

                          up2_1 = digitalRead(touch_3); delay(10);
                          up2_2 = digitalRead(touch_3); delay(10);
                          up2_3 = digitalRead(touch_3); delay(10);
                          up2_4 = digitalRead(touch_3); delay(10);
  up2 = (up2_1 + up2_2 + up2_3 + up2_4)/4;

                          dn1_1 = digitalRead(touch_2); delay(10); Serial.print("Dn1 = "); Serial.println(dn1_1);
                          dn1_2 = digitalRead(touch_2); delay(10); Serial.print("Dn2 = "); Serial.println(dn1_2);
                          dn1_3 = digitalRead(touch_2); delay(10); Serial.print("Dn3 = "); Serial.println(dn1_3);
                          dn1_4 = digitalRead(touch_2); delay(10); Serial.print("Dn4 = "); Serial.println(dn1_4);
   down1 = (dn1_1 + dn1_2 + dn1_3 + dn1_4)/4;  //till the error is debugged -!!
   //down1 = digitalRead(touch_2); delay(50);

                          dn2_1 = digitalRead(touch_4); delay(10);
                          dn2_2 = digitalRead(touch_4); delay(10);
                          dn2_3 = digitalRead(touch_4); delay(10);
                          dn2_4 = digitalRead(touch_4); delay(10);
   down2 = (dn2_1 + dn2_2 + dn2_3 + dn2_4)/4;
   

}

void touchProcessor(){ // LEVEL SHIFTER
  
  if(up1 == 1) { Screen.display(0, heat_1_level); if(heat_1_level < 6) heat_1_level++; else heat_1_level = 6; }
  if(up2 == 1) {  Screen.display(3, heat_2_level); if(heat_2_level < 6) heat_2_level++; else heat_2_level = 6; }
  
  if(down1 == 1) { Screen.display(0, heat_1_level); if(heat_1_level > 0) heat_1_level--; else heat_1_level = 0;}
  if(down2 == 1) { Screen.display(3, heat_2_level); if(heat_2_level > 0) heat_2_level--; else heat_2_level = 0;}

if(up1 == 1 || up2 == 1 || down1 == 1 || down2 == 1){ digitalWrite(LED2, HIGH); digitalWrite(sound, 1);  delay(50); digitalWrite(sound, 0); digitalWrite(LED2, LOW);}

if(up1 == 0 || up2 == 0 || down1 == 0 || down2 == 0) { digitalWrite(LED2, LOW); digitalWrite(LED2, LOW); }

Serial.println();
Serial.print("Coil 1 Level: "); Serial.println(heat_1_level);
Serial.print("Coil 2 Level: "); Serial.println(heat_2_level);
Serial.println();

heatingFactor(heat_1_level, heat_2_level);

  //return 0;
}

bool Coil_1_ON = false, Coil_2_ON = false;
bool waiting = false;

unsigned long wait_ko = 10000; // 5 minutes ideal Cooling time \\ 300,000

unsigned long interval_1 = 10000; // 10 seconds
unsigned long interval_2 = 20000; // 20 seconds
unsigned long interval_3 = 30000; // 30 seconds
unsigned long interval_4 = 60000; // 60 seconds // 1 minute
unsigned long interval_5 = 300000; // 5 minutes
unsigned long interval_6 = 600000; // 10 minutes
unsigned long interval_7 = 1800000; // 30 minutes
unsigned long interval_8 = 3600000; // 60 minutes
unsigned long interval_10= 7200000; // 2 hour



void heatingFactor(int coil_1_index, int coil_2_index){
  //kozesa coil 1 index otambuze coil 1 state ... and likewise 2 4 2
  if(coil_1_index == 0){
     if(Coil_1_ON) {Interpret_State(Coil_1_ON); Coil_1_ON = false; }
        Serial.println("Coil 1 now OFF...");
  }//close if zero

else { //open if not zero (1-9) 
  Serial.print("Coil 1 Started at: "); Serial.println(stat/1000);
    if(!Coil_1_ON) {//tuen it on if it is not in waiting state
      if(!waiting) {Interpret_State(Coil_1_ON); Coil_1_ON = true; stat = millis();
           Serial.print("Coil 1 fired ON at: "); Serial.print(stat);
            }}

  switch(coil_1_index){ 
      case 1: if(now - stat <= interval_1) { Serial.print("COIL 1 now firing...for=> ");  Serial.print("::"); Serial.println((now-stat)/1000); }
              else { // if more than first interval
                  if(now - stat <= (wait_ko + interval_1)){Interpret_State(Coil_1_ON); waiting = true; Coil_1_ON = false; Serial.println("Waiting Now...");}//wating time
                  else {Interpret_State(Coil_1_ON); waiting = false; Serial.println("Waiting Time Over"); } //toggled back ON
                    } 
            break;

      case 2:  if(now - stat <= interval_2) { Serial.print("COIL 1 now firing... for=> ");  Serial.print("::"); Serial.println((now-stat)/1000);}
              else { // if more than first interval
                  if(now - stat <= (wait_ko + interval_2)){Interpret_State(Coil_1_ON); waiting = true; Coil_1_ON = false; Serial.println("Waiting Now...");}//wating time
                  else {Interpret_State(Coil_1_ON); waiting = false; Serial.println("Waiting Time Over"); } //toggled back ON
                    }  
             break;

      case 3:  if(now - stat <= interval_3) { Serial.print("COIL 1 now firing... for=> "); Serial.print("::"); Serial.println((now-stat)/1000);}
              else { // if more than first interval
                  if(now - stat <= (wait_ko + interval_3)){Interpret_State(Coil_1_ON); waiting = true; Coil_1_ON = false; Serial.println("Waiting Now...");}//wating time
                  else {Interpret_State(Coil_1_ON); waiting = false; Serial.println("Waiting Time Over"); } //toggled back ON
                    }
             break;

      case 4: if(now - stat <= interval_4) { Serial.print("COIL 1 now firing... for=> "); Serial.print("::"); Serial.println((now-stat)/1000);}
              else { // if more than first interval
                  if(now - stat <= (wait_ko + interval_4)){Interpret_State(Coil_1_ON); waiting = true; Coil_1_ON = false; Serial.println("Waiting Now...");}//wating time
                  else {Interpret_State(Coil_1_ON); waiting = false; Serial.println("Waiting Time Over"); } //toggled back ON
                    }
              break;
      
      case 5: if(now - stat <= interval_5) { Serial.print("COIL 1 now firing... for=> "); Serial.print("::"); Serial.println((now-stat)/1000);}
              else { // if more than first interval
                  if(now - stat <= (wait_ko + interval_5)){Interpret_State(Coil_1_ON); waiting = true; Coil_1_ON = false; Serial.println("Waiting Now...");}//wating time
                  else {Interpret_State(Coil_1_ON); waiting = false; Serial.println("Waiting Time Over"); } //toggled back ON
                    }
              break;

      case 6: if(now - stat <= interval_6) { Serial.print("COIL 1 now firing... for=> "); Serial.print("::"); Serial.println((now-stat)/1000);}
              else { // if more than first interval
                  if(now - stat <= (wait_ko + interval_6)){Interpret_State(Coil_1_ON); waiting = true; Coil_1_ON = false; Serial.println("Waiting Now...");}//wating time
                  else {Interpret_State(Coil_1_ON); waiting = false; Serial.println("Waiting Time Over"); } //toggled back ON
                    }
              break;

    }

  }//closing if not zero
  
  
}


void Interpret_State(bool current_state) {
  if(current_state == true) { //ACTIVE WHEN LOW
      digitalWrite(Coil_1, HIGH); //LOW
      digitalWrite(fan_1, HIGH);} 

  if(current_state == false) {//ACTIVE LOW
      digitalWrite(Coil_1, LOW); //HIGH
      digitalWrite(fan_1, LOW);}



}
