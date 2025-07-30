#include "NetworkManager.h"
#include "esp_task_wdt.h"

NetworkManager::NetworkManager() : 
    ap_mode_active(false),
    ap_ssid("CYD Crypto Ticker Config"),
    web_server(nullptr),
    dns_server(nullptr),
    has_new_credentials(false),
    scan_in_progress(false),
    pending_scan_request(nullptr),
    has_scanned_networks(false),
    has_new_api_config(false) {
    ap_password = ""; // No password - open AP
}

NetworkManager::~NetworkManager() {
    if (web_server) {
        delete web_server;
    }
    if (dns_server) {
        delete dns_server;
    }
}

bool NetworkManager::connect(const char* ssid, const char* password, unsigned long timeout_ms) {
    if (!ssid || strlen(ssid) == 0) {
        Serial.println("No WiFi SSID provided");
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi");
    unsigned long start_time = millis();
    
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < timeout_ms) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected to WiFi! IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    
    Serial.println();
    Serial.println("WiFi connection timeout");
    return false;
}

bool NetworkManager::isConnected() const {
    return WiFi.isConnected();
}

String NetworkManager::getSignalStrength() const {
    if (!WiFi.isConnected()) {
        return "Disconnected";
    }
    return convertRSSIToText(WiFi.RSSI());
}

String NetworkManager::getLocalIP() const {
    if (!WiFi.isConnected()) {
        return "0.0.0.0";
    }
    return WiFi.localIP().toString();
}

String NetworkManager::getCurrentSSID() const {
    if (!WiFi.isConnected()) {
        return "Not connected";
    }
    return WiFi.SSID();
}

void NetworkManager::disconnect() {
    WiFi.disconnect();
}

bool NetworkManager::httpGet(const String& url, const String& api_key, String& response, int& httpCode) {
    HTTPClient http;
    http.begin(url);
    
    // Add headers
    http.addHeader("X-CMC_PRO_API_KEY", api_key);
    http.addHeader("Accept", "application/json");
    
    httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        http.end();
        return true;
    }
    
    http.end();
    return false;
}

String NetworkManager::convertRSSIToText(int rssi) const {
    if (rssi >= -50)
        return "Excellent";
    else if (rssi >= -60)
        return "Good";
    else if (rssi >= -70)
        return "Fair";
    else if (rssi >= -80)
        return "Weak";
    else
        return "Poor";
}

bool NetworkManager::startAPMode() {
    if (ap_mode_active) {
        return true;
    }
    
    WiFi.mode(WIFI_AP_STA); // AP+STA mode allows scanning while in AP mode
    bool success = WiFi.softAP(ap_ssid.c_str()); // Open AP - no password
    
    if (success) {
        ap_mode_active = true;
        setupWebServer();
        
        Serial.println("AP Mode started");
        Serial.print("SSID: ");
        Serial.println(ap_ssid);
        Serial.println("Password: (open - no password)");
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP());
    }
    
    return success;
}

void NetworkManager::stopAPMode() {
    if (!ap_mode_active) {
        return;
    }
    
    if (web_server) {
        web_server->end();
        delete web_server;
        web_server = nullptr;
    }
    
    if (dns_server) {
        dns_server->stop();
        delete dns_server;
        dns_server = nullptr;
    }
    
    WiFi.softAPdisconnect(true);
    ap_mode_active = false;
    
    Serial.println("AP Mode stopped");
}

bool NetworkManager::isAPMode() const {
    return ap_mode_active;
}

String NetworkManager::getAPSSID() const {
    return ap_ssid;
}

String NetworkManager::getAPPassword() const {
    return ap_password;
}

void NetworkManager::handleAPMode() {
    if (ap_mode_active && dns_server) {
        dns_server->processNextRequest();
    }
}

bool NetworkManager::hasNewCredentials() const {
    return has_new_credentials;
}

String NetworkManager::getNewSSID() const {
    return new_ssid;
}

String NetworkManager::getNewPassword() const {
    return new_password;
}

void NetworkManager::clearNewCredentials() {
    has_new_credentials = false;
    new_ssid = "";
    new_password = "";
}

String NetworkManager::generateRandomPassword(int length) {
    String password = "";
    const char charset[] = "abcdefghijklmnopqrstuvwxyz";
    
    randomSeed(esp_random());
    
    for (int i = 0; i < length; i++) {
        password += charset[random(0, sizeof(charset) - 1)];
    }
    
    return password;
}

