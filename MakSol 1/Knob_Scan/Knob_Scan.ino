
int knobRead = A0; //INPUTS

void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
}

void loop() {
Serial.println(analogRead(A0));
Serial.println(analogRead(A1));
Serial.println(analogRead(A2));
Serial.println(analogRead(A3));

Serial.println();
delay(50);
}
