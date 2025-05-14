#include <Arduino.h>
#include <CaptivePortal.h>

#include "Config.h"
CaptivePortal portal;

void setup() {
  portal.begin("ESP32-Login", "12345678");
}

void loop() {
  portal.handle();
}