void NetworkManager::setupWebServer() {
    if (web_server) {
        delete web_server;
    }
    if (dns_server) {
        delete dns_server;
    }
    
    web_server = new AsyncWebServer(80);
    dns_server = new DNSServer();
    
    // Setup captive portal DNS
    dns_server->start(53, "*", WiFi.softAPIP());
    
    // Main configuration page
    web_server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = R"HTML(<!DOCTYPE html>
<html>
<head>
    <title>CYD Crypto Ticker Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background: #222;
            color: #fff;
            margin: 0;
            padding: 0;
        }
        
        .container {
            max-width: 500px;
            margin: 20px auto;
            padding: 20px;
        }
        
        h1 {
            color: #4CAF50;
            margin-bottom: 30px;
        }
        
        h3 {
            color: #fff;
            margin-top: 30px;
            margin-bottom: 15px;
            border-bottom: 1px solid #444;
            padding-bottom: 10px;
        }
        
        input {
            width: 100%;
            padding: 12px;
            margin: 8px 0;
            box-sizing: border-box;
            border: 1px solid #555;
            background: #333;
            color: #fff;
            border-radius: 4px;
        }
        
        input:focus {
            border-color: #4CAF50;
            outline: none;
        }
        
        button {
            background: #4CAF50;
            color: white;
            padding: 14px 20px;
            margin: 8px 0;
            border: none;
            cursor: pointer;
            width: 100%;
            border-radius: 4px;
            font-size: 16px;
        }
        
        button:hover {
            background: #45a049;
        }
        
        .scan-btn {
            background: #2196F3;
        }
        
        .scan-btn:hover {
            background: #1976D2;
        }
        
        #networks {
            text-align: left;
            margin: 10px 0;
        }
        
        .network {
            padding: 10px;
            border: 1px solid #555;
            margin: 5px 0;
            cursor: pointer;
            border-radius: 4px;
            background: #333;
        }
        
        .network:hover {
            background: #444;
        }
        
        .help-text {
            font-size: 14px;
            color: #aaa;
            margin-top: 5px;
            text-align: left;
            line-height: 1.4;
        }
        
        .help-text a {
            color: #4CAF50;
            text-decoration: none;
        }
        
        .help-text a:hover {
            text-decoration: underline;
        }
        
        #validation-errors {
            color: #ee5a52;
            margin: 10px 0;
            text-align: left;
            background: #441;
            padding: 10px;
            border-radius: 4px;
            border-left: 4px solid #ee5a52;
            display: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>CYD Crypto Ticker Configuration</h1>
        
        <div style="margin: 10px 0;">
            <button class="scan-btn" onclick="refreshNetworks()">Refresh Networks (Reboot)</button>
        </div>
        
        <div id="networks">)HTML";
        
        // Embed the network list directly in HTML
        if (has_scanned_networks) {
            String networks_json = getScannedNetworksJSON();
            html += "<script>var networksData = " + networks_json + ";</script>";
            
            // Parse and display networks immediately
            html += R"HTML(<script>
                if (networksData.length === 0) {
                    document.write('<h3>No networks found</h3><p><small>Try using "Refresh (Reboot)" to rescan</small></p>');
                } else {
                    document.write('<h3>Available Networks (' + networksData.length + ' found):</h3>');
                    networksData.forEach(function(net) {
                        var securityIcon = net.secure ? '&#128274;' : '&#128246;'; // Lock and signal icons as HTML entities
                        var signalText = '';
                        if (net.rssi >= -50) signalText = 'Excellent';
                        else if (net.rssi >= -60) signalText = 'Good';
                        else if (net.rssi >= -70) signalText = 'Fair';
                        else if (net.rssi >= -80) signalText = 'Weak';
                        else signalText = 'Poor';
                        
                        document.write('<div class="network" onclick="selectNetwork(\'' + net.ssid + '\')">' + securityIcon + ' ' + net.ssid + ' (' + signalText + ')</div>');
                    });
                }
            </script>)HTML";
        } else {
            html += "<h3>No networks scanned</h3><p><small>Try using \"Refresh Networks (Reboot)\" to scan</small></p>";
        }
        
        html += R"HTML(</div>
        <form action="/connect" method="POST" onsubmit="return validateForm()">)HTML";
        
        // Load stored WiFi config to populate default values
        String stored_ssid, stored_password;
        Preferences temp_prefs;
        temp_prefs.begin("wifi", true); // Read-only
        stored_ssid = temp_prefs.getString("ssid", "");
        stored_password = temp_prefs.getString("password", "");
        temp_prefs.end();
        bool has_stored_config = (stored_ssid.length() > 0);
        
        // Load stored API config
        String stored_api_key, stored_coin_ids;
        temp_prefs.begin("api", true); // Read-only
        stored_api_key = temp_prefs.getString("api_key", "");
        stored_coin_ids = temp_prefs.getString("coin_ids", "1,1027,1839,52,20947,74");
        temp_prefs.end();
        
        html += R"HTML(            <h3>WiFi Configuration</h3>
            <input type="text" name="ssid" placeholder="WiFi Network Name (SSID)" )HTML";
        if (has_stored_config) {
            html += "value=\"" + escapeJsonString(stored_ssid) + "\" ";
        }
        html += R"HTML(required>
            <input type="password" name="password" placeholder="WiFi Password" )HTML";
        if (has_stored_config) {
            html += "value=\"" + escapeJsonString(stored_password) + "\" ";
        }
        html += R"HTML(>
            
            <h3>CoinMarketCap API Configuration</h3>
            <input type="text" name="api_key" placeholder="API Key (UUID format)" )HTML";
        if (stored_api_key.length() > 0) {
            html += "value=\"" + escapeJsonString(stored_api_key) + "\" ";
        }
        html += R"HTML(required>
            <div class="help-text">
                Get your free API key from <a href="https://coinmarketcap.com/api/" target="_blank">CoinMarketCap API</a>
            </div>
            
            <input type="text" name="coin_ids" placeholder="Coin IDs (comma-separated numbers)" )HTML";
        html += "value=\"" + escapeJsonString(stored_coin_ids) + "\" ";
        html += R"HTML(required>
            <div class="help-text">
                Find coin IDs on CoinMarketCap coin pages (e.g., <a href="https://coinmarketcap.com/currencies/dogecoin/" target="_blank">Dogecoin</a>) 
                in the URL or as the UCID property. Examples: Bitcoin=1, Ethereum=1027, Dogecoin=74
            </div>
            
            <div id="validation-errors"></div>
            <button type="submit">Save Configuration</button>
        </form>
    </div>
    <script>
        function validateForm() {
            var errors = [];
            var apiKey = document.querySelector('input[name="api_key"]').value;
            var coinIds = document.querySelector('input[name="coin_ids"]').value;
            var errorDiv = document.getElementById('validation-errors');
            
            // Clear previous errors
            errorDiv.style.display = 'none';
            errorDiv.innerHTML = '';
            
            // Validate UUID format for API key
            var uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
            if (!uuidRegex.test(apiKey)) {
                errors.push('API Key must be in UUID format (e.g., 12345678-1234-1234-1234-123456789abc)');
            }
            
            // Validate coin IDs format
            if (coinIds.indexOf(' ') !== -1 || coinIds.indexOf('\t') !== -1) {
                errors.push('Coin IDs must not contain whitespace');
            }
            
            var coinIdRegex = /^[0-9]+(,[0-9]+)*$/;
            if (!coinIdRegex.test(coinIds)) {
                errors.push('Coin IDs must be comma-separated numbers (e.g., 1,1027,1839)');
            }
            
            // Show errors if any
            if (errors.length > 0) {
                errorDiv.innerHTML = errors.join('<br>');
                errorDiv.style.display = 'block';
                return false;
            }
            
            return true;
        }
        
        function refreshNetworks() {
            if (confirm('This will reboot the device to rescan WiFi networks. Continue?')) {
                document.getElementById('networks').innerHTML = 
                    '<h3>Rebooting device...</h3>' +
                    '<p><small>Please reconnect to this access point in a few seconds</small></p>';
                
                // Ignore connection errors during reboot
                fetch('/refresh').catch(function() {
                    // Expected to fail during reboot
                });
            }
        }
        
        function selectNetwork(ssid) {
            document.querySelector('input[name="ssid"]').value = ssid;
        }
    </script>
