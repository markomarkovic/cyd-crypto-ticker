#ifndef APPLICATION_STATE_MANAGER_H
#define APPLICATION_STATE_MANAGER_H

#include <Arduino.h>

class ApplicationStateManager {
public:
    enum class AppState {
        INITIALIZING,
        AP_MODE,
        CONNECTING,
        NORMAL_OPERATION,
        WIFI_RECONNECTING
    };
    
    enum class WiFiState {
        CONNECTED,
        DISCONNECTED,
        RECONNECTING,
        AP_MODE
    };
    
    ApplicationStateManager();
    
    // State management
    void setAppState(AppState state);
    AppState getAppState() const;
    void setWiFiState(WiFiState state);
    WiFiState getWiFiState() const;
    
    // WiFi reconnection management
    void startWiFiDisconnection();
    void resetWiFiDisconnection();
    bool isWiFiDisconnected() const;
    unsigned long getWiFiDisconnectionDuration() const;
    void setReconnectionMessageShown(bool shown);
    bool isReconnectionMessageShown() const;
    
    // Update timing management
    void updateWiFiTimestamp();
    void updateCryptoTimestamp();
    bool shouldUpdateWiFi(unsigned long wifi_update_interval) const;
    bool shouldUpdateCrypto(unsigned long crypto_update_interval) const;
    unsigned long getTimeSinceLastWiFiUpdate() const;
    unsigned long getTimeSinceLastCryptoUpdate() const;
    
    // LVGL timing
    void updateLVGLTick();
    
private:
    AppState current_app_state_;
    WiFiState current_wifi_state_;
    
    // WiFi reconnection state
    bool wifi_disconnection_detected_;
    unsigned long wifi_disconnection_start_;
    bool reconnection_message_shown_;
    
    // Update timing
    unsigned long last_wifi_update_;
    unsigned long last_crypto_update_;
    
    // LVGL timing
    unsigned long lv_last_tick_;
};

#endif // APPLICATION_STATE_MANAGER_H