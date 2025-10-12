#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

const float BATTERY_CAPACITY_mAh = 1800.0;
unsigned long lastTime = 0;
float totalConsumption_mAh = 0.0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("INA219 Current Sensor Test");

  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) {
      delay(10);
    }
  }
  // ina219.setCalibration_16V_400mA();

  Serial.println("INA219 found!");

  lastTime = millis();
}

void loop() {
   unsigned long startTime = millis();
   float sumCurrent = 0.0;
   int readings = 0;

   while (millis() - startTime < 1000) {
      sumCurrent += ina219.getCurrent_mA();
      readings++;
      delay(100);
   }

   float avgCurrent_mA = sumCurrent / readings;

   if (avgCurrent_mA > 0) {
      totalConsumption_mAh += avgCurrent_mA / 3600.0; // 1 second = 1/3600 hours
   }

   float remaining_mAh = BATTERY_CAPACITY_mAh - totalConsumption_mAh;
   float remaining_percent = (remaining_mAh / BATTERY_CAPACITY_mAh) * 100.0;
   float estimatedHours = (avgCurrent_mA > 0) ? remaining_mAh / avgCurrent_mA : 0;

   Serial.print("Current: ");
   Serial.print(avgCurrent_mA);
   Serial.print(" mA, Consumption: ");
   Serial.print(totalConsumption_mAh);
   Serial.print(" mAh, Remaining: ");
   Serial.print(remaining_mAh);
   Serial.print(" mAh (");
   Serial.print(remaining_percent);
   Serial.print("%), Est. Hours: ");
   Serial.println(estimatedHours);

   lastTime = millis();
}