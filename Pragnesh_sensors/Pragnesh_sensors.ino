/*
make the secrets.h file for this project with your network connection and ThingSpeak channel details.
write following content in it:

#define SECRET_SSID "MySSID"		// replace MySSID with your WiFi network name
#define SECRET_PASS "MyPassword"	// replace MyPassword with your WiFi password

#define SECRET_CH_ID 0000000			// replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "XYZ"   // replace XYZ with your channel write API Key
*/

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define TS_ENABLE_SSL  // For HTTPS SSL connection
#define FlamePin 2
#define DhtPin 15
#define BuzzPin 23

#include <LiquidCrystal_I2C.h>  // download LCD I2C library first
#include <DHT.h>
#include "Adafruit_VL53L0X.h"
#include <WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h"  // always include thingspeak header file after other header files and custom macros

//first run I2C scanner to know lcd's I2C address
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address to 0x27 for a 16 chars and 2 line display
int count = 0;                       // lcd demo counter
char ssid[] = SECRET_SSID;           // your network SSID (name)
char pass[] = SECRET_PASS;           // your network password
int keyIndex = 0;                    // your network key Index number (needed only for WEP)

unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;

//whatever fields you have to send to thingspeak, make it global so it doesn't go out of scope
short dist;      // distance by TOF sensor
float temp;      // For DHT11
float humidity;  // For DHT11

DHT dht(DhtPin, DHT11);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();  //TOF and LCD have different I2C address
WiFiClient client;

void IRAM_ATTR FlameIsr();  // Flame Isr declaration.

void setup() {  //setup task runs at priority 1
  pinMode(FlamePin, INPUT);
  pinMode(BuzzPin, OUTPUT);
  lcd.begin();
  dht.begin();
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  Serial.begin(300);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  // wait until serial port opens
  while (!Serial) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  xTaskCreatePinnedToCore(Task_DHT11, "Task_DHT11", 1024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(Task_TOF, "Task_TOF", 2024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(Task_LCD, "Task_LCD", 2024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(Task_THINGSPEAK, "Task_THINGSPEAK", 5024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  attachInterrupt(FlamePin, FlameIsr, CHANGE);  // go to isr whenever level changes

  // Delete "setup" task
  vTaskDelete(NULL);
}

void loop() {
  // Delete "loop" task
  vTaskDelete(NULL);
}

void Task_DHT11(void *pvParameters)  // This is a DHT task.
{
  (void)pvParameters;

  while (1)  // A Task shall never return or exit.
  {
    temp = dht.readTemperature();
    humidity = dht.readHumidity();
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print("c  ");
    Serial.print("humidty: ");
    Serial.print(humidity);
    Serial.println("%");

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void Task_TOF(void *pvParameters)  // This is a TOF task.
{
  (void)pvParameters;

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while (1) { ; }
  }

  while (1)  // A Task shall never return or exit.
  {
    VL53L0X_RangingMeasurementData_t measure;

    lox.rangingTest(&measure, false);  // pass in 'true' to get debug data printout!

    if (measure.RangeStatus != 4) {  // phase failures have incorrect data
      Serial.print("Distance (mm): ");
      dist = measure.RangeMilliMeter;
      Serial.println(dist);

    } else {
      Serial.println(" out of range ");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void Task_LCD(void *pvParameters)  // This is an LCD task.
{
  (void)pvParameters;

  while (1)  // A Task shall never return or exit.
  {
    lcd.clear();  // clear previous values from screen
    lcd.print("LCD demo");
    lcd.setCursor(0, 1);
    lcd.print("Counting:");
    lcd.setCursor(11, 1);
    lcd.print(count);
    count++;
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void Task_THINGSPEAK(void *pvParameters)  // This is a task.
{
  (void)pvParameters;

  while (1) {
    // Connect or reconnect to WiFi
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(SECRET_SSID);
      while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        Serial.print(".");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
      Serial.println("\nConnected.");
    }

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, dist);

    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
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