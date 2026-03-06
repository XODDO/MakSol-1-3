
int heat_levelLed[4] = {A10, A12, A14, 32};  // --- Level indicators --- //
int heat2_levelLed[4] = {40, 42, 44, 46};


int sound = 49;
int coil = 5;       // PWM Enabled
int coil_1_fan = 3;    // PWM Enabled
int coil2= 11;
int coil_2_fan = 13;

int knobRead = A0; //ADJUSTABLE INPUTS
int knobRead2= A2; // ADJUSTABLE INPUT 2


int battLvl_LED[4]; //power indicators

int SolarNow = 7; // PWM Enabled
int BattNow = 6;  
int powerSupply = 3;     

int SolarPin = A4; // POWER INPUTS
int BattPin = A6;

int SolarLED = 33; // POWER INDICATORS
int BattLED = 35; 

int LED = 13;

void setup() {
// === DDRD = B11111100; //setting D2 to D7 as OUTPUTS --- ||| ---
pinMode(LED, OUTPUT);


pinMode(coil_1_fan, OUTPUT); 
pinMode(coil, OUTPUT);

 for(int i=0; i<4; i++){  pinMode(heat_levelLed[i], OUTPUT); /*digitalWrite(heat_levelLed[i], 1);*/}
    Serial.begin(115200);

digitalWrite(coil_1_fan, HIGH); // --- AutoRUN until --- //

digitalWrite(coil, HIGH); // ---- toggle only when knob is halfway --- //


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

int LED_Count = 0;

bool LED_ON = false;

void loop() {

 _1st_Pos = analogRead(knobRead);
        delay(50);
 _2nd_Pos = analogRead(knobRead);
        
knob = _2nd_Pos;

 //Serial.println();        
 //Serial.print("Position 1: "); Serial.println(_1st_Pos);
 //Serial.print("Position 2: "); Serial.println(_2nd_Pos);

//Serial.println();


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

  digitalWrite(coil_1_fan, LOW); // --- TOGGLE ON COOLING FIRST --- //
  if(!coil_ON) {digitalWrite(coil, LOW); coil_ON = true;} // ---- toggle ON only when knob is halfway --- //
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
      digitalWrite(coil_1_fan, HIGH); // FAN goes OFF last
      if(coil_ON) {digitalWrite(coil, HIGH); coil_ON = false; /*Serial.println("COIL OFF");*/} // ---- toggle OFF if was still ON --- //
        }
      
 else if(knob > 100 && knob <= 400){ // Remove the second ---- down to level 1
     
        if(ON[1]) {ON[1] = false; switch_OFF(heat_levelLed[1]);}
        if(ON[2]) {ON[2] = false; switch_OFF(heat_levelLed[2]);}
        if(ON[3]) {ON[3] = false; switch_OFF(heat_levelLed[3]);}
      if(coil_ON) {digitalWrite(coil, HIGH); coil_ON = false; /*Serial.println("COIL OFF");*/}
         }

 else if(knob > 400 && knob <= 700){ // Remove the third ---- Down to level 2
     
        if(ON[2]) {ON[2] = false; switch_OFF(heat_levelLed[2]);}
        if(ON[3]) {ON[3] = false; switch_OFF(heat_levelLed[3]);} //4th
           
         }

 else if(knob > 700 && knob <=1000){ // Remove the fourth  --- Down to level 3
            
          ON[3] = false; switch_OFF(heat_levelLed[3]); //4th
            
         }
      
      }//close backward

    LED_Count++;
//Serial.println(LED_Count);

if(LED_Count <= 15) {if(LED_ON) {digitalWrite(LED, LOW); LED_ON = false;}}
if(LED_Count > 20 && LED_Count < 25){
  if(!LED_ON) {digitalWrite(LED, HIGH); LED_ON = true;  Serial.println("COIL ON");}
}

else if(LED_Count > 25){LED_Count = 0;}

   // Serial.println();
  }//close LOOP

  void switch_ON(int kataala){
       digitalWrite(kataala, HIGH);
  }
  
  void switch_OFF(int kataala){
      digitalWrite(kataala, LOW);
  }
