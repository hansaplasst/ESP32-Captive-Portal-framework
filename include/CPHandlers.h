#ifndef CP_HANDLERS_H
#define CP_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>

class CaptivePortal;  // Forward declaration

struct CPContentType {
  const char* textplain = "text/plain; charset=utf-8";
  const char* texthtml = "text/html; charset=utf-8";
};

class CPHandlers {
 public:
  CPHandlers(WebServer* webServer, CaptivePortal* portal);

  /**
   * @brief Sends a styled HTML message to the client with a title and message.
   *
   * @param code HTTP response code
   * @param title Message title
   * @param message Detailed message
   * @param buttonText Text for the action button (default: "Back")
   * @param target HREF target for the button (default: "/")
   */
  void sendMobileMessage(int code, const String& title, const String& message, const String& buttonText = "Back", const String& target = "/");

  String getSessionIdFromCookie();
  bool requireAuth();

  // Route handlers
  void handleRoot();
  void handleLogin();
  void handleUpdateDeviceName();
  void handleUpdatePass();
  void handleHome();
  void handleEdit();
  void handleDevices();
  void handleSystem();
  void handleLogout();
  void handleReboot();
  void handleFactoryReset();
  void handleCaptive();
  void handleFirmwareUpload();
  void handleFirmwareUpdateDone();
  void handleListFiles();
  void handleEditFileGet();
  void handleEditFilePost();
  void handleWiFiScan();
  void handleConfigGet();

  void noCache();  // Sends no-chache headers to a client

 private:
  WebServer* s_webServer;
  CaptivePortal* s_portal;
  CPContentType contentType;
};

#endif  // CP_HANDLERS_H