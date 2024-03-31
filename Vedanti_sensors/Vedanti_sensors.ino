//Use only core 1 
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else 
static const BaseType_t app_cpu = 1;
#endif

//Pins
static const int soilMoisture = 28;
static const int pirSensor = 27;
static const int gasSensor = 26;
static const int buzzer = 2;

#define timeSeconds 10

//Timer
long currentTime = millis();
long lastTrigger = 0;             //holds time when PIR detects motion
boolean startTimer = false;       //starts timer when motion is detected

//Our task: measure soil moisture
void measureMoisture(void *parameter){
  while(1){
    Serial.println("Moisture:");
    Serial.print(analogRead(soilMoisture));
  }
}

//Our task: measure gas detect
void gasDetect(void *parameter){
  while(1){
    Serial.println("Gas:");
    Serial.print(analogRead(gasSensor));
  }
}

//IRAM_ATTR: to detect movement
void detectMovement(){
  digitalWrite(buzzer, HIGH);
  startTimer = true;
  lastTrigger = millis();

  Serial.println("Detected");

     currentTime = millis();

  if((startTimer)&&(currentTime - lastTrigger>(timeSeconds*1000))){
    digitalWrite(buzzer, LOW);
    Serial.println("Motion Stopped");

    startTimer = false;
  }
}

void setup() {
  pinMode(soilMoisture,INPUT);
  pinMode(gasSensor, INPUT_PULLUP);
  pinMode(pirSensor, INPUT_PULLUP); 
  
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  xTaskCreatePinnedToCore(
    measureMoisture,
    "Measure soil moisture",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);

    xTaskCreatePinnedToCore(
    gasDetect,
    "Detect Gases",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);

  attachInterrupt(digitalPinToInterrupt(pirSensor), detectMovement, RISING);   //(pin,function,mode)

  Serial.begin(115200);
}

void loop() {

}
