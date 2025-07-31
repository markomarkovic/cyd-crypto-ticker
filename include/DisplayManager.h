#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include "BinanceDataManager.h"

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager() = default;
    
    void initialize();
    void setupDarkTheme();
    void updateCryptoDisplay(const BinanceDataManager& crypto_manager, 
                            const String& wifi_info, 
                            const String& sync_status,
                            bool is_websocket_connected = true);
    
    void showConnectingMessage(const String& message);
    void showErrorMessage(const String& message);
    void showAPModeScreen(const String& ssid);
    
private:
    lv_obj_t* status_label_;
    lv_obj_t* wifi_info_label_;
    
    void createStatusLabel();
    void createCoinDisplay(const CoinData* coin_data, int coin_count, 
                          const String& error_message, int& y_offset,
                          bool is_websocket_connected = true);
    String formatPrice(float price) const;
    void parseTradingPair(const String& symbol, String& base_symbol, String& quote_symbol) const;
};

#endif // DISPLAY_MANAGER_H