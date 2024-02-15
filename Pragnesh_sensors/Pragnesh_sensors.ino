/*

*/
#include <DHT.h>
#define dhtPin 4

DHT dht(dhtPin,DHT11);

void setup() {
  dht.begin();
  delay(2000);
  Serial.begin(115200);
}

// the loop function runs over and over again forever
void loop() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" c");
  Serial.print("humidty: ");
  Serial.print(humidity);
  Serial.println(" %");
  delay(2000);
}