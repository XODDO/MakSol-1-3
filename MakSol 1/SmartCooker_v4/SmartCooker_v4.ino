//ESP32
//uses touch sensrs as buttons
//runs 2 cooker indicative LEDs
//and a 7 seg clock display
//and a battery level indicative display


// --- UNIVERSAL INDICATOR
const int LED = 2;
const int LED2=23;

// --- ANALOG INPUTS
const int readBattery = 34;
const int readSolar = 35;

// --- PWM OUTPUTS
const int Coil_1 = 32;
const int Coil_2 = 33;
const int fan_1 = 25;
const int fan_2 = 26;
const int SolarCharge = 27;

// --- TOUCH INPUTS
const int touch_1 = 13;   // up 1
const int touch_2 = 15; // down 1
const int touch_3 = 12; // up 2
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


void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT); digitalWrite(LED, LOW);
  pinMode(LED2, OUTPUT); digitalWrite(LED2, LOW);

  delay(500); //soma time to launch the serial monitor

  
  ledcAttachPin(TONE_OUTPUT_PIN, TONE_PWM_CHANNEL);  
  Serial.println("Booting...");

}

int currentMode = 1;
uint8_t heat_1_level = 0;
uint8_t heat_2_level = 0;


int up1; int up2; const int threshold_up = 10;

int down1; int down2; const int threshod_dn = 50;
 



void loop() {



up1 = touchRead(touch_1);
down1 = touchRead(touch_2);

up2 = touchRead(touch_3);
down2 = touchRead(touch_4);


Serial.print("UP1 = "); Serial.println(up1);
Serial.print("UP2 = "); Serial.println(up2);
Serial.print("DOWN1 = "); Serial.println(down1);
Serial.print("DOWN2 = "); Serial.println(down2);

touchProcessor();


//delay(50);
delay(200);
// close loop()
}


void touchProcessor(){
  
  if(up1 < threshold_up) { heat_1_level++;}
  if(up2 < threshold_up) { heat_2_level++;}
  
  if(down1 < threshod_dn) { heat_1_level--;}
  if(down2 < threshod_dn) { heat_2_level--;}

if(up1 < threshold_up || down1 < threshod_dn){digitalWrite(LED, HIGH); ledcWriteTone(TONE_PWM_CHANNEL, 1500);  delay(50); ledcWriteTone(TONE_PWM_CHANNEL, 0);}
if(up2 < threshold_up || down2 < threshod_dn){digitalWrite(LED2, HIGH); ledcWriteTone(TONE_PWM_CHANNEL, 1500);  delay(50); ledcWriteTone(TONE_PWM_CHANNEL, 0);}

if(up1 >= threshold_up || up2 >= threshold_up || down1 >= threshod_dn || down2 >= threshod_dn) { digitalWrite(LED, LOW); digitalWrite(LED2, LOW); }

Serial.println();
Serial.print("Coil 1 Level: "); Serial.println(heat_1_level);
Serial.print("Coil 2 Level: "); Serial.println(heat_2_level);
Serial.println();
  //return 0;
}
