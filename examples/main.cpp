#include <Arduino.h>
#include <CaptivePortal.h>

#include "Config.h"
CaptivePortal* portal = nullptr;

void setup() {
  Serial.begin(115200);
  portal = new CaptivePortal();
  portal->begin();
}

void loop() {
  if (portal) {
    portal->handle();
  }
}
