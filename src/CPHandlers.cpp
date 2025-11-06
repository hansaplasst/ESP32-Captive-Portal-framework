#include "CPHandlers.h"

#include <ArduinoJson.h>
#include <ESPResetUtil.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>

#include "CaptivePortal.h"
#include "Config.h"
#include "PageRenderer.h"

/**
 * @brief Construct a new CPHandlers object
 *
 * @param server Pointer to the WebServer instance
 * @param portal Pointer to the CaptivePortal instance
 */
CPHandlers::CPHandlers(WebServer* server, CaptivePortal* portal) {
  DPRINTF(0, "[CPHandlers::CPHandlers]");
  this->server = server;
  this->portal = portal;
}

/**
 * @brief Sends a styled message page to the client with an optional button.
 */
void CPHandlers::sendMobileMessage(int code, const String& title, const String& message, const String& buttonText, const String& target) {
  DPRINTF(0, "[CPHandlers::sendMobileMessage]");
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link rel='stylesheet' href='/styles.css'>";
  html += "<title>" + title + "</title>";
  html += "</head><body>";
  html += "<div class='container' style='border:1px solid #fca5a5; background:#fef2f2; color:#b91c1c;'>";
  html += "<h2>" + title + "</h2><p>" + message + "</p>";
  html += "<a href='" + target + "' style='display:inline-block; margin-top:20px; padding:10px 20px; background-color:#ef4444; color:white; text-decoration:none; border-radius:5px;'>" + buttonText + "</a>";
  html += "</div></body></html>";
  server->send(code, contentType.texthtml, html);
}

/**
 * @brief Extracts sessionId from the Cookie header.
 *
 * @return sessionId string or empty if not found
 */
String CPHandlers::getSessionIdFromCookie() {
  DPRINTF(0, "[CPHandlers::getSessionIdFromCookie]");
  if (!server->hasHeader("Cookie")) return "";
  String cookie = server->header("Cookie");
  int pos = cookie.indexOf("sessionId=");
  if (pos == -1) return "";
  String sid = cookie.substring(pos + 10);
  int semi = sid.indexOf(';');
  if (semi > 0) sid = sid.substring(0, semi);
  sid.trim();
  return sid;
}

/**
 * @brief Checks if the user is authenticated via session cookie.
 *
 * If not authenticated, redirects to login page.
 *
 * @return true if authenticated
 * @return false if not authenticated (response already sent)
 */
bool CPHandlers::requireAuth() {
  DPRINTF(0, "[CPHandlers::requireAuth]");
  String sid = getSessionIdFromCookie();
  if (!portal->isSessionValid(sid)) {
    DPRINTF(1, "Session invalid or missing, redirecting to login");
    server->sendHeader("Location", "/login");
    server->send(302, "text/plain; charset=utf-8", "Redirecting to login");
    return false;
  }
  return true;
}

/**
 * @brief Serves the login page.
 */
void CPHandlers::handleRoot() {
  DPRINTF(0, "[CPHandlers::handleRoot]");
  server->send(200, contentType.texthtml, loadFile("/login.html"));
}

/**
 * @brief Processes login POST request.
 */
void CPHandlers::handleLogin() {
  DPRINTF(0, "[CPHandlers::handleLogin]");
  if (!server->hasArg("user") || !server->hasArg("pass")) {
    server->send(400, contentType.textplain, "Missing fields");
    return;
  }

  if (server->arg("user") == portal->Settings.AdminUser && server->arg("pass") == portal->Settings.AdminPassword) {
    String sid = portal->createSession();
    DPRINTF(0, "Login successful, creating sessionId: %s", sid.c_str());
    server->sendHeader("Set-Cookie", "sessionId=" + sid + "; Path=/;");
    if (server->arg("pass") == portal->Settings.DefaultPassword) {
      server->send(200, contentType.texthtml, loadFile("/defaultpass_prompt.html"));
    } else {
      server->sendHeader("Location", "/home");
      server->send(302, contentType.textplain, "Redirecting...");
    }
  } else {
    sendMobileMessage(403, "Invalid Login", "Incorrect username or password.");
  }
}

/**
 * @brief Updates admin password and logs out.
 */