</body>
</html>)HTML";
        request->send(200, "text/html", html);
    });
    
    // Refresh endpoint - reboot device to rescan networks
    web_server->on("/refresh", HTTP_GET, [this](AsyncWebServerRequest *request){
        Serial.println("Refresh requested - rebooting device");
        request->send(200, "text/html", 
            "<html><body style='font-family:Arial; text-align:center; background:#222; color:#fff;'>"
            "<h1>Refreshing...</h1><p>Device is rebooting to rescan networks.</p>"
            "<p>Please reconnect in a few seconds.</p></body></html>");
        delay(1000);
        ESP.restart();
    });
    
    // Handle WiFi and API configuration
    web_server->on("/connect", HTTP_POST, [this](AsyncWebServerRequest *request){
        String ssid = "";
        String password = "";
        String api_key = "";
        String coin_ids = "";
        
        if (request->hasParam("ssid", true)) {
            ssid = request->getParam("ssid", true)->value();
        }
        if (request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        }
        if (request->hasParam("api_key", true)) {
            api_key = request->getParam("api_key", true)->value();
        }
        if (request->hasParam("coin_ids", true)) {
            coin_ids = request->getParam("coin_ids", true)->value();
        }
        
        // Server-side validation
        String errors = "";
        if (ssid.length() == 0) {
            errors += "SSID is required.<br>";
        }
        if (api_key.length() == 0) {
            errors += "API Key is required.<br>";
        } else if (!validateUUID(api_key)) {
            errors += "API Key must be in UUID format.<br>";
        }
        if (coin_ids.length() == 0) {
            errors += "Coin IDs are required.<br>";
        } else if (!validateCoinIds(coin_ids)) {
            errors += "Coin IDs must be comma-separated numbers with no whitespace.<br>";
        }
        
        if (errors.length() > 0) {
            request->send(400, "text/html", 
                "<html><body style='font-family:Arial; text-align:center; background:#222; color:#fff;'>"
                "<h1>Validation Error</h1><p>" + errors + "</p><a href='/'>Go Back</a></body></html>");
            return;
        }
        
        // Store new configurations
        new_ssid = ssid;
        new_password = password;
        has_new_credentials = true;
        
        new_api_key = api_key;
        new_coin_ids = coin_ids;
        has_new_api_config = true;
        
        request->send(200, "text/html", 
            "<html><body style='font-family:Arial; text-align:center; background:#222; color:#fff;'>"
            "<h1>Configuration Saved</h1><p>The device will now try to connect to: " + ssid + "</p>"
            "<p>API configuration has been saved.</p>"
            "<p>If WiFi connection is successful, this access point will be closed.</p></body></html>");
    });
    
    // Catch all for captive portal
    web_server->onNotFound([this](AsyncWebServerRequest *request){
        request->redirect("/");
    });
    
    web_server->begin();
    Serial.println("Web server started");
}

