/*
make the secrets.h file for this project with your network connection and ThingSpeak channel details.
write following content in it:

#define SECRET_SSID "MySSID"		// replace MySSID with your WiFi network name
#define SECRET_PASS "MyPassword"	// replace MyPassword with your WiFi password

#define SECRET_CH_ID 0000000			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "XYZ"   // replace XYZ with your channel write API Key
*/
static SemaphoreHandle_t mutex;

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define FlamePin 2
#define DhtPin 15
#define BuzzPin 23
#define soilMoisture 27
#define gasSensor 25

#include <LiquidCrystal_I2C.h>  // download LCD I2C library first
#include <DHT.h>
#include "Adafruit_VL53L0X.h"
#include <WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h"  // always include thingspeak header file after other header files and custom macros

//first run I2C scanner to know lcd's I2C address
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address to 0x27 for a 16 chars and 2 line display
char ssid[] = SECRET_SSID;           // your network SSID (name)
char pass[] = SECRET_PASS;           // your network password
int keyIndex = 0;                    // your network key Index number (needed only for WEP)

unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;

//whatever fields you have to send to thingspeak, make it global so it doesn't go out of scope
unsigned short dist;      // distance by TOF sensor
unsigned short humidity;  // For DHT11
unsigned char temp;       // For DHT11

DHT dht(DhtPin, DHT11);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();  //TOF and LCD have different I2C address
WiFiClient client;

void IRAM_ATTR FlameIsr();  // Flame Isr declaration.

