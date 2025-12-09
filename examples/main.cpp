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

  portal = new CaptivePortal(config);
  portal->begin();

  config.checkFactoryResetMarker();  // Remover the marker file after factory reset
  // portal->checkFactoryResetMarker(); // Never found since portal->webFileSystem is never reset
}

void loop() {
  if (portal) {
    portal->handle();
  }
}
