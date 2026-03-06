
int heat_levelLed[4] = {10, 11, 12, 13};
int batt_levelLed[4] = {6, 7, 8, 9};
int coilDrive = 5;   //PWM Enabled
int battCharge = 4; // PWM Enabled
int CoolingFan = 3; // PWM Enabled
int speaker = 2;

int SolarNow = 1;
int BattNow = 0;

int adjust = A0;
int tempRead=A1;
int SolarPin = A2;
int BattPin = A3;

int SolarLED = A4;
int BattLED = A5; 


// temp calculations
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;


void setup() {
 DDRD = B11111100; //D2 to D7 as OUTPUTS

serial.begin(9600);
}

unsigned int knob = 0;
bool ON[4] = {false, false, false, false};

void loop() {


 knob = analogRead(adjust);

  if(knob <= 10){
    if(!ON[0])light_ON(heat_levelLed[0]);
  }

  else if(knob > 10 && knob <= 20){
     if(!ON[1])light_ON(heat_levelLed[1]);
  }

  else if(knob > 20 && knob <= 30){
     if(!ON[2])light_ON(heat_levelLed[2]);
  }
  else if(knob > 30){
     if(!ON[3])light_ON(heat_levelLed[3]);
  }

Serial.println(knob); //pulled down

  Vo = analogRead(tempRead);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/ 5.0 + 32.0; 

  Serial.print("Temperature: "); 
  Serial.print(T);
  Serial.println(" F"); 

}

  void light_ON(int kataala){
    digitalWrite(kataala, HIGH);
  }
  
  void light_OFF(int kataala){
    digitalWrite(kataala, LOW);
  }
