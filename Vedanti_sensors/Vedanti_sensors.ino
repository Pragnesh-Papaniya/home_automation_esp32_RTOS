//#include<semphr.h>
SemaphoreHandle_t xMutex;

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd (0x27,16,2);

//Use only core 1 
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else 
static const BaseType_t app_cpu = 1;
#endif

//Pins
static const int soilMoisture = 25;
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

    if(xSemaphoreTake(xMutex, 0xFFFF)==1){
    Serial.print("Moisture:");
    Serial.println(analogRead(soilMoisture));
    xSemaphoreGive(xMutex);
    lcd.setCursor(0,0);   /*Set cursor to Row 1*/
    lcd.print("Moisture:"); 
    lcd.print(analogRead(soilMoisture)); /*print text on LCD*/
  }
  vTaskDelay(500/portTICK_PERIOD_MS);
  }
  
}

//Our task: measure gas detect
void gasDetect(void *parameter){
 
   while(1){

     if(xSemaphoreTake(xMutex, 0xFFFF)==1){
    Serial.print("Gas:");
    Serial.println(analogRead(gasSensor));
    xSemaphoreGive(xMutex);
    lcd.setCursor(0,1);   /*set cursor on row 2*/
    lcd.print("Gas:"); 
    lcd.print(analogRead(gasSensor));
  }
  vTaskDelay(500/portTICK_PERIOD_MS);
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
  pinMode(gasSensor, INPUT);
  pinMode(pirSensor, INPUT_PULLUP); 
  
    // Initialize the LCD connected 
  lcd.init();
  // Turn on the backlight on LCD. 
  lcd.backlight();

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  xTaskCreatePinnedToCore(
    measureMoisture,
    "Measure soil moisture",
    2048,
    NULL,
    1,
    NULL,
    app_cpu);

    xTaskCreatePinnedToCore(
    gasDetect,
    "Detect Gases",
    2048,
    NULL,
    1,
    NULL,
    app_cpu);

  attachInterrupt(digitalPinToInterrupt(pirSensor), detectMovement, RISING);   //(pin,function,mode)

  Serial.begin(115200);
}

void loop() {

}
