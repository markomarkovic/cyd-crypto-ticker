#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    bool connect(const char* ssid, const char* password, unsigned long timeout_ms = 20000);
    bool isConnected() const;
    String getSignalStrength() const;
    String getLocalIP() const;
    String getCurrentSSID() const;
    void disconnect();
    
    bool httpGet(const String& url, const String& api_key, String& response, int& httpCode);
    
    // AP mode functionality
    bool startAPMode();
    void stopAPMode();
    bool isAPMode() const;
    String getAPSSID() const;
    String getAPPassword() const;
    void handleAPMode();
    bool hasNewCredentials() const;
    String getNewSSID() const;
    String getNewPassword() const;
    void clearNewCredentials();
    
    // WiFi config persistence
    void clearStoredWiFiConfig();
    void saveWiFiConfig(const String& ssid, const String& password);
    bool loadStoredWiFiConfig(String& ssid, String& password);
    
    // Reconfiguration flag management
    void setReconfigurationRequested(bool requested);
    bool isReconfigurationRequested() const;
    void clearReconfigurationFlag();
    
    // Factory reset - clears all stored data
    void factoryReset();
    
    // Boot-time WiFi scanning
    bool scanWiFiNetworks();
    String getScannedNetworksJSON() const;
    
    // API configuration methods
    bool validateUUID(const String& uuid) const;
    bool validateCoinIds(const String& coin_ids) const;
    bool hasNewAPIConfig() const;
    String getNewAPIKey() const;
    String getNewCoinIds() const;
    void clearNewAPIConfig();
    void saveAPIConfig(const String& api_key, const String& coin_ids);
    bool loadStoredAPIConfig(String& api_key, String& coin_ids);
    
private:
    String convertRSSIToText(int rssi) const;
    String generateRandomPassword(int length = 8);
    void setupWebServer();
    String escapeJsonString(const String& str);
    
    // AP mode variables
    bool ap_mode_active;
    String ap_ssid;
    String ap_password;
    AsyncWebServer* web_server;
    DNSServer* dns_server;
    
    // New credentials from web portal
    bool has_new_credentials;
    String new_ssid;
    String new_password;
    
    // Preferences for WiFi config storage
    Preferences preferences;
    
    // WiFi scan state
    bool scan_in_progress;
    AsyncWebServerRequest* pending_scan_request;
    
    // Pre-scanned networks storage
    String scanned_networks_json;
    bool has_scanned_networks;
    
    // New API configuration from web portal
    bool has_new_api_config;
    String new_api_key;
    String new_coin_ids;
};

#endif // NETWORK_MANAGER_H