#include "CPHandlers.h"

#include <ArduinoJson.h>
#include <ESPResetUtil.h>
#include <LittleFS.h>

#include "Config.h"
#include "PageRenderer.h"

extern WebServer server;
extern String loadFile(const String &path);
extern String loadPageWithMenu(const String &filePath, const String &activeTab, const String &pageTitle);
extern void updatePassword(const String &newpass);
extern bool readUser(String &user, String &pass);

/**
 * @brief Reads username and password from config.json.
 *
 * @param user Output reference for username
 * @param pass Output reference for password
 * @return true if read successfully
 * @return false if file not found or parse error
 */
bool readUser(String &user, String &pass) {
  File f = LittleFS.open(CONFIG_FILE, "r");
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
void updatePassword(const String &newpass) {
  File f = LittleFS.open(CONFIG_FILE, "r");
  if (!f) return;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;
  doc["user"]["pass"] = newpass;
  f = LittleFS.open(CONFIG_FILE, "w");
  serializeJson(doc, f);
  f.close();
}

/// Whether the user is currently authenticated
bool isAuthenticated = false;

/**
 * @brief Sends a styled message page to the client with an optional button.
 */
void sendMobileMessage(int code, const String &title, const String &message, const String &buttonText, const String &target) {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link rel='stylesheet' href='/styles.css'>";
  html += "<title>" + title + "</title>";
  html += "</head><body>";
  html += "<div class='container' style='border:1px solid #fca5a5; background:#fef2f2; color:#b91c1c;'>";
  html += "<h2>" + title + "</h2><p>" + message + "</p>";
  html += "<a href='" + target + "' style='display:inline-block; margin-top:20px; padding:10px 20px; background-color:#ef4444; color:white; text-decoration:none; border-radius:5px;'>" + buttonText + "</a>";
  html += "</div></body></html>";
  server.send(code, "text/html", html);
}

/**
 * @brief Serves the login page.
 */
void handleRoot() {
  server.send(200, "text/html", loadFile("/login.html"));
}

/**
 * @brief Processes login POST request.
 */
void handleLogin() {
  if (!server.hasArg("user") || !server.hasArg("pass")) {
    server.send(400, "text/plain", "Missing fields");
    return;
  }

  String u, p;
  if (!readUser(u, p)) {
    server.send(500, "text/plain", "Could not read user");
    return;
  }

  if (server.arg("user") == u && server.arg("pass") == p) {
    isAuthenticated = true;
    if (u == ADMIN_USER && p == ADMIN_PASSWORD) {
      server.send(200, "text/html", loadFile("/defaultpass_prompt.html"));
    } else {
      server.sendHeader("Location", "/home");
      server.send(302);
    }
  } else {
    sendMobileMessage(403, "Invalid Login", "Incorrect username or password.");
  }
}

/**
 * @brief Updates admin password and logs out.
 */
void handleUpdatePass() {
  if (!server.hasArg("newpass")) {
    server.send(400, "text/plain", "Missing new password");
    return;
  }
  updatePassword(server.arg("newpass"));
  isAuthenticated = false;
  server.sendHeader("Location", "/");
  server.send(302);
}

/**
 * @brief Shows the home page if logged in.
 */
void handleHome() {
  if (!isAuthenticated) {
    sendMobileMessage(403, "Access Denied", "You must be logged in.");
    return;
  }
  server.send(200, "text/html", loadPageWithMenu("/home.html", "home", "Home"));
}

void handleEdit() {
  server.send(200, "text/html", loadPageWithMenu("/edit.html", "edit", "Edit"));
}

void handleDevices() {
  server.send(200, "text/html", loadPageWithMenu("/devices.html", "devices", "Devices"));
}

void handleSystem() {
  server.send(200, "text/html", loadPageWithMenu("/system.html", "system", "System"));
}

/**
 * @brief Logs out the current user.
 */
void handleLogout() {
  isAuthenticated = false;
  server.sendHeader("Location", "/");
  server.send(302);
}

/**
 * @brief Reboots the device.
 */
void handleReboot() {
  espReset();
}

/**
 * @brief Deletes config and restarts the device.
 */
void handleFactoryReset() {
  handleLogout();
  factoryReset();
}

/**
 * @brief Redirects captive clients to the portal.
 */
void handleCaptive() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  server.send(302, "text/plain", "");
}