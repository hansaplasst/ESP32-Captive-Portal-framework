#include "PageRenderer.h"
#include <LittleFS.h>

#define FSYS LittleFS

String loadFile(const String &path) {
  File f = FSYS.open(path, "r");
  if (!f) return "<h2>404 Not Found</h2>";
  String content = f.readString();
  f.close();
  return content;
}

String loadPageWithMenu(const String &filePath, const String &activeTab, const String &pageTitle) {
  String menu = loadFile("/tabmenu.html");
  menu.replace("{home}", activeTab == "home" ? "active" : "");
  menu.replace("{edit}", activeTab == "edit" ? "active" : "");
  menu.replace("{devices}", activeTab == "devices" ? "active" : "");
  menu.replace("{system}", activeTab == "system" ? "active" : "");

  String body = loadFile(filePath);

  String fullPage = "<!DOCTYPE html><html><head>";
  fullPage += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  fullPage += "<link rel=\"stylesheet\" href=\"/styles.css\">";
  fullPage += "<title>" + pageTitle + "</title>";
  fullPage += "</head><body>" + menu + body + "</body></html>";

  return fullPage;
}
