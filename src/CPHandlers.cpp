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
 * @param webServer Pointer to the WebServer instance
 * @param portal Pointer to the CaptivePortal instance
 */
CPHandlers::CPHandlers(WebServer* webServer, CaptivePortal* portal) : s_webServer(webServer), s_portal(portal) {
  DPRINTF(0, "[CPHandlers::CPHandlers]");
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
  s_webServer->send(code, contentType.texthtml, html);
}

/**
 * @brief Extracts sessionId from the Cookie header.
 *
 * @return sessionId string or empty if not found
 */
String CPHandlers::getSessionIdFromCookie() {
  DPRINTF(0, "[CPHandlers::getSessionIdFromCookie]");
  if (!s_webServer->hasHeader("Cookie")) return "";
  String cookie = s_webServer->header("Cookie");
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
  if (!s_portal->isSessionValid(sid)) {
    DPRINTF(1, "Session invalid or missing, redirecting to login");
    s_webServer->sendHeader("Location", "/login");
    s_webServer->send(302, "text/plain; charset=utf-8", "Redirecting to login");
    return false;
  }
  return true;
}

/**
 * @brief Serves the login page.
 */
void CPHandlers::handleRoot() {
  DPRINTF(0, "[CPHandlers::handleRoot]");
  s_webServer->send(200, contentType.texthtml, loadFile(s_portal->getWebFileSystem(), "/login.html"));
}

/**
 * @brief Processes login POST request.
 */
void CPHandlers::handleLogin() {
  DPRINTF(0, "[CPHandlers::handleLogin]");
  if (!s_webServer->hasArg("user") || !s_webServer->hasArg("pass")) {
    s_webServer->send(400, contentType.textplain, "Missing fields");
    return;
  }

  if (s_webServer->arg("user") == s_portal->Settings.AdminUser && s_webServer->arg("pass") == s_portal->Settings.AdminPassword) {
    String sid = s_portal->createSession();
    DPRINTF(0, "Login successful, creating sessionId: %s", sid.c_str());
    s_webServer->sendHeader("Set-Cookie", "sessionId=" + sid + "; Path=/;");
    if (s_webServer->arg("pass") == s_portal->Settings.DefaultPassword) {
      s_webServer->send(200, contentType.texthtml, loadFile(s_portal->getWebFileSystem(), "/defaultpass_prompt.html"));
    } else {
      s_webServer->sendHeader("Location", "/home");
      s_webServer->send(302, contentType.textplain, "Redirecting...");
    }
  } else {
    sendMobileMessage(403, "Invalid Login", "Incorrect username or password.");
  }
}

/**
 * @brief Updates the device name.
 */
void CPHandlers::handleUpdateDeviceName() {
  DPRINTF(1, "[CPHandlers::handleUpdateDeviceName]");

  if (!requireAuth()) return;

  String body = s_webServer->arg("plain");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    s_webServer->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String name = doc["name"] | "";
  name.trim();

  // if (name.length() == 0) {
  //   s_webServer->send(400, "application/json", "{\"error\":\"Device name required\"}");
  //   return;
  // }

  if (!s_portal->Settings.setDeviceName(name)) {
    s_webServer->send(500, "application/json", "{\"error\":\"Failed to save\"}");
    return;
  }

  noCache();
  s_webServer->send(200, "application/json", "{\"status\":\"ok\"}");
}

/**
 * @brief Updates admin password and logs out.
 */
void CPHandlers::handleUpdatePass() {
  DPRINTF(0, "[CPHandlers::handleUpdatePass]");
  if (!requireAuth()) return;
  if (!s_webServer->hasArg("newpass")) {
    s_webServer->send(400, contentType.textplain, "Missing new password");
    return;
  }

  if (s_webServer->arg("newpass").length() < 8) {
    s_webServer->send(400, "text/plain", "Password must be at least 8 characters.");
    return;
  }

  s_portal->Settings.AdminPassword = s_webServer->arg("newpass");
  s_portal->Settings.save();

  handleLogout();
}

/**
 * @brief Shows the home page if logged in.
 */
void CPHandlers::handleHome() {
  DPRINTF(0, "[CPHandlers::handleHome]");
  if (!requireAuth()) return;
  streamPageWithMenu(s_webServer, s_portal->getWebFileSystem(), "/home.html", "home", "Home");
}

void CPHandlers::handleEdit() {
  DPRINTF(0, "[CPHandlers::handleEdit]");
  if (!requireAuth()) return;
  streamPageWithMenu(s_webServer, s_portal->getWebFileSystem(), "/edit.html", "edit", "Edit");
}

void CPHandlers::handleDevices() {
  DPRINTF(0, "[CPHandlers::handleDevices]");
  if (!requireAuth()) return;
  noCache();
  streamPageWithMenu(s_webServer, s_portal->getWebFileSystem(), "/devices.html", "devices", "Devices");
}

void CPHandlers::handleSystem() {
  DPRINTF(0, "[CPHandlers::handleSystem]");
  if (!requireAuth()) return;
  streamPageWithMenu(s_webServer, s_portal->getWebFileSystem(), "/system.html", "system", "System");
}

/**
 * @brief Logs out the current user.
 */
void CPHandlers::handleLogout() {
  DPRINTF(0, "[CPHandlers::handleLogout]");

  // Remove sessionId from webServer-side storage
  String sid = getSessionIdFromCookie();
  if (sid.length()) {
    DPRINTF(0, "Removing sessionId: %s", sid.c_str());
    s_portal->removeSession(sid);
  }

  // Make Client-side cookie invalid
  s_webServer->sendHeader("Set-Cookie", "sessionId=deleted; Path=/; Max-Age=0");

  // Disable Cache
  s_webServer->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  s_webServer->sendHeader("Pragma", "no-cache");

  // Redirect to login page
  s_webServer->sendHeader("Location", "/login");
  s_webServer->send(302, contentType.textplain, "Logged out");
}

