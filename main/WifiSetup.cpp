#include "WiFiSetup.h"

const char* apSSID = "DEEJ";
DNSServer dnsServer;
WebServer server(80);
String scannedNetworks = "";
bool wifiSetupDone = false;
bool useWifi = true;
bool inWifiSetupMode = false; // Initially false

// Forward declarations
void handleRoot();
void handleScan();
void handleConnectPrompt();
void handleConnect();
void handleConfigPage();
void handleFileUploadPost();
void handleFileUpload();
void handleWiFiSettingsPage();
void handleEnableWiFi();
void handleDisableWiFi();

void displayMessage(String line1, String line2, String line3) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, line1.c_str());
    u8g2.drawStr(0, 35, line2.c_str());
    u8g2.drawStr(0, 55, line3.c_str());
    u8g2.sendBuffer();
    Serial.println(line1 + " | " + line2 + " | " + line3);
}

// Save WiFi credentials to SPIFFS (including useWifi)
void saveWiFiCredentials(const char* ssid, const char* password) {
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["ssid"] = ssid;
    jsonDoc["password"] = password;
    jsonDoc["useWifi"] = useWifi;

    File file = SPIFFS.open("/wifi_config.json", "w");
    if (file) {
        serializeJson(jsonDoc, file);
        file.close();
        Serial.println("WiFi credentials saved.");
    } else {
        Serial.println("Failed to save WiFi credentials.");
    }
}

// Save only the useWifi setting without altering credentials
void saveWiFiSetting(bool enabled) {
    String ssid, password;
    loadWiFiCredentials(ssid, password); // Load existing

    useWifi = enabled;
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["ssid"] = ssid;
    jsonDoc["password"] = password;
    jsonDoc["useWifi"] = useWifi;

    File file = SPIFFS.open("/wifi_config.json", "w");
    if (file) {
        serializeJson(jsonDoc, file);
        file.close();
        Serial.printf("WiFi setting changed to %s.\n", useWifi ? "enabled" : "disabled");
    } else {
        Serial.println("Failed to save WiFi setting.");
    }
}

// Load WiFi credentials from SPIFFS
bool loadWiFiCredentials(String &ssid, String &password) {
    if (!SPIFFS.exists("/wifi_config.json")) return false;

    File file = SPIFFS.open("/wifi_config.json", "r");
    StaticJsonDocument<256> jsonDoc;
    if (deserializeJson(jsonDoc, file) == DeserializationError::Ok) {
        ssid = jsonDoc["ssid"] | "";
        password = jsonDoc["password"] | "";
        useWifi = jsonDoc["useWifi"] | true; // Default to true if not found
        file.close();
        return true;
    }
    file.close();
    return false;
}

void startWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/connect", HTTP_GET, handleConnectPrompt);
    server.on("/connect", HTTP_POST, handleConnect);
    server.on("/config", HTTP_GET, handleConfigPage);
    server.on("/upload", HTTP_POST, handleFileUploadPost, handleFileUpload);

    // WiFi settings pages
    server.on("/wifi_settings", HTTP_GET, handleWiFiSettingsPage);
    server.on("/enable_wifi", HTTP_POST, handleEnableWiFi);
    server.on("/disable_wifi", HTTP_POST, handleDisableWiFi);

    server.begin();
    Serial.println("Web server started");
}

void startAccessPoint() {
    WiFi.softAP(apSSID);
    // Apply TX power control if enabled
    if (useTxPowerControl) {
        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        Serial.println("TX power control applied: 8.5 dBm");
    } else {
        Serial.println("TX power control disabled.");
    }
    dnsServer.start(53, "", WiFi.softAPIP());

    startWebServer();
    Serial.println("AP Mode Started");
    delay(1000);
    displayMessage("To configure Device", "connect to DEEJ", "and visit 192.168.4.1");
}

// startWifiSetupMode: enters AP mode without altering useWifi setting.
// Also sets inWifiSetupMode = true to stop DeejControl loops.
void startWifiSetupMode() {
    inWifiSetupMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID);
    // Apply TX power control if enabled
    if (useTxPowerControl) {
        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        Serial.println("TX power control applied: 8.5 dBm");
    } else {
        Serial.println("TX power control disabled.");
    }
    dnsServer.start(53, "", WiFi.softAPIP());
    startWebServer(); 
    Serial.println("AP Mode Started");

    delay(1000);
    displayMessage("To configure device", "connect to DEEJ", "and visit 192.168.4.1");
}

