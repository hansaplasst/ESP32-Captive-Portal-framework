#include <Arduino.h>
#include <CaptivePortal.h>

#include "Config.h"
CaptivePortal *portal = nullptr;

void setup() {
  portal = new CaptivePortal();
  portal->begin("ESP32-Login", "12345678");
}

void loop() {
  if (portal) {
    portal->handle();
  }
}
