#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include "CryptoDataManager.h"

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager() = default;
    
    void initialize();
    void setupDarkTheme();
    void updateCryptoDisplay(const CryptoDataManager& crypto_manager, 
                            const String& wifi_info, 
                            const String& sync_status);
    
    bool checkPullToRefresh();
    void showConnectingMessage(const String& message);
    void showErrorMessage(const String& message);
    void showAPModeScreen(const String& ssid);
    
private:
    lv_obj_t* status_label_;
    lv_obj_t* wifi_info_label_;
    lv_obj_t* refresh_label_;
    
    bool is_refreshing_;
    
    void createStatusLabel();
    void createWifiInfoLabel(int y_offset, const String& wifi_info, const String& sync_status);
    void createCoinDisplay(const CoinData* coin_data, int coin_count, 
                          bool has_stale_data, const String& error_message, int& y_offset);
    String formatPrice(float price) const;
};

#endif // DISPLAY_MANAGER_H