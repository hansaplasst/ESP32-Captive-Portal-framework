#ifndef PAGE_RENDERER_H
#define PAGE_RENDERER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
/**
 * @brief Loads the contents of a file from the filesystem.
 *
 * This function reads the contents of the given file path from LittleFS.
 * If the file does not exist or cannot be opened, a fallback HTML error message is returned.
 *
 * @param path The path to the file (e.g. "/home.html")
 * @return A string containing the file contents, or an error message if failed.
 */
String loadFile(fs::LittleFSFS& fileSystem, const String& path);

/**
 * @brief Streams a full HTML page with a navigation menu and dynamic title.
 *
 * This function inserts a tab menu loaded from "/tabmenu.html", replaces the active tab,
 * inserts the body loaded from the specified HTML file, and streams it in complete HTML markup.
 *
 * @param filePath Path to the HTML file to load into the <body>
 * @param activeTab The tab to highlight as active ("home", "edit", "devices", "system")
 * @param pageTitle Title to be used in the <title> tag
 * @return A complete HTML page as a string
 */
void streamPageWithMenu(WebServer* server, fs::LittleFSFS& fileSystem,
                        const String& filePath,
                        const String& activeTab,
                        const String& pageTitle);

#endif  // PAGE_RENDERER_H