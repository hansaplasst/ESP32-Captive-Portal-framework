#include "PageRenderer.h"

#include <LittleFS.h>
#include <WebServer.h>  // of ESP32WebServer

String loadFile(fs::LittleFSFS& fileSystem, const String& path) {
  File f = fileSystem.open(path, "r");
  if (!f) return "<h2>404 Not Found</h2>";
  String content = f.readString();
  f.close();
  return content;
}

void streamPageWithMenu(WebServer* server, fs::LittleFSFS& fileSystem,
                        const String& filePath,
                        const String& activeTab,
                        const String& pageTitle) {
  // 1. Begin chunked response
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "text/html", "");

  // 2. Menu en head in een paar kleine stukken
  String menu = loadFile(fileSystem, "/tabmenu.html");
  menu.replace("{home}", activeTab == "home" ? "active" : "");
  menu.replace("{devices}", activeTab == "devices" ? "active" : "");
  menu.replace("{system}", activeTab == "system" ? "active" : "");

  String head = "<!DOCTYPE html><html><head>";
  head += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  head += "<link rel=\"preload\" href=\"/styles.css\" as=\"style\" onload=\"this.rel='stylesheet'\" />";
  head += "<noscript><link rel=\"stylesheet\" href=\"/styles.css\" /></noscript>";
  head += "<title>" + pageTitle + "</title>";
  head += "</head><body>";

  server->sendContent(head);
  server->sendContent(menu);

  // 3. Body file in kleine chunks streamen
  File f = fileSystem.open(filePath, "r");
  if (!f) {
    server->sendContent("<h2>404 Not Found</h2>");
  } else {
    static char buf[512];
    while (int n = f.readBytes(buf, sizeof(buf))) {
      server->sendContent(String(buf, n));
    }
    f.close();
  }

  // 4. Sluit HTML af
  server->sendContent("</body></html>");
  server->sendContent("");  // flush
}
