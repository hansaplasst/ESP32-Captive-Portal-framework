#include <Arduino.h>
#include <CaptivePortal.h>
#include <LittleFS.h>
#include <dprintf.h>

#include "Config.h"

fs::LittleFSFS configFS;
CaptivePortalConfig config(configFS);
CaptivePortal* portal = nullptr;

void setup() {
  Serial.begin(115200);
  delay(3000);

  config.begin();
  portal = new CaptivePortal(config);
  portal->begin();

  config.checkFactoryResetMarker();  // Remover the marker file after factory reset
}

void loop() {
  if (portal) {
    portal->handle();
  }
}