/**
 * @brief Reboots the device.
 */
void CPHandlers::handleReboot() {
  DPRINTF(0, "[CPHandlers::handleReboot]");
  if (!requireAuth()) return;
  espResetUtil::espReset(s_portal->Settings.LedPin);
}

/**
 * @brief Deletes config and restarts the device.
 */
void CPHandlers::handleFactoryReset() {
  DPRINTF(0, "[CPHandlers::handleFactoryReset]");
  if (!requireAuth()) return;
  handleLogout();
  s_portal->Settings.resetToFactoryDefault();
}

/**
 * @brief Redirects captive clients to the portal->
 */
void CPHandlers::handleCaptive() {
  DPRINTF(0, "[CPHandlers::handleCaptive]");

  // DPRINTF(1, "URI: %s", _webServer->uri().c_str());

  s_webServer->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  s_webServer->send(302, contentType.textplain, "");
}

/**
 * @brief Handles firmware update via POST to /update.
 */
void CPHandlers::handleFirmwareUpload() {
  DPRINTF(0, "[CPHandlers::handleFirmwareUpload]");
  if (!requireAuth()) return;
  HTTPUpload& upload = s_webServer->upload();

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
    s_webServer->send(500, contentType.textplain, "Update failed!");
  } else {
    s_webServer->send(200, contentType.textplain, "Update successful. Rebooting...");
    delay(3000);
    ESP.restart();
  }
}

/**
 * @brief Lists files in FSYS as a JSON array.
 */
void CPHandlers::handleListFiles() {
  DPRINTF(0, "[CPHandlers::handleListFiles]");
  if (!requireAuth()) return;
  String json = "[";
  File root = s_portal->getSettingsFileSystem().open("/");
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
  noCache();
  s_webServer->send(200, "application/json", json);
}

/**
 * @brief Serves the file editing page.
 */
void CPHandlers::handleEditFileGet() {
  DPRINTF(0, "[CPHandlers::handleEditFileGet]");
  if (!requireAuth()) return;
  if (!s_webServer->hasArg("name")) {
    s_webServer->send(400, contentType.textplain, "Missing filename");
    return;
  }
  String name = s_webServer->arg("name");
  if (!name.startsWith("/")) name = "/" + name;  // <-- fix

  File file = s_portal->getSettingsFileSystem().open(name, "r");
  if (!file) {
    s_webServer->send(404, contentType.textplain, "File not found");
    return;
  }
  String content = file.readString();
  file.close();
  noCache();
  s_webServer->send(200, contentType.textplain, content);
}

/**
 * @brief Handles saving edits to a file.
 */
void CPHandlers::handleEditFilePost() {
  DPRINTF(0, "[CPHandlers::handleEditFilePost]");
  if (!requireAuth()) return;
  if (!s_webServer->hasArg("name") || !s_webServer->hasArg("content")) {
    s_webServer->send(400, contentType.textplain, "Missing params");
    return;
  }
  String name = s_webServer->arg("name");
  if (!name.startsWith("/")) name = "/" + name;  // <-- fix

  String content = s_webServer->arg("content");
  File file = s_portal->getSettingsFileSystem().open(name, "w");
  if (!file) {
    s_webServer->send(500, contentType.textplain, "Could not open file for writing");
    return;
  }
  file.print(content);
  file.close();
  noCache();
  s_webServer->send(200, contentType.textplain, "File saved!");
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
  if (s_webServer->hasArg("start")) {
    // Clear any previous results to avoid stale reads
    WiFi.scanDelete();

    // Start async scan (show_hidden=false for speed; passive=false; 120ms/chan)
    bool ok = WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/false, /*passive=*/false);
    if (!ok) {
      DPRINTF(2, "WiFi.scanNetworks async start FAILED");
      s_webServer->send(200, "application/json", "{\"status\":\"failed\"}");
      return;
    }
    DPRINTF(1, "WiFi.scanNetworks async start OK");
    s_webServer->send(200, "application/json", "{\"status\":\"started\"}");
    return;
  }

  // Poll for results
  int r = WiFi.scanComplete();  // >=0: count, -1: running, -2: failed
  if (r == WIFI_SCAN_RUNNING) {
    // Still scanning
    s_webServer->send(200, "application/json", "{\"status\":\"running\"}");
    return;
  }
  if (r == WIFI_SCAN_FAILED) {
    // DPRINTF(2, "WiFi.scanComplete -> FAILED"); // Ignore false positives
    s_webServer->send(200, "application/json", "{\"status\":\"failed\"}");
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
  s_webServer->send(200, "application/json", json);
}

/**
 * @brief Returns current configuration as JSON.
 */
void CPHandlers::handleConfigGet() {
  DPRINTF(0, "[CPHandlers::handleConfigGet]");
  if (!requireAuth()) return;

  JsonDocument doc;
  doc["name"] = s_portal->Settings.DeviceName;
  doc["hostname"] = s_portal->Settings.DeviceHostname;

  String json;
  serializeJson(doc, json);

  noCache();
  s_webServer->send(200, "application/json", json);
}

/**
 * @brief sends no-caching headers to a client
 */
void CPHandlers::noCache() {
  DPRINTF(0, "CPHandlers::noCache");
  s_webServer->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  s_webServer->sendHeader("Pragma", "no-cache");
  s_webServer->sendHeader("Expires", "0");
}