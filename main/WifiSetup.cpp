#include "WiFiSetup.h"

const char* apSSID = "DEEJ";
DNSServer dnsServer;
WebServer server(80);
String scannedNetworks = "";
bool wifiSetupDone = false;

void handleRoot();
void handleScan();
void handleConnectPrompt();
void handleConnect();
void handleConfigPage();
void handleFileUploadPost();
void handleFileUpload();

void displayMessage(String line1, String line2, String line3) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, line1.c_str());
    u8g2.drawStr(0, 35, line2.c_str());
    u8g2.drawStr(0, 55, line3.c_str());
    u8g2.sendBuffer();
    Serial.println(line1 + " | " + line2 + " | " + line3);
}

// Save WiFi credentials to SPIFFS
void saveWiFiCredentials(const char* ssid, const char* password) {
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["ssid"] = ssid;
    jsonDoc["password"] = password;

    File file = SPIFFS.open("/wifi_config.json", "w");
    if (file) {
        serializeJson(jsonDoc, file);
        file.close();
        Serial.println("WiFi credentials saved.");
    } else {
        Serial.println("Failed to save WiFi credentials.");
    }
}

// Load WiFi credentials from SPIFFS
bool loadWiFiCredentials(String &ssid, String &password) {
    if (!SPIFFS.exists("/wifi_config.json")) return false;

    File file = SPIFFS.open("/wifi_config.json", "r");
    StaticJsonDocument<256> jsonDoc;
    if (deserializeJson(jsonDoc, file) == DeserializationError::Ok) {
        ssid = jsonDoc["ssid"].as<String>();
        password = jsonDoc["password"].as<String>();
        file.close();
        return true;
    }
    file.close();
    return false;
}

void startWebServer() {
    // Default pages
    server.on("/", HTTP_GET, handleRoot);
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/connect", HTTP_GET, handleConnectPrompt);
    server.on("/connect", HTTP_POST, handleConnect);

    // Configuration upload pages
    server.on("/config", HTTP_GET, handleConfigPage);
    server.on("/upload", HTTP_POST, handleFileUploadPost, handleFileUpload);

    server.begin();
    Serial.println("Web server started");
}

void startAccessPoint() {
    WiFi.softAP(apSSID);
    dnsServer.start(53, "", WiFi.softAPIP());

    startWebServer(); 
    Serial.println("AP Mode Started");
    delay(1000);
    displayMessage("To configure WiFi", "connect to DEEJ", "and visit 192.168.4.1");
}

// Handle the root page
void handleRoot() {
    String html = "<html><head><title>WiFi Setup</title></head><body>";
    html += "<h1>WiFi Setup</h1>";
    html += "<form action='/scan' method='GET'><button type='submit'>Scan Networks</button></form>";
    if (scannedNetworks != "") {
        html += "<h3>Available Networks</h3><ul>" + scannedNetworks + "</ul>";
    }
    html += "<p><a href='/config'>Upload Slider Config</a></p>";
    html += "</body></html>";
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
        String html = "<html><body><h1>Connect to " + ssid + "</h1>";
        html += "<form action='/connect' method='POST'>";
        html += "<input type='hidden' name='ssid' value='" + ssid + "'>";
        html += "Password: <input type='password' name='password'><br>";
        html += "<button type='submit'>Connect</button></form></body></html>";
        server.send(200, "text/html", html);
    }
}

// Handle WiFi connection
void handleConnect() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        WiFi.begin(ssid.c_str(), password.c_str());

        for (int attempts = 0; attempts < 5; attempts++) {
            if (WiFi.status() == WL_CONNECTED) {
                saveWiFiCredentials(ssid.c_str(), password.c_str());
                server.send(200, "text/html", "<h1>Connected!</h1><p>Rebooting...</p>");
                displayMessage("Connected!", "Rebooting...", "");
                delay(2000);
                ESP.restart();
                return;
            }
            displayMessage("Connecting to", ssid, "Attempt: " + String(attempts + 1));
            delay(5000);
        }

        server.send(400, "text/html", "<h1>Connection Failed</h1><a href='/'>Back</a>");
        displayMessage("Failed", "Check Credentials", "");
    }
}

// Upload configuration page
void handleConfigPage() {
    String html = "<html><head><title>Upload Slider Config</title></head><body>";
    html += "<h1>Slider Configuration Upload</h1>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='config'><br><br>";
    html += "<input type='submit' value='Upload'></form>";
    html += "</body></html>";
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
    // Send a response and then reboot after short delay
    String message = "<html><body><h1>Configuration Uploaded</h1><p>Device will reboot in a few seconds.</p></body></html>";
    server.send(200, "text/html", message);

    delay(3000);
    ESP.restart();
}

// Initialize WiFi setup
void initWiFiSetup() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        displayMessage("SPIFFS Error", "Restart Required", "");
        return;
    }

    String ssid, password;
    if (loadWiFiCredentials(ssid, password)) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        int attemptCount = 0;
        while (attemptCount < 5) {
            if (WiFi.status() == WL_CONNECTED) {
                wifiSetupDone = true;
                displayMessage("Connected", WiFi.localIP().toString(), "");
                Serial.println("WiFi Connected Successfully.");
                startWebServer(); // Start web server to access page from device IP as well
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
        delay(1000);
    } else {
        Serial.println("No saved credentials found.");
        displayMessage("No Network", "Starting AP Mode", "");
    }
    startAccessPoint();
}

void handleWiFiTasks() {
    dnsServer.processNextRequest();
    server.handleClient();
}