void CPHandlers::handleUpdatePass() {
  DPRINTF(0, "[CPHandlers::handleUpdatePass]");
  if (!requireAuth()) return;
  if (!server->hasArg("newpass")) {
    server->send(400, contentType.textplain, "Missing new password");
    return;
  }
  portal->Settings.AdminPassword = server->arg("newpass");
  portal->Settings.save();  // TODO optimize to only update password

  handleLogout();
}

/**
 * @brief Shows the home page if logged in.
 */
void CPHandlers::handleHome() {
  DPRINTF(0, "[CPHandlers::handleHome]");
  if (!requireAuth()) return;
  server->send(200, contentType.texthtml, loadPageWithMenu("/home.html", "home", "Home"));
}

void CPHandlers::handleEdit() {
  DPRINTF(0, "[CPHandlers::handleEdit]");
  if (!requireAuth()) return;
  server->send(200, contentType.texthtml, loadPageWithMenu("/edit.html", "edit", "Edit"));
}

void CPHandlers::handleDevices() {
  DPRINTF(0, "[CPHandlers::handleDevices]");
  if (!requireAuth()) return;
  server->send(200, contentType.texthtml, loadPageWithMenu("/devices.html", "devices", "Devices"));
}

void CPHandlers::handleSystem() {
  DPRINTF(0, "[CPHandlers::handleSystem]");
  if (!requireAuth()) return;
  server->send(200, contentType.texthtml, loadPageWithMenu("/system.html", "system", "System"));
}

/**
 * @brief Logs out the current user.
 */
void CPHandlers::handleLogout() {
  DPRINTF(0, "[CPHandlers::handleLogout]");

  // Remove sessionId from server-side storage
  String sid = getSessionIdFromCookie();
  if (sid.length()) {
    DPRINTF(0, "Removing sessionId: %s", sid.c_str());
    portal->removeSession(sid);
  }

  // Make Client-side cookie invalid
  server->sendHeader("Set-Cookie", "sessionId=deleted; Path=/; Max-Age=0");

  // Disable Cache
  server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server->sendHeader("Pragma", "no-cache");

  // Redirect to login page
  server->sendHeader("Location", "/login");
  server->send(302, contentType.textplain, "Logged out");
}

/**
 * @brief Reboots the device.
 */
void CPHandlers::handleReboot() {
  DPRINTF(0, "[CPHandlers::handleReboot]");
  if (!requireAuth()) return;
  espReset(portal->Settings.LedPin);
}

/**
 * @brief Deletes config and restarts the device.
 */
void CPHandlers::handleFactoryReset() {
  DPRINTF(0, "[CPHandlers::handleFactoryReset]");
  if (!requireAuth()) return;
  handleLogout();
  factoryReset();
}

/**
 * @brief Redirects captive clients to the portal->
 */
void CPHandlers::handleCaptive() {
  DPRINTF(0, "[CPHandlers::handleCaptive]");
  server->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  server->send(302, contentType.textplain, "");
}

/**
 * @brief Handles firmware update via POST to /update.
 */
