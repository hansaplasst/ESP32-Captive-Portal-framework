#include "CPHandlers.h"

#include <ArduinoJson.h>
#include <ESPResetUtil.h>
#include <LittleFS.h>
#include <Update.h>

#include "CaptivePortal.h"
#include "Config.h"
#include "PageRenderer.h"

/**
 * @brief Construct a new CPHandlers object
 *
 * @param server Pointer to the WebServer instance
 * @param portal Pointer to the CaptivePortal instance
 */
CPHandlers::CPHandlers(WebServer *server, CaptivePortal *portal) {
  DPRINTF(0, "[CPHandlers::CPHandlers]");
  this->server = server;
  this->portal = portal;
}

/**
 * @brief Reads username and password from config.json.
 *
 * @param user Output reference for username
 * @param pass Output reference for password
 * @return true if read successfully
 * @return false if file not found or parse error
 */
bool CPHandlers::readUser(String &user, String &pass) {
  DPRINTF(0, "[CPHandlers::readUser]");
  File f = LittleFS.open(portal->Settings.ConfigFile, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  user = doc["user"]["name"].as<String>();
  pass = doc["user"]["pass"].as<String>();
  DPRINTF(1, "User: %s, Pass: %s\n", user.c_str(), pass.c_str());
  return true;
}

/**
 * @brief Updates the admin password in config.json.
 *
 * @param newpass New password to store
 */
void CPHandlers::updatePassword(const String &newpass) {
  DPRINTF(0, "[CPHandlers::updatePassword]");
  File f = LittleFS.open(portal->Settings.ConfigFile, "r");
  if (!f) return;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;
  doc["user"]["pass"] = newpass;
  f = LittleFS.open(portal->Settings.ConfigFile, "w");
  serializeJson(doc, f);
  f.close();
}

/**
 * @brief Sends a styled message page to the client with an optional button.
 */
void CPHandlers::sendMobileMessage(int code, const String &title, const String &message, const String &buttonText, const String &target) {
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
  DPRINTF(0, "Extracted sessionId: %s\n", sid.c_str());
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

  String u, p;
  if (!readUser(u, p)) {
    server->send(500, contentType.textplain, "Could not read user");
    return;
  }

  if (server->arg("user") == u && server->arg("pass") == p) {
    if (u == portal->Settings.AdminUser && p == portal->Settings.AdminPassword) {
      server->send(200, contentType.texthtml, loadFile("/defaultpass_prompt.html"));
    } else {
      String sid = portal->createSession();
      DPRINTF(0, "Login successful, creating sessionId: %s", sid.c_str());
      server->sendHeader("Set-Cookie", "sessionId=" + sid + "; Path=/;");
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
  if (!server->hasArg("newpass")) {
    server->send(400, contentType.textplain, "Missing new password");
    return;
  }
  updatePassword(server->arg("newpass"));
  handleLogout();
}

/**
 * @brief Shows the home page if logged in.
 */
void CPHandlers::handleHome() {
  DPRINTF(0, "[CPHandlers::handleHome]");

  int numHeaders = server->headers();
  for (int i = 0; i < numHeaders; i++) {
    DPRINTF(0, "Header[%d]: %s = %s\n", i, server->headerName(i).c_str(), server->header(i).c_str());
  }

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
  portal->removeSession(getSessionIdFromCookie());
  server->sendHeader("Set-Cookie", "sessionId=; Path=/; Max-Age=0");
  server->sendHeader("Location", "/login");
  server->send(302, contentType.textplain, "Logged out");
}

/**
 * @brief Reboots the device.
 */
void CPHandlers::handleReboot() {
  DPRINTF(0, "[CPHandlers::handleReboot]");
  espReset();
}

/**
 * @brief Deletes config and restarts the device.
 */
void CPHandlers::handleFactoryReset() {
  DPRINTF(0, "[CPHandlers::handleFactoryReset]");
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
  HTTPUpload &upload = server->upload();

  if (upload.status == UPLOAD_FILE_START) {
    DPRINTF(1, "[OTA] Update start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      DPRINTF(1, "[OTA] Update success: %u bytes\n", upload.totalSize);
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