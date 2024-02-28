//Gas Sensor with (active) piezoelectric Buzzer
//4 pins: VCC,GND,D0,A0

#define timeSeconds 10

//Set Pins
const int gasSensor = 26;  //Using Digiatal Pin
const int buzzer = 2;

//Timer
long currentTime = millis();
long lastTrigger = 
int sensorValue = 0;
void setup() {
  pinMode(gasSensor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(gasSensor), detectGas, FALLING);
  
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  Serial.begin(115200);
}

void loop() {
  sensorValue = analogRead(gasSensor);
  Serial.println(sensorValue);
  
  delay(500);
}
