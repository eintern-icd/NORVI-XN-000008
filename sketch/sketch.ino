// =====================================================================
//  Simple Blink & Serial Test Code for NORVI-XN-000008
// =====================================================================
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("NORVI-XN-000008 initialized successfully!");
}

void loop() {
  Serial.println("System running...");
  delay(1000);
}
