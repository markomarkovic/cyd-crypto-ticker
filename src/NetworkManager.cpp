#include "NetworkManager.h"
#include "constants.h"
#include "esp_task_wdt.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

NetworkManager::NetworkManager() : 
    ap_mode_active(false),
    ap_ssid("CYD Crypto Ticker Config"),
    web_server(nullptr),
    dns_server(nullptr),
    has_new_credentials(false),
    scan_in_progress(false),
    pending_scan_request(nullptr),
    has_scanned_networks(false),
    has_new_symbols_config(false) {
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
        LOG_DEBUG("No WiFi SSID provided");
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    LOG_DEBUG("Connecting to WiFi");
    unsigned long start_time = millis();
    
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < timeout_ms) {
        delay(500);
        LOG_DEBUG(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        LOG_DEBUG("");
        LOG_DEBUG("Connected to WiFi! IP address: ");
        LOG_DEBUG(WiFi.localIP());
        return true;
    }
    
    LOG_DEBUG("");
    LOG_DEBUG("WiFi connection timeout");
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
        
        LOG_DEBUG("AP Mode started");
        LOG_DEBUG("SSID: ");
        LOG_DEBUG(ap_ssid);
        LOG_DEBUG("Password: (open - no password)");
        LOG_DEBUG("IP address: ");
        LOG_DEBUG(WiFi.softAPIP());
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
    
    LOG_DEBUG("AP Mode stopped");
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
    <style>:root{--pico-font-family:system-ui,"Segoe UI","Roboto","Helvetica Neue","Noto Sans","Liberation Sans","Arial",sans-serif;--pico-line-height:1.5;--pico-font-weight:400;--pico-font-size:16px;--pico-border-radius:0.25rem;--pico-border-width:1px;--pico-outline-width:3px;--pico-spacing:1rem;--pico-form-element-spacing-vertical:0.75rem;--pico-form-element-spacing-horizontal:1rem;--pico-transition:0.2s ease-in-out}*{box-sizing:border-box}body,html{overflow-x:hidden;width:100%;margin:0;padding:0}:root:not([data-theme=light]){color-scheme:dark;--pico-primary:#3b82f6;--pico-primary-background:#3b82f6;--pico-primary-hover:#2563eb;--pico-primary-hover-background:#2563eb;--pico-primary-focus:rgba(59,130,246,.25);--pico-primary-inverse:#fff;--pico-secondary:#64748b;--pico-secondary-background:#64748b;--pico-secondary-hover:#475569;--pico-secondary-hover-background:#475569;--pico-secondary-focus:rgba(100,116,139,.25);--pico-secondary-inverse:#fff;--pico-background-color:#0f172a;--pico-color:#cbd5e1;--pico-h1-color:#f1f5f9;--pico-h2-color:#e2e8f0;--pico-h3-color:#cbd5e1;--pico-muted-color:#64748b;--pico-border-color:#334155;--pico-form-element-background-color:#1e293b;--pico-form-element-border-color:#334155;--pico-form-element-color:#cbd5e1;--pico-form-element-placeholder-color:#64748b;--pico-form-element-active-background-color:#1e293b;--pico-form-element-active-border-color:var(--pico-primary);--pico-form-element-focus-color:var(--pico-primary-focus)}:root[data-theme=light]{color-scheme:light;--pico-primary:#3b82f6;--pico-primary-background:#3b82f6;--pico-primary-hover:#2563eb;--pico-primary-hover-background:#2563eb;--pico-primary-focus:rgba(59,130,246,.25);--pico-primary-inverse:#fff;--pico-secondary:#64748b;--pico-secondary-background:#64748b;--pico-secondary-hover:#475569;--pico-secondary-hover-background:#475569;--pico-secondary-focus:rgba(100,116,139,.25);--pico-secondary-inverse:#fff;--pico-background-color:#fff;--pico-color:#1e293b;--pico-h1-color:#0f172a;--pico-h2-color:#1e293b;--pico-h3-color:#334155;--pico-muted-color:#64748b;--pico-border-color:#e2e8f0;--pico-form-element-background-color:#fff;--pico-form-element-border-color:#d1d5db;--pico-form-element-color:#1e293b;--pico-form-element-placeholder-color:#9ca3af;--pico-form-element-active-background-color:#fff;--pico-form-element-active-border-color:var(--pico-primary);--pico-form-element-focus-color:var(--pico-primary-focus)}[type=button],[type=reset],[type=submit],button{--pico-background-color:var(--pico-primary-background);--pico-border-color:var(--pico-primary-background);--pico-color:var(--pico-primary-inverse);padding:var(--pico-form-element-spacing-vertical) var(--pico-form-element-spacing-horizontal);border:var(--pico-border-width) solid var(--pico-border-color);border-radius:var(--pico-border-radius);outline:none;background-color:var(--pico-background-color);color:var(--pico-color);font-weight:var(--pico-font-weight);font-size:1rem;line-height:var(--pico-line-height);text-align:center;cursor:pointer;transition:background-color var(--pico-transition),border-color var(--pico-transition),color var(--pico-transition);text-decoration:none;display:block;width:100%;box-sizing:border-box}[type=button]:is([aria-current],:hover,:active,:focus),[type=reset]:is([aria-current],:hover,:active,:focus),[type=submit]:is([aria-current],:hover,:active,:focus),button:is([aria-current],:hover,:active,:focus){--pico-background-color:var(--pico-primary-hover-background);--pico-border-color:var(--pico-primary-hover-background)}[type=button]:focus,[type=reset]:focus,[type=submit]:focus,button:focus{box-shadow:0 0 0 var(--pico-outline-width) var(--pico-primary-focus)}input:not([type=checkbox],[type=radio],[type=range],[type=file]),select,textarea{--pico-background-color:var(--pico-form-element-background-color);--pico-border-color:var(--pico-form-element-border-color);--pico-color:var(--pico-form-element-color);border:var(--pico-border-width) solid var(--pico-border-color);border-radius:var(--pico-border-radius);outline:none;background-color:var(--pico-background-color);color:var(--pico-color);font-weight:var(--pico-font-weight);font-size:1rem;line-height:var(--pico-line-height);transition:background-color var(--pico-transition),border-color var(--pico-transition),color var(--pico-transition);margin-bottom:var(--pico-spacing);padding:var(--pico-form-element-spacing-vertical) var(--pico-form-element-spacing-horizontal);width:100%;box-sizing:border-box}input:not([type=checkbox],[type=radio],[type=range],[type=file])::placeholder,select::placeholder,textarea::placeholder{color:var(--pico-form-element-placeholder-color);opacity:1}input:not([type=checkbox],[type=radio],[type=range],[type=file]):is(:active,:focus),select:is(:active,:focus),textarea:is(:active,:focus){--pico-background-color:var(--pico-form-element-active-background-color);--pico-border-color:var(--pico-form-element-active-border-color);box-shadow:0 0 0 var(--pico-outline-width) var(--pico-form-element-focus-color)}h1,h2,h3{margin-top:calc(var(--pico-spacing) * 1.5);margin-bottom:calc(var(--pico-spacing) * .5);font-weight:700;line-height:1.125}h1{font-size:2rem;color:var(--pico-h1-color)}h1:first-child{margin-top:0}h2{font-size:1.75rem;color:var(--pico-h2-color)}h3{font-size:1.5rem;color:var(--pico-h3-color)}.container{width:100%;margin-right:auto;margin-left:auto;padding:var(--pico-spacing);max-width:500px}.scan-btn{--pico-background-color:var(--pico-secondary-background);--pico-border-color:var(--pico-secondary-background)}.scan-btn:is([aria-current],:hover,:active,:focus){--pico-background-color:var(--pico-secondary-hover-background);--pico-border-color:var(--pico-secondary-hover-background)}.network{padding:var(--pico-spacing);border:var(--pico-border-width) solid var(--pico-border-color);margin:calc(var(--pico-spacing) / 2) 0;cursor:pointer;border-radius:var(--pico-border-radius);background:var(--pico-form-element-background-color);transition:background-color var(--pico-transition)}.network:hover{background:var(--pico-form-element-active-background-color)}.help-text{font-size:.875rem;color:var(--pico-muted-color);margin-top:calc(var(--pico-spacing) / 4);margin-bottom:calc(var(--pico-spacing) * 1.5);text-align:left;line-height:1.4}.help-text a{color:var(--pico-primary);text-decoration:none}.help-text a:hover{text-decoration:underline}#validation-errors{color:#dc2626;margin:var(--pico-spacing) 0;text-align:left;background:rgba(220,38,38,.1);padding:var(--pico-spacing);border-radius:var(--pico-border-radius);border-left:4px solid #dc2626;display:none}.password-container{position:relative;display:block;width:100%}.password-toggle{position:absolute;right:calc(var(--pico-form-element-spacing-horizontal) / 2);top:50%;transform:translateY(-50%);background:none;border:none;color:var(--pico-muted-color);cursor:pointer;padding:2px 6px;height:auto;min-width:32px;width:auto;display:inline-flex;align-items:center;justify-content:center;font-size:12px;font-weight:500;user-select:none;transition:color var(--pico-transition);text-transform:lowercase}.password-toggle:hover{color:var(--pico-color)}.password-toggle:focus{outline:none!important;box-shadow:none!important}.password-container input[type=password],.password-container input[type=text]{padding-right:calc(var(--pico-form-element-spacing-horizontal) + 24px + var(--pico-form-element-spacing-horizontal) / 2)}</style>
</head>
<body>
    <main class="container">
        <h1>CYD Crypto Ticker Configuration</h1>
        
        <div>
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
        
        // Load stored symbols config
        String stored_symbols;
        temp_prefs.begin("symbols", true); // Read-only
        stored_symbols = temp_prefs.getString("symbols", "BTCUSDT,ETHUSDT,BNBUSDT,XRPUSDT,SOLUSDT,DOGEUSDT");
        temp_prefs.end();
        
        // Parse stored symbols into array for individual inputs
        String symbol_array[6] = {"", "", "", "", "", ""};
        if (stored_symbols.length() > 0) {
            int start = 0;
            int index = 0;
            for (int i = 0; i <= stored_symbols.length() && index < 6; i++) {
                if (i == stored_symbols.length() || stored_symbols.charAt(i) == ',') {
                    if (i > start) {
                        symbol_array[index] = stored_symbols.substring(start, i);
                        symbol_array[index].trim();
                        index++;
                    }
                    start = i + 1;
                }
            }
        }
        
        html += R"HTML(            <h3>WiFi Configuration</h3>
            <input type="text" name="ssid" placeholder="WiFi Network Name (SSID)" )HTML";
        if (has_stored_config) {
            html += "value=\"" + escapeJsonString(stored_ssid) + "\" ";
        }
        html += R"HTML(required>
            <div class="password-container">
                <input type="password" name="password" placeholder="WiFi Password" )HTML";
        if (has_stored_config) {
            html += "value=\"" + escapeJsonString(stored_password) + "\" ";
        }
        html += R"HTML(>
                <button type="button" class="password-toggle" onclick="togglePassword()" aria-label="Toggle password visibility">show</button>
            </div>
            
            <h3>Cryptocurrency Configuration</h3>
            <input type="text" name="coin1" placeholder="Coin 1 (e.g., BTCUSDT)" )HTML";
        if (symbol_array[0].length() > 0) {
            html += "value=\"" + escapeJsonString(symbol_array[0]) + "\" ";
        }
        html += R"HTML(required>
            <input type="text" name="coin2" placeholder="Coin 2 (e.g., ETHUSDT)" )HTML";
        if (symbol_array[1].length() > 0) {
            html += "value=\"" + escapeJsonString(symbol_array[1]) + "\" ";
        }
        html += R"HTML(required>
            <input type="text" name="coin3" placeholder="Coin 3 (e.g., BNBUSDT)" )HTML";
        if (symbol_array[2].length() > 0) {
            html += "value=\"" + escapeJsonString(symbol_array[2]) + "\" ";
        }
        html += R"HTML(required>
            <input type="text" name="coin4" placeholder="Coin 4 (e.g., XRPUSDT)" )HTML";
        if (symbol_array[3].length() > 0) {
            html += "value=\"" + escapeJsonString(symbol_array[3]) + "\" ";
        }
        html += R"HTML(required>
            <input type="text" name="coin5" placeholder="Coin 5 (e.g., SOLUSDT)" )HTML";
        if (symbol_array[4].length() > 0) {
            html += "value=\"" + escapeJsonString(symbol_array[4]) + "\" ";
        }
        html += R"HTML(required>
            <input type="text" name="coin6" placeholder="Coin 6 (e.g., DOGEUSDT)" )HTML";
        if (symbol_array[5].length() > 0) {
            html += "value=\"" + escapeJsonString(symbol_array[5]) + "\" ";
        }
        html += R"HTML(required>
            <div class="help-text">
                Enter 6 Binance USDT trading pairs. All fields are required.
            </div>
            
            <div id="validation-errors"></div>
            <button type="submit">Save Configuration</button>
        </form>
    </main>
    <script>
        function validateForm() {
            var errors = [];
            var errorDiv = document.getElementById('validation-errors');
            
            // Clear previous errors
            errorDiv.style.display = 'none';
            errorDiv.innerHTML = '';
            
            // Validate all 6 coin inputs
            var coinInputs = ['coin1', 'coin2', 'coin3', 'coin4', 'coin5', 'coin6'];
            var validSymbols = [];
            
            for (var i = 0; i < coinInputs.length; i++) {
                var coinInput = document.querySelector('input[name="' + coinInputs[i] + '"]');
                var symbol = coinInput.value.trim().toUpperCase();
                
                if (symbol.length === 0) {
                    errors.push('Coin ' + (i + 1) + ' is required');
                } else {
                    // Check for valid Binance symbol format
                    var symbolRegex = /^[A-Z0-9]+$/;
                    if (!symbolRegex.test(symbol)) {
                        errors.push('Coin ' + (i + 1) + ': Invalid symbol format. Use only letters and numbers (e.g., BTCUSDT)');
                    } else if (symbol.length < 6 || !symbol.endsWith('USDT')) {
                        errors.push('Coin ' + (i + 1) + ': Invalid symbol "' + symbol + '". Use Binance USDT pairs (e.g., BTCUSDT)');
                    } else {
                        // Check for duplicates
                        if (validSymbols.indexOf(symbol) !== -1) {
                            errors.push('Coin ' + (i + 1) + ': Duplicate symbol "' + symbol + '". Each coin must be unique');
                        } else {
                            validSymbols.push(symbol);
                        }
                    }
                }
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
        
        function togglePassword() {
            var passwordInput = document.querySelector('input[name="password"]');
            var toggleButton = document.querySelector('.password-toggle');
            
            if (passwordInput.type === 'password') {
                passwordInput.type = 'text';
                toggleButton.textContent = 'hide';
                toggleButton.setAttribute('aria-label', 'Hide password');
            } else {
                passwordInput.type = 'password';
                toggleButton.textContent = 'show';
                toggleButton.setAttribute('aria-label', 'Show password');
            }
            
            // Focus back to the password input
            passwordInput.focus();
        }
        
        // Auto-convert all coin inputs to uppercase
        document.addEventListener('DOMContentLoaded', function() {
            var coinInputs = ['coin1', 'coin2', 'coin3', 'coin4', 'coin5', 'coin6'];
            
            for (var i = 0; i < coinInputs.length; i++) {
                var coinInput = document.querySelector('input[name="' + coinInputs[i] + '"]');
                coinInput.addEventListener('input', function(e) {
                    var start = e.target.selectionStart;
                    var end = e.target.selectionEnd;
                    e.target.value = e.target.value.toUpperCase();
                    e.target.setSelectionRange(start, end);
                });
            }
        });
    </script>
</body>
</html>)HTML";
        request->send(200, "text/html", html);
    });
    
    // Refresh endpoint - reboot device to rescan networks
    web_server->on("/refresh", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_DEBUG("Refresh requested - rebooting device");
        request->send(200, "text/html", 
            "<!DOCTYPE html><html><head><title>Refreshing Networks</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>:root{--pico-background-color:#0f172a;--pico-color:#cbd5e1;--pico-h1-color:#f1f5f9;--pico-font-family:system-ui,sans-serif;--pico-spacing:1rem}body{font-family:var(--pico-font-family);background-color:var(--pico-background-color);color:var(--pico-color);text-align:center;margin:0;padding:calc(var(--pico-spacing)*2)}h1{color:var(--pico-h1-color)}</style></head>"
            "<body><h1>Refreshing...</h1><p>Device is rebooting to rescan networks.</p>"
            "<p>Please reconnect in a few seconds.</p></body></html>");
        delay(2000);
        ESP.restart();
    });
    
    // Handle WiFi and API configuration
    web_server->on("/connect", HTTP_POST, [this](AsyncWebServerRequest *request){
        String ssid = "";
        String password = "";
        String symbols = "";
        
        if (request->hasParam("ssid", true)) {
            ssid = request->getParam("ssid", true)->value();
        }
        if (request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        }
        
        // Collect 6 individual coin parameters and combine them
        String coinInputs[6] = {"coin1", "coin2", "coin3", "coin4", "coin5", "coin6"};
        String coinValues[6];
        
        for (int i = 0; i < 6; i++) {
            if (request->hasParam(coinInputs[i], true)) {
                coinValues[i] = request->getParam(coinInputs[i], true)->value();
                coinValues[i].trim();
                coinValues[i].toUpperCase();
            }
        }
        
        // Combine coins into comma-separated string
        for (int i = 0; i < 6; i++) {
            if (coinValues[i].length() > 0) {
                if (symbols.length() > 0) {
                    symbols += ",";
                }
                symbols += coinValues[i];
            }
        }
        
        // Server-side validation
        String errors = "";
        if (ssid.length() == 0) {
            errors += "SSID is required.<br>";
        }
        
        // Validate all 6 coins are provided
        int validCoins = 0;
        for (int i = 0; i < 6; i++) {
            if (coinValues[i].length() == 0) {
                errors += "Coin " + String(i + 1) + " is required.<br>";
            } else {
                validCoins++;
            }
        }
        
        if (validCoins > 0 && !validateSymbols(symbols)) {
            errors += "All coins must be valid Binance USDT trading pairs (e.g., BTCUSDT,ETHUSDT).<br>";
        }
        
        if (errors.length() > 0) {
            request->send(400, "text/html", 
                "<!DOCTYPE html><html><head><title>Validation Error</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>:root{--pico-background-color:#0f172a;--pico-color:#cbd5e1;--pico-h1-color:#f1f5f9;--pico-primary:#3b82f6;--pico-font-family:system-ui,sans-serif;--pico-spacing:1rem}body{font-family:var(--pico-font-family);background-color:var(--pico-background-color);color:var(--pico-color);text-align:center;margin:0;padding:calc(var(--pico-spacing)*2)}h1{color:var(--pico-h1-color)}.error{color:#dc2626}a{color:var(--pico-primary);text-decoration:none}</style></head>"
                "<body><h1>Validation Error</h1><p class='error'>" + errors + "</p>"
                "<a href='/'>Go Back</a></body></html>");
            return;
        }
        
        // Store new configurations
        new_ssid = ssid;
        new_password = password;
        has_new_credentials = true;
        
        new_symbols = symbols;
        has_new_symbols_config = true;
        
        request->send(200, "text/html", 
            "<!DOCTYPE html><html><head><title>Configuration Saved</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>:root{--pico-background-color:#0f172a;--pico-color:#cbd5e1;--pico-h1-color:#f1f5f9;--pico-font-family:system-ui,sans-serif;--pico-spacing:1rem}body{font-family:var(--pico-font-family);background-color:var(--pico-background-color);color:var(--pico-color);text-align:center;margin:0;padding:calc(var(--pico-spacing)*2)}h1{color:var(--pico-h1-color)}</style></head>"
            "<body><h1>Configuration Saved</h1><p>The device will now try to connect to: <strong>" + ssid + "</strong></p>"
            "<p>Cryptocurrency symbols have been saved.</p>"
            "<p>If WiFi connection is successful, this access point will be closed.</p></body></html>");
        delay(2000);
    });
    
    // Catch all for captive portal
    web_server->onNotFound([this](AsyncWebServerRequest *request){
        request->redirect("/");
    });
    
    web_server->begin();
    LOG_DEBUG("Web server started");
}

