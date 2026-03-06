
int heat_levelLed[4] = {8, 9, 10, 11};  // --- 13 --- //
int batt_levelLed[4] = {22, 24, 26, 28};

int SolarNow = 7;
int BattNow = 6;  

int speaker = 5;
int coilDrive = 4;       // PWM Enabled
int battCharge = 3;     // PWM Enabled
int CoolingFan = 2;    // PWM Enabled

int adjust = A2; //INPUTS
int tempRead=A1;
int SolarPin = A0;
int BattPin = A3;

int SolarLED = A4;
int BattLED = A5; 


void setup() {
// === DDRD = B11111100; //setting D2 to D7 as OUTPUTS --- ||| ---

pinMode(CoolingFan, OUTPUT); 
pinMode(coilDrive, OUTPUT);

 for(int i=0; i<4; i++){  pinMode(heat_levelLed[i], OUTPUT); /*digitalWrite(heat_levelLed[i], 1);*/}
    Serial.begin(9600);

digitalWrite(CoolingFan, HIGH); // --- AutoRUN until --- //

digitalWrite(coilDrive, HIGH); // ---- toggle only when knob is halfway --- //


}

unsigned int knob = 0; //0 - 65,000
unsigned int _1st_Pos = 0;// -32,000 - +32,000
unsigned int _2nd_Pos = 0;

bool ON[4] = {false, false, false, false};

bool forward = false; 
bool backward=false;

bool set = false;
bool coil_ON = false;
bool fan_ON = false;

void loop() {

 _1st_Pos = analogRead(adjust);
        delay(50);
 _2nd_Pos = analogRead(adjust);
        
knob = _2nd_Pos;

 Serial.println();        
 Serial.print("Position 1: "); Serial.println(_1st_Pos);
 Serial.print("Position 2: "); Serial.println(_2nd_Pos);

Serial.println();


     if(_2nd_Pos - _1st_Pos >= 3){forward = true; backward = false; }

else if(_1st_Pos - _2nd_Pos >= 3) {backward = true; forward = false;}



if(forward){//if forward movement

  if(knob <= 100){//heat level zero  
        if(ON[0]) {ON[0] = false; switch_OFF(heat_levelLed[0]);}      
        if(ON[1]) {ON[1] = false; switch_OFF(heat_levelLed[1]);}
        if(ON[2]) {ON[2] = false; switch_OFF(heat_levelLed[2]);}
        if(ON[3]) {ON[3] = false; switch_OFF(heat_levelLed[3]);} 
    }  
  
  else if(knob > 100 && knob <= 400){ // up to heat level 1
    if(!ON[0]) {ON[0] = true; switch_ON(heat_levelLed[0]);}
    
  }


  else if(knob > 400 && knob <= 700){ //up to heat level 2
    if(!ON[0]) {ON[0] = true; switch_ON(heat_levelLed[0]);}
    if(!ON[1]) {ON[1] = true; switch_ON(heat_levelLed[1]);}

  digitalWrite(CoolingFan, LOW); // --- TOGGLE ON COOLING FIRST --- //
  if(!coil_ON) {digitalWrite(coilDrive, LOW); coil_ON = true;} // ---- toggle ON only when knob is halfway --- //
    }

  else if(knob > 700 && knob <= 1000){ //up to heat level 3
    if(!ON[0]) {ON[0] = true; switch_ON(heat_levelLed[0]);}
    if(!ON[1]) {ON[1] = true; switch_ON(heat_levelLed[1]);}
    if(!ON[2]) {ON[2] = true; switch_ON(heat_levelLed[2]);}

    }

  else if(knob > 1000){ //up to heat level 4
    if(!ON[0]) {ON[0] = true; switch_ON(heat_levelLed[0]);}
    if(!ON[1]) {ON[1] = true; switch_ON(heat_levelLed[1]);}
    if(!ON[2]) {ON[2] = true; switch_ON(heat_levelLed[2]);}
    if(!ON[3]) {ON[1] = true; switch_ON(heat_levelLed[3]);}
  }
else {}

    }

    if(backward){
      
      if(knob <=100){//Remove ALL --- BACK TO level 0
        if(ON[0]) {ON[0] = false; switch_OFF(heat_levelLed[0]);}      
        if(ON[1]) {ON[1] = false; switch_OFF(heat_levelLed[1]);}
        if(ON[2]) {ON[2] = false; switch_OFF(heat_levelLed[2]);}
        if(ON[3]) {ON[3] = false; switch_OFF(heat_levelLed[3]);}      
      digitalWrite(CoolingFan, HIGH); // FAN goes OFF last
      if(coil_ON) {digitalWrite(coilDrive, HIGH); coil_ON = false;} // ---- toggle OFF if was still ON --- //
        }
      
 else if(knob > 100 && knob <= 400){ // Remove the second ---- down to level 1
     
        if(ON[1]) {ON[1] = false; switch_OFF(heat_levelLed[1]);}
        if(ON[2]) {ON[2] = false; switch_OFF(heat_levelLed[2]);}
        if(ON[3]) {ON[3] = false; switch_OFF(heat_levelLed[3]);}
      if(coil_ON) {digitalWrite(coilDrive, HIGH); coil_ON = false;}
         }

 else if(knob > 400 && knob <= 700){ // Remove the third ---- Down to level 2
     
        if(ON[2]) {ON[2] = false; switch_OFF(heat_levelLed[2]);}
        if(ON[3]) {ON[3] = false; switch_OFF(heat_levelLed[3]);} //4th
           
         }

 else if(knob > 700 && knob <=1000){ // Remove the fourth  --- Down to level 3
            
          ON[3] = false; switch_OFF(heat_levelLed[3]); //4th
            
         }
      
      }//close backward
    
    
  }//close LOOP

  void switch_ON(int kataala){
       digitalWrite(kataala, HIGH);
  }
  
  void switch_OFF(int kataala){
      digitalWrite(kataala, LOW);
  }