void NetworkManager::clearStoredWiFiConfig() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    Serial.println("WiFi configuration cleared from storage");
}

void NetworkManager::saveWiFiConfig(const String& ssid, const String& password) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    Serial.println("WiFi configuration saved to storage");
}

bool NetworkManager::loadStoredWiFiConfig(String& ssid, String& password) {
    preferences.begin("wifi", true); // Read-only
    
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    
    preferences.end();
    
    if (ssid.length() > 0) {
        Serial.println("WiFi configuration loaded from storage");
        return true;
    }
    
    Serial.println("No WiFi configuration found in storage");
    return false;
}

void NetworkManager::setReconfigurationRequested(bool requested) {
    preferences.begin("system", false);
    preferences.putBool("reconfig_req", requested);
    preferences.end();
    
    if (requested) {
        Serial.println("Reconfiguration flag set in persistent storage");
    } else {
        Serial.println("Reconfiguration flag cleared from persistent storage");
    }
}

bool NetworkManager::isReconfigurationRequested() const {
    Preferences temp_prefs;
    temp_prefs.begin("system", true); // Read-only
    bool requested = temp_prefs.getBool("reconfig_req", false);
    temp_prefs.end();
    
    return requested;
}

void NetworkManager::clearReconfigurationFlag() {
    setReconfigurationRequested(false);
}

void NetworkManager::factoryReset() {
    Serial.println("Performing factory reset - clearing all stored data...");
    
    // Clear WiFi configuration
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    
    // Clear system configuration (including reconfiguration flag)
    preferences.begin("system", false);
    preferences.clear();
    preferences.end();
    
    Serial.println("Factory reset complete - all stored data cleared");
}