// A function to return a consistent HTML header with dark mode styling
String htmlHeader(const String &title) {
    String h = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    h += "<title>" + title + "</title>";
    h += "<style>";
    // Dark mode CSS styling
    h += "body { font-family: Arial, sans-serif; background: #1e1e1e; color: #ddd; margin: 0; padding: 20px; }";
    h += "h1, h2, h3 { color: #fff; }";
    h += "a { color: #58a6ff; text-decoration: none; }";
    h += "a:hover { text-decoration: underline; }";
    h += "button, input[type='submit'] { background: #444; color: #fff; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; }";
    h += "button:hover, input[type='submit']:hover { background: #333; }";
    h += "input[type='password'], input[type='file'] { width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #666; border-radius: 5px; background: #2d2d2d; color: #ccc; }";
    h += "form { background: #2d2d2d; padding: 20px; border-radius: 5px; max-width: 400px; margin: 20px auto; }";
    h += "ul { list-style: none; padding: 0; }";
    h += "li { background: #2d2d2d; margin: 5px 0; padding: 10px; border-radius: 5px; }";
    h += ".container { max-width: 600px; margin: auto; }";
    h += ".nav { margin-bottom: 20px; }";
    h += ".nav a { margin-right: 10px; }";
    h += "</style></head><body><div class='container'>";
    return h;
}

// A function to return a consistent HTML footer
String htmlFooter() {
    return "</div></body></html>";
}

// Handle the root page
void handleRoot() {
    String html = htmlHeader("WiFi Setup");
    html += "<h1>WiFi Setup</h1>";
    html += "<div class='nav'><a href='/scan'>Scan Networks</a> | <a href='/config'>Upload Slider Config</a> | <a href='/wifi_settings'>WiFi Settings</a></div>";
    if (scannedNetworks != "") {
        html += "<h3>Available Networks</h3><ul>" + scannedNetworks + "</ul>";
    } else {
        html += "<p>Click 'Scan Networks' to see available WiFi networks.</p>";
    }
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// Handle network scanning
void handleScan() {
    scannedNetworks = "";

    Serial.println("Starting WiFi scan...");
    int n = WiFi.scanNetworks();

    if (n > 0) {
        Serial.println("Networks found: " + String(n));
        for (int i = 0; i < n; i++) {
            scannedNetworks += "<li><a href='/connect?ssid=" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</a></li>";
        }
    } else if (n == 0) {
        scannedNetworks = "<li>No networks found</li>";
        Serial.println("No networks found.");
    } else {
        scannedNetworks = "<li>WiFi scan failed</li>";
        Serial.println("WiFi scan failed.");
    }

    handleRoot();
}

// Handle connection prompt
void handleConnectPrompt() {
    if (server.hasArg("ssid")) {
        String ssid = server.arg("ssid");
        String html = htmlHeader("Connect to WiFi");
        html += "<h1>Connect to " + ssid + "</h1>";
        html += "<form action='/connect' method='POST'>";
        html += "<input type='hidden' name='ssid' value='" + ssid + "'>";
        html += "<label for='password'>Password:</label><br>";
        html += "<input type='password' name='password'><br><br>";
        html += "<input type='submit' value='Connect'>";
        html += "</form>";
        html += "<p><a href='/'>Back</a></p>";
        html += htmlFooter();
        server.send(200, "text/html", html);
    }
}

// Handle WiFi connection
void handleConnect() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        WiFi.begin(ssid.c_str(), password.c_str());
        // Apply TX power control if enabled
        if (useTxPowerControl) {
            WiFi.setTxPower(WIFI_POWER_8_5dBm);
            Serial.println("TX power control applied: 8.5 dBm");
        } else {
            Serial.println("TX power control disabled.");
        }
        for (int attempts = 0; attempts < 5; attempts++) {
            if (WiFi.status() == WL_CONNECTED) {
                saveWiFiCredentials(ssid.c_str(), password.c_str());
                saveWiFiSetting(true);
                String html = htmlHeader("Connected");
                html += "<h1>Connected!</h1><p>Rebooting...</p>";
                html += htmlFooter();
                server.send(200, "text/html", html);
                displayMessage("Connected!", "Rebooting...", "");
                delay(2000);
                ESP.restart();
                return;
            }
            displayMessage("Connecting to", ssid, "Attempt: " + String(attempts + 1));
            delay(5000);
        }

        String html = htmlHeader("Connection Failed");
        html += "<h1>Connection Failed</h1><p>Check your credentials and try again.</p><p><a href='/'>Back</a></p>";
        html += htmlFooter();
        server.send(400, "text/html", html);
        displayMessage("Failed", "Check Credentials", "");
    }
}