void CPHandlers::handleFirmwareUpload() {
  DPRINTF(0, "[CPHandlers::handleFirmwareUpload]");
  if (!requireAuth()) return;
  HTTPUpload& upload = server->upload();

  if (upload.status == UPLOAD_FILE_START) {
    DPRINTF(1, "[OTA] Update start: %s", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      DPRINTF(1, "[OTA] Update success: %u bytes", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}
/**
 * @brief Handles firmware update completion.
 */
void CPHandlers::handleFirmwareUpdateDone() {
  DPRINTF(0, "[CPHandlers::handleFirmwareUpdateDone]");
  if (!requireAuth()) return;
  if (Update.hasError()) {
    server->send(500, contentType.textplain, "❌ Update failed!");
  } else {
    server->send(200, contentType.textplain, "✅ Update successful. Rebooting...");
    delay(1000);
    ESP.restart();
  }
}

/**
 * @brief Lists files in LittleFS as a JSON array.
 */
void CPHandlers::handleListFiles() {
  DPRINTF(0, "[CPHandlers::handleListFiles]");
  if (!requireAuth()) return;
  String json = "[";
  File root = LittleFS.open("/");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    bool first = true;
    while (file) {
      if (!first) json += ",";
      json += "\"" + String(file.name()) + "\"";
      first = false;
      file = root.openNextFile();
    }
  }
  json += "]";
  server->send(200, "application/json", json);
}

/**
 * @brief Serves the file editing page.
 */
void CPHandlers::handleEditFileGet() {
  DPRINTF(0, "[CPHandlers::handleEditFileGet]");
  if (!requireAuth()) return;
  if (!server->hasArg("name")) {
    server->send(400, contentType.textplain, "Missing filename");
    return;
  }
  String name = server->arg("name");
  if (!name.startsWith("/")) name = "/" + name;  // <-- fix

  File file = LittleFS.open(name, "r");
  if (!file) {
    server->send(404, contentType.textplain, "File not found");
    return;
  }
  String content = file.readString();
  file.close();
  server->send(200, contentType.textplain, content);
}

/**
 * @brief Handles saving edits to a file.
 */
void CPHandlers::handleEditFilePost() {
  DPRINTF(0, "[CPHandlers::handleEditFilePost]");
  if (!requireAuth()) return;
  if (!server->hasArg("name") || !server->hasArg("content")) {
    server->send(400, contentType.textplain, "Missing params");
    return;
  }
  String name = server->arg("name");
  if (!name.startsWith("/")) name = "/" + name;  // <-- fix

  String content = server->arg("content");
  File file = LittleFS.open(name, "w");
  if (!file) {
    server->send(500, contentType.textplain, "Could not open file for writing");
    return;
  }
  file.print(content);
  file.close();
  server->send(200, contentType.textplain, "File saved!");
}

/**
 * @brief Asynchronous WiFi scan endpoint.
 *
 * GET /wifiscan?start=1  -> starts scan, returns {"status":"started"}
 * GET /wifiscan          -> if running: {"status":"running"}
 *                           if failed:  {"status":"failed"}
 *                           if ready:   [ {ssid, rssi, channel, secure}, ... ]
 */
/**
 * @brief Asynchronous WiFi scan endpoint.
 *
 * GET /wifiscan?start=1  -> starts scan, returns {"status":"started"}
 * GET /wifiscan          -> if running: {"status":"running"}
 *                           if failed:  {"status":"failed"}
 *                           if ready:   [ {ssid, rssi, channel, secure}, ... ]
 */
void CPHandlers::handleWiFiScan() {
  DPRINTF(0, "[CPHandlers::handleWiFiScan]");
  if (!requireAuth()) return;

  // Keep AP alive and ensure STA is enabled for scanning
  // Important: do NOT restore to AP-only while the async scan is running.
  if (WiFi.getMode() != WIFI_MODE_APSTA) {
    WiFi.mode(WIFI_MODE_APSTA);
    DPRINTF(1, "WiFi.mode -> AP+STA");
  }

  // Start a new scan?
  if (server->hasArg("start")) {
    // Clear any previous results to avoid stale reads
    WiFi.scanDelete();

    // Start async scan (show_hidden=false for speed; passive=false; 120ms/chan)
    bool ok = WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/false, /*passive=*/false);
    if (!ok) {
      DPRINTF(2, "WiFi.scanNetworks async start FAILED");
      server->send(200, "application/json", "{\"status\":\"failed\"}");
      return;
    }
    DPRINTF(1, "WiFi.scanNetworks async start OK");
    server->send(200, "application/json", "{\"status\":\"started\"}");
    return;
  }

  // Poll for results
  int r = WiFi.scanComplete();  // >=0: count, -1: running, -2: failed
  if (r == WIFI_SCAN_RUNNING) {
    // Still scanning
    server->send(200, "application/json", "{\"status\":\"running\"}");
    return;
  }
  if (r == WIFI_SCAN_FAILED) {
    // DPRINTF(2, "WiFi.scanComplete -> FAILED"); // Ignore false positives
    server->send(200, "application/json", "{\"status\":\"failed\"}");
    // Keep AP+STA; next start will reuse it
    return;
  }

  // r >= 0 -> results ready
  DPRINTF(1, "WiFi.scanComplete -> %d networks", r);
  String json = "[";
  for (int i = 0; i < r; ++i) {
    if (i) json += ",";
    bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    json += "{";
    json += "\"ssid\":\"" + String(WiFi.SSID(i)) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"channel\":" + String(WiFi.channel(i)) + ",";
    json += "\"secure\":" + String(secure ? "true" : "false");
    json += "}";
  }
  json += "]";

  WiFi.scanDelete();  // free results

  // We blijven in AP+STA; dat is robuuster voor herhaalde scans
  server->send(200, "application/json", json);
}