String NetworkManager::escapeJsonString(const String& str) {
    String escaped = "";
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        switch (c) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (c < 32) {
                    // Escape other control characters
                    escaped += "\\u" + String(c, HEX);
                    while (escaped.length() % 6 != 2) escaped = escaped.substring(0, escaped.length()-1) + "0" + escaped.substring(escaped.length()-1);
                } else {
                    escaped += c;
                }
                break;
        }
    }
    return escaped;
}

bool NetworkManager::scanWiFiNetworks() {
    Serial.println("Scanning for WiFi networks...");
    
    // Set to STA mode for scanning
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Clear any previous scan results
    WiFi.scanDelete();
    
    // Perform synchronous scan
    int n = WiFi.scanNetworks();
    
    Serial.print("WiFi scan completed. Found ");
    Serial.print(n);
    Serial.println(" networks");
    
    if (n > 0) {
        // Build JSON string with scan results
        scanned_networks_json = "[";
        for (int i = 0; i < n; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() > 0) { // Only include networks with valid SSID
                if (scanned_networks_json.length() > 1) scanned_networks_json += ",";
                scanned_networks_json += "{";
                scanned_networks_json += "\"ssid\":\"" + escapeJsonString(ssid) + "\",";
                scanned_networks_json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
                scanned_networks_json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                scanned_networks_json += "}";
            }
        }
        scanned_networks_json += "]";
        
        has_scanned_networks = true;
        WiFi.scanDelete(); // Clean up
        
        Serial.println("WiFi networks cached for AP mode");
        return true;
    } else if (n == 0) {
        Serial.println("No WiFi networks found");
        scanned_networks_json = "[]";
        has_scanned_networks = true;
        return true;
    } else {
        Serial.print("WiFi scan failed with error: ");
        Serial.println(n);
        scanned_networks_json = "[]";
        has_scanned_networks = false;
        return false;
    }
}

String NetworkManager::getScannedNetworksJSON() const {
    if (has_scanned_networks) {
        return scanned_networks_json;
    }
    return "[]";
}

bool NetworkManager::validateUUID(const String& uuid) const {
    if (uuid.length() != 36) return false;
    
    for (int i = 0; i < 36; i++) {
        char c = uuid.charAt(i);
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') return false;
        } else {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                return false;
            }
        }
    }
    return true;
}

bool NetworkManager::validateCoinIds(const String& coin_ids) const {
    if (coin_ids.length() == 0) return false;
    
    // Check for whitespace
    if (coin_ids.indexOf(' ') != -1 || coin_ids.indexOf('\t') != -1 || 
        coin_ids.indexOf('\n') != -1 || coin_ids.indexOf('\r') != -1) {
        return false;
    }
    
    // Split by commas and validate each part
    String temp = coin_ids;
    while (temp.length() > 0) {
        int comma_pos = temp.indexOf(',');
        String part = (comma_pos == -1) ? temp : temp.substring(0, comma_pos);
        
        if (part.length() == 0) return false; // Empty part
        
        // Check if part is a number
        for (int i = 0; i < part.length(); i++) {
            if (!isDigit(part.charAt(i))) return false;
        }
        
        if (comma_pos == -1) break;
        temp = temp.substring(comma_pos + 1);
    }
    
    return true;
}

bool NetworkManager::hasNewAPIConfig() const {
    return has_new_api_config;
}

String NetworkManager::getNewAPIKey() const {
    return new_api_key;
}

String NetworkManager::getNewCoinIds() const {
    return new_coin_ids;
}

void NetworkManager::clearNewAPIConfig() {
    has_new_api_config = false;
    new_api_key = "";
    new_coin_ids = "";
}

void NetworkManager::saveAPIConfig(const String& api_key, const String& coin_ids) {
    preferences.begin("api", false);
    preferences.putString("api_key", api_key);
    preferences.putString("coin_ids", coin_ids);
    preferences.end();
    Serial.println("API configuration saved to storage");
}

bool NetworkManager::loadStoredAPIConfig(String& api_key, String& coin_ids) {
    preferences.begin("api", true); // Read-only
    
    api_key = preferences.getString("api_key", "");
    coin_ids = preferences.getString("coin_ids", "1,1027,1839,52,20947,74");
    
    preferences.end();
    
    if (api_key.length() > 0) {
        Serial.println("API configuration loaded from storage");
        return true;
    }
    
    Serial.println("No API configuration found in storage");
    return false;
}