// Upload configuration page
void handleConfigPage() {
    String html = htmlHeader("Upload Slider Config");
    html += "<h1>Slider Configuration Upload</h1>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='config'><br><br>";
    html += "<input type='submit' value='Upload'>";
    html += "</form>";
    html += "<p><a href='/'>Back</a></p>";
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// Handle file upload
void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    static File uploadFile;

    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Upload Start: %s\n", upload.filename.c_str());
        if (SPIFFS.exists("/sliders_config.json")) {
            SPIFFS.remove("/sliders_config.json");
        }
        uploadFile = SPIFFS.open("/sliders_config.json", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            Serial.printf("Upload End: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
        } else {
            Serial.println("Upload failed - could not open file");
        }
    }
}

// After file upload post
void handleFileUploadPost() {
    String message = htmlHeader("Configuration Uploaded");
    message += "<h1>Configuration Uploaded</h1><p>Device will reboot in a few seconds.</p>";
    message += htmlFooter();
    server.send(200, "text/html", message);

    delay(3000);
    ESP.restart();
}

// WiFi Settings page
void handleWiFiSettingsPage() {
    String html = htmlHeader("WiFi Settings");
    html += "<h1>WiFi Settings</h1>";
    html += "<p>Current state: " + String(useWifi ? "Enabled" : "Disabled") + "</p>";
    html += "<form method='POST' action='/enable_wifi'><input type='submit' value='Enable WiFi'></form><br>";
    html += "<form method='POST' action='/disable_wifi'><input type='submit' value='Disable WiFi'></form><br>";
    html += "<p><a href='/'>Back</a></p>";
    html += htmlFooter();
    server.send(200, "text/html", html);
}

void handleEnableWiFi() {
    if (useWifi) {
        String html = htmlHeader("WiFi Already Enabled");
        html += "<h1>WiFi is already enabled.</h1>";
        html += "<p><a href='/'>Back</a></p>";
        html += htmlFooter();
        server.send(200, "text/html", html);
    } else {
        saveWiFiSetting(true);
        String html = htmlHeader("Enabling WiFi");
        html += "<h1>Enabling WiFi...</h1><p>Rebooting in a moment.</p>";
        html += htmlFooter();
        server.send(200, "text/html", html);
        delay(2000);
        ESP.restart();
    }
}

void handleDisableWiFi() {
    if (!useWifi) {
        String html = htmlHeader("WiFi Already Disabled");
        html += "<h1>WiFi is already disabled.</h1>";
        html += "<p><a href='/'>Back</a></p>";
        html += htmlFooter();
        server.send(200, "text/html", html);
    } else {
        saveWiFiSetting(false);
        String html = htmlHeader("Disabling WiFi");
        html += "<h1>Disabling WiFi...</h1><p>Rebooting in a moment.</p>";
        html += htmlFooter();
        server.send(200, "text/html", html);
        delay(2000);
        ESP.restart();
    }
}

void initWiFiSetup() {
    String ssid, password;
    bool credsLoaded = loadWiFiCredentials(ssid, password);

    if (!useWifi) {
        Serial.println("WiFi usage disabled. Skipping WiFi setup.");
        wifiSetupDone = true;
        return;
    }

    if (credsLoaded && ssid.length() > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        // Apply TX power control if enabled
        if (useTxPowerControl) {
            WiFi.setTxPower(WIFI_POWER_8_5dBm);
            Serial.println("TX power control applied: 8.5 dBm");
        } else {
            Serial.println("TX power control disabled.");
        }
        int attemptCount = 0;
        while (attemptCount < 5) {
            if (WiFi.status() == WL_CONNECTED) {
                wifiSetupDone = true;
                displayMessage("Connected", WiFi.localIP().toString(), "");
                Serial.println("WiFi Connected Successfully.");
                startWebServer();
                delay(5000);
                return;
            }

            displayMessage("Connecting to", ssid, "Attempt: " + String(attemptCount + 1));
            delay(5000);
            attemptCount++;
        }

        Serial.println("WiFi connection failed after 5 attempts.");
        displayMessage("WiFi Failed", "Stopping WiFi...", "");
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        delay(2000);
    } else {
        startAccessPoint();
    }
}

void handleWiFiTasks() {
    dnsServer.processNextRequest();
    server.handleClient();
}