void setup() {  //setup task runs at priority 1
  pinMode(FlamePin, INPUT);
  pinMode(soilMoisture, INPUT);
  pinMode(gasSensor, INPUT);
  pinMode(BuzzPin, OUTPUT);
  //lcd.begin();
  lcd.init();
  lcd.backlight();
  dht.begin();
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  // Create mutex before starting tasks
  mutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(Task_DHT11, "Task_DHT11", 2024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(Task_TOF, "Task_TOF", 2024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(Task_THINGSPEAK, "Task_THINGSPEAK", 2024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(
    measureMoisture,
    "Measure soil moisture",
    2048,
    NULL,
    1,
    NULL,
    ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    gasDetect,
    "Detect Gases",
    2048,
    NULL,
    1,
    NULL,
    ARDUINO_RUNNING_CORE);
  attachInterrupt(FlamePin, FlameIsr, CHANGE);  // go to isr whenever level changes

  // Delete "setup" task
  vTaskDelete(NULL);
}

void loop() {
  // Delete "loop" task
  vTaskDelete(NULL);
}
void measureMoisture(void *parameter) {

  while (1) {
   // if (soilMoisture < 300) {
      if (xSemaphoreTake(mutex, 0) == 1) {
        Serial.print("Moisture:");
        Serial.println(analogRead(soilMoisture));
        lcd.clear();  // clear dht and tof data to see wifi and thingspeak
        lcd.setCursor(0, 0); /*Set cursor to Row 1*/
        // lcd.print("Moisture:");
        // lcd.print(analogRead(soilMoisture)); /*print text on LCD*/
        lcd.print("Water Pump ON");
        vTaskDelay(5000/ portTICK_PERIOD_MS);  // Wait .5 seconds to clearly see output
        // lcd.clear();                
        xSemaphoreGive(mutex);
      }
    //}
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

//Our task: measure gas detect
void gasDetect(void *parameter) {

  while (1) {
    //if (gasSensor > 300) {
      if (xSemaphoreTake(mutex, 0) == 1) {
        Serial.print("Gas:");
        Serial.println(analogRead(gasSensor));
        lcd.clear();                
        lcd.setCursor(0, 1); /*set cursor on row 2*/
        lcd.print("Windows Opening");
        // lcd.print(analogRead(gasSensor));
        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Wait .5 seconds to clearly see output
        // lcd.clear();                
        xSemaphoreGive(mutex);
      }
    //}
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
void Task_DHT11(void *pvParameters)  // This is a DHT task.
{
  (void)pvParameters;

  while (1)  // A Task shall never return or exit.
  {
    temp = dht.readTemperature();
    humidity = dht.readHumidity();
    // take mutex before entering critical section of lcd
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
      lcd.setCursor(0, 0);
      lcd.print("Tmp ");
      lcd.setCursor(4, 0);
      lcd.print(temp);
      lcd.setCursor(6, 0);
      lcd.print("c  ");
      lcd.setCursor(8, 0);
      lcd.print("hmd ");
      lcd.setCursor(12, 0);
      lcd.print(humidity);
      lcd.setCursor(14, 0);
      lcd.print("%");
      xSemaphoreGive(mutex);  // give back mutex once task has completed work with critical section
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void Task_TOF(void *pvParameters)  // This is a TOF task.
{
  (void)pvParameters;

  if (!lox.begin()) {
    // take mutex before entering critical section of lcd
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Failed to"));
      lcd.setCursor(0, 1);
      lcd.print(F("boot VL53L0X"));
      xSemaphoreGive(mutex);  // give back mutex once task has completed work with critical section
      while (1) { ; }
    }
  }

  while (1)  // A Task shall never return or exit.
  {
    VL53L0X_RangingMeasurementData_t measure;

    lox.rangingTest(&measure, false);  // pass in 'true' to get debug data printout!

    if (measure.RangeStatus != 4) {  // phase failures have incorrect data

      // take mutex before entering critical section of lcd
      if (xSemaphoreTake(mutex, 0) == pdTRUE) {
        lcd.setCursor(0, 1);
        lcd.print("Dist (mm): ");
        dist = measure.RangeMilliMeter;
        lcd.setCursor(12, 1);
        lcd.print(dist);
        xSemaphoreGive(mutex);  // give back mutex once task has completed work with critical section
      }
    } else {
      // take mutex before entering critical section of lcd
      if (xSemaphoreTake(mutex, 0) == pdTRUE) {
        lcd.print(" out of range ");
        xSemaphoreGive(mutex);  // give back mutex once task has completed work with critical section
      }
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void Task_THINGSPEAK(void *pvParameters)  // This is a thingspeak cloud task.
{
  (void)pvParameters;

  while (1) {
    // take mutex before entering critical section of lcd
    // Connect or reconnect to WiFi
    if (WiFi.status() != WL_CONNECTED) {
      if (xSemaphoreTake(mutex, 0) == pdTRUE) {
        lcd.clear();  // clear dht and tof data to see wifi and thingspeak
        lcd.setCursor(0, 0);
        lcd.print("connecting to ID");
        lcd.setCursor(0, 1);
        lcd.print(SECRET_SSID);
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Wait .5 seconds to clearly see output
        lcd.clear();                           // clear lcd again for new data of DHT and TOF
        xSemaphoreGive(mutex);                 // give back mutex once task has completed work with critical section
      }
      while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
      // take mutex before entering critical section of lcd
      if (xSemaphoreTake(mutex, 0) == pdTRUE) {
        lcd.clear();  // clear dht and tof data to see wifi and thingspeak
        lcd.setCursor(0, 0);
        lcd.print("Connected.");
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Wait .5 seconds to clearly see output
        lcd.clear();                           // clear lcd again for new data of DHT and TOF
        xSemaphoreGive(mutex);                 // give back mutex once task has completed work with critical section
      }
    }

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, dist);

    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      // take mutex before entering critical section of lcd
      if (xSemaphoreTake(mutex, 0) == pdTRUE) {
        lcd.clear();  // clear dht and tof data to see wifi and thingspeak
        lcd.setCursor(0, 0);
        lcd.print("Channel updated");
        lcd.setCursor(0, 1);
        lcd.print("successfully.");
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Wait .5 seconds to clearly see output
        lcd.clear();                           // clear lcd again for new data of DHT and TOF
        xSemaphoreGive(mutex);                 // give back mutex once task has completed work with critical section
      }
    } else {
      // take mutex before entering critical section of lcd
      if (xSemaphoreTake(mutex, 0) == pdTRUE) {
        lcd.clear();  // clear dht and tof data to see wifi and thingspeak
        lcd.setCursor(0, 0);
        lcd.print("Problem updating");
        lcd.setCursor(0, 1);
        lcd.print("channel.code" + String(x));
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Wait .5 seconds to clearly see output
        lcd.clear();                           // clear lcd again for new data of DHT and TOF
        xSemaphoreGive(mutex);                 // give back mutex once task has completed work with critical section
      }
    }

    vTaskDelay(20000 / portTICK_PERIOD_MS);  // Wait 20 seconds to update the channel again
  }
}


void IRAM_ATTR FlameIsr()  // flame isr that is to be executed
{
  if ((digitalRead(FlamePin)) == 1) {
    digitalWrite(BuzzPin, HIGH);
  } else {
    digitalWrite(BuzzPin, LOW);
  }
}