void NetworkManager::clearStoredWiFiConfig() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    LOG_DEBUG("WiFi configuration cleared from storage");
}

void NetworkManager::saveWiFiConfig(const String& ssid, const String& password) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    LOG_DEBUG("WiFi configuration saved to storage");
}

bool NetworkManager::loadStoredWiFiConfig(String& ssid, String& password) {
    preferences.begin("wifi", true); // Read-only
    
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    
    preferences.end();
    
    if (ssid.length() > 0) {
        LOG_DEBUG("WiFi configuration loaded from storage");
        return true;
    }
    
    LOG_DEBUG("No WiFi configuration found in storage");
    return false;
}

void NetworkManager::setReconfigurationRequested(bool requested) {
    preferences.begin("system", false);
    preferences.putBool("reconfig_req", requested);
    preferences.end();
    
    if (requested) {
        LOG_DEBUG("Reconfiguration flag set in persistent storage");
    } else {
        LOG_DEBUG("Reconfiguration flag cleared from persistent storage");
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
    LOG_DEBUG("Performing factory reset - clearing all stored data...");
    
    // Clear WiFi configuration
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    
    // Clear system configuration (including reconfiguration flag)
    preferences.begin("system", false);
    preferences.clear();
    preferences.end();
    
    LOG_DEBUG("Factory reset complete - all stored data cleared");
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
    LOG_DEBUG("Scanning for WiFi networks...");
    
    // Set to STA mode for scanning
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Clear any previous scan results
    WiFi.scanDelete();
    
    // Perform synchronous scan
    int n = WiFi.scanNetworks();
    
    LOG_DEBUG("WiFi scan completed. Found ");
    LOG_DEBUG(n);
    LOG_DEBUG(" networks");
    
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
        
        LOG_DEBUG("WiFi networks cached for AP mode");
        return true;
    } else if (n == 0) {
        LOG_DEBUG("No WiFi networks found");
        scanned_networks_json = "[]";
        has_scanned_networks = true;
        return true;
    } else {
        LOG_DEBUG("WiFi scan failed with error: ");
        LOG_DEBUG(n);
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

bool NetworkManager::validateSymbols(const String& symbols) const {
    if (symbols.length() == 0) return false;
    
    // Check for whitespace
    if (symbols.indexOf(' ') != -1 || symbols.indexOf('\t') != -1 || 
        symbols.indexOf('\n') != -1 || symbols.indexOf('\r') != -1) {
        return false;
    }
    
    // Split by commas and validate each symbol
    String temp = symbols;
    temp.toUpperCase();
    
    while (temp.length() > 0) {
        int comma_pos = temp.indexOf(',');
        String symbol = (comma_pos == -1) ? temp : temp.substring(0, comma_pos);
        
        if (symbol.length() == 0) return false; // Empty symbol
        
        // Check if symbol is valid format (letters/numbers, typically ends with USDT)
        if (symbol.length() < 6 || !symbol.endsWith("USDT")) {
            return false;
        }
        
        // Check for valid characters (A-Z, 0-9)
        for (int i = 0; i < symbol.length(); i++) {
            char c = symbol.charAt(i);
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                return false;
            }
        }
        
        if (comma_pos == -1) break;
        temp = temp.substring(comma_pos + 1);
    }
    
    return true;
}

bool NetworkManager::hasNewSymbolsConfig() const {
    return has_new_symbols_config;
}

String NetworkManager::getNewSymbols() const {
    return new_symbols;
}

void NetworkManager::clearNewSymbolsConfig() {
    has_new_symbols_config = false;
    new_symbols = "";
}

void NetworkManager::saveSymbolsConfig(const String& symbols) {
    preferences.begin("symbols", false);
    preferences.putString("symbols", symbols);
    preferences.end();
    LOG_DEBUG("Symbols configuration saved to storage");
}

bool NetworkManager::loadStoredSymbolsConfig(String& symbols) {
    preferences.begin("symbols", true); // Read-only
    
    symbols = preferences.getString("symbols", "BTCUSDT,ETHUSDT,BNBUSDT,ADAUSDT,SOLUSDT,DOGEUSDT");
    
    preferences.end();
    
    if (symbols.length() > 0) {
        LOG_DEBUG("Symbols configuration loaded from storage");
        return true;
    }
    
    LOG_DEBUG("No symbols configuration found in storage");
    return false;
}

/**
 * @brief Makes a secure HTTPS GET request with memory optimization for ESP32
 * 
 * Creates a WiFiClientSecure connection with certificate verification disabled
 * to reduce memory usage. Implements proper timeout handling and cleanup.
 * 
 * @param url Full HTTPS URL to request (must start with https://)
 * @param response String reference to store the response body
 * @param httpCode Integer reference to store the HTTP status code
 * @param timeout_ms Request timeout in milliseconds (default: 15000)
 * @return true if request succeeded (HTTP 200), false otherwise
 * 
 * @note Uses setInsecure() to skip SSL certificate verification
 * @note Automatically handles client cleanup to prevent memory leaks
 * @note Sets ESP32-CYD-Ticker user agent for API identification
 */
bool NetworkManager::httpGetSecure(const String& url, String& response, int& httpCode, unsigned long timeout_ms) {
    LOG_DEBUG("Free heap before HTTPS: " + String(ESP.getFreeHeap()) + " bytes");
    
    // Check if we have enough memory for SSL operation (at least 40KB recommended)
    size_t free_heap = ESP.getFreeHeap();
    if (free_heap < 40000) {
        LOG_ERROR("Insufficient memory for HTTPS connection: " + String(free_heap) + " bytes (need 40KB+)");
        httpCode = -32512; // SSL memory allocation error
        return false;
    }
    
    // Create WiFiClientSecure with minimal memory footprint
    WiFiClientSecure* client = new WiFiClientSecure;
    if (!client) {
        LOG_ERROR("Failed to create secure HTTP client - only " + String(ESP.getFreeHeap()) + " bytes free");
        httpCode = -7; // ESP32 HTTPClient error: too less RAM
        return false;
    }
    
    // Configure for minimal memory usage
    client->setInsecure(); // Skip certificate verification to save memory
    client->setTimeout(timeout_ms / 1000); // Convert to seconds
    
    HTTPClient http;
    http.begin(*client, url);
    http.setTimeout(timeout_ms);
    http.setUserAgent("ESP32-CYD-Ticker/1.0");
    
    LOG_DEBUG("Making HTTPS GET request to: " + url);
    LOG_DEBUG("Free heap during HTTPS setup: " + String(ESP.getFreeHeap()) + " bytes");
    
    httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        LOG_DEBUG("HTTPS response received, size: " + String(response.length()));
        LOG_DEBUG("Free heap after response: " + String(ESP.getFreeHeap()) + " bytes");
    } else {
        LOG_ERROR("HTTPS request failed with code: " + String(httpCode) + ", free heap: " + String(ESP.getFreeHeap()));
    }
    
    http.end();
    delete client;
    client = nullptr;
    
    LOG_DEBUG("Free heap after cleanup: " + String(ESP.getFreeHeap()) + " bytes");
    
    return httpCode == HTTP_CODE_OK;
}

bool NetworkManager::httpGet(const String& url, const String& api_key, String& response, int& httpCode) {
    // Check if URL is HTTPS and use secure method
    if (url.startsWith("https://")) {
        LOG_DEBUG("HTTPS URL detected, using secure HTTP client");
        return httpGetSecure(url, response, httpCode, 15000); // 15 second timeout
    }
    
    // Plain HTTP
    HTTPClient http;
    http.begin(url);
    
    // For Binance API, no special headers needed - it's a public endpoint
    
    httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        http.end();
        return true;
    }
    
    http.end();
    return false;
}