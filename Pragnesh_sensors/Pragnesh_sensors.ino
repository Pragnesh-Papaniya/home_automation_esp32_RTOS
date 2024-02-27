/*

*/
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include <DHT.h>
#include "Adafruit_VL53L0X.h"
#define dhtPin 15
#define FlamePin 2
#define BuzzPin 23

void setup() {
  Serial.begin(300);
  xTaskCreatePinnedToCore(Task_DHT11, "Task_DHT11", 4000, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(Task_TOF, "Task_TOF", 4000, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(Task_Flame, "Task_Flame", 4000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
}

// the loop function runs over and over again forever
void loop() {
  
}

void Task_DHT11(void *pvParameters) // This is a task.
{
    (void)pvParameters;

    DHT dht(dhtPin,DHT11);

    while (! Serial) {
      vTaskDelay(1/portTICK_PERIOD_MS);  
    }

    dht.begin();
    vTaskDelay(2000/portTICK_PERIOD_MS);

    while (1) // A Task shall never return or exit.
    {
        float temp = dht.readTemperature();
        float humidity = dht.readHumidity();
        Serial.print("Temp: ");
        Serial.print(temp);
        Serial.print("c  ");
        Serial.print("humidty: ");
        Serial.print(humidity);
        Serial.println("%");
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

void Task_TOF(void *pvParameters) // This is a task.
{
  (void)pvParameters;

  Adafruit_VL53L0X lox = Adafruit_VL53L0X();

  // wait until serial port opens for native USB devices
  while (! Serial) {
    vTaskDelay(1/portTICK_PERIOD_MS);  
  }

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

    while (1) // A Task shall never return or exit.
    {
      VL53L0X_RangingMeasurementData_t measure;

      // Serial.print("Reading a measurement... ");
      lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

      if (measure.RangeStatus != 4) {  // phase failures have incorrect data
        Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
      } else {
        Serial.println(" out of range ");
      }
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void Task_Flame(void *pvParameters) // This is a task.
{
  
  (void)pvParameters;
  
  pinMode(FlamePin, INPUT);
  pinMode(BuzzPin, OUTPUT);

    while (1) // A Task shall never return or exit.
    {
      if((digitalRead(FlamePin))==1){
        digitalWrite(BuzzPin,HIGH);
      }
      else{
        digitalWrite(BuzzPin,LOW);
      }
      vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}