#ifndef CP_HANDLERS_H
#define CP_HANDLERS_H

#include <Arduino.h>
#include <WebServer.h>

/**
 * @brief Reads username and password from the config file
 *
 * @param user Output reference to store the username
 * @param pass Output reference to store the password
 * @return true if credentials were read successfully
 * @return false if reading failed
 */
bool readUser(String &user, String &pass);

/**
 * @brief Updates the admin password in the config file
 *
 * @param newpass The new password to store
 */
void updatePassword(const String &newpass);

/**
 * @brief Sends a styled HTML message to the client with a title and message.
 *
 * @param code HTTP response code
 * @param title Message title
 * @param message Detailed message
 * @param buttonText Text for the action button (default: "Back")
 * @param target HREF target for the button (default: "/")
 */
void sendMobileMessage(int code, const String &title, const String &message, const String &buttonText = "Back", const String &target = "/");

// Route handlers
void handleRoot();
void handleLogin();
void handleUpdatePass();
void handleHome();
void handleEdit();
void handleDevices();
void handleSystem();
void handleLogout();
void handleReboot();
void handleFactoryReset();
void handleCaptive();

#endif  // CP_HANDLERS_H