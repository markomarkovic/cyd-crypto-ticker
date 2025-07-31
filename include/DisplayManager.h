#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include "BinanceDataManager.h"
#include "constants.h"

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
    
    // Screen state management
    void setScreenState(ScreenState state);
    ScreenState getScreenState() const;
    void showDetailScreen(int coinIndex, const BinanceDataManager& crypto_manager);
    void showListScreen();
    
    // Touch event handling
    bool handleTouch(lv_coord_t x, lv_coord_t y, const BinanceDataManager& crypto_manager);
    int getSelectedCoinIndex() const;
    
    // Chart update methods
    /**
     * @brief Updates the candlestick chart area with new data
     * @param crypto_manager Reference to data manager containing candlestick data
     * @note Redraws the entire chart container with latest data
     */
    void updateChartArea(const BinanceDataManager& crypto_manager);
    
    /**
     * @brief Updates coin info display in detail view with real-time data
     * @param crypto_manager Reference to data manager containing current prices
     * @note Updates price, 24h change, and background colors based on trend
     */
    void updateDetailCoinInfo(const BinanceDataManager& crypto_manager);
    
    /**
     * @brief Shows price indicator at clicked chart coordinates
     * @param x_pos X coordinate of click position
     * @param y_pos Y coordinate of click position (relative to chart area)
     * @param price_min Minimum price in current chart range for calculation
     * @param price_max Maximum price in current chart range for calculation
     * @note Creates blue horizontal line and price label in top-left corner
     */
    void showPriceIndicator(lv_coord_t x_pos, lv_coord_t y_pos, float price_min, float price_max);
    
private:
    lv_obj_t* status_label_;
    lv_obj_t* wifi_info_label_;
    
    // Screen state management
    ScreenState current_screen_;
    int selected_coin_index_;
    unsigned long last_touch_time_;
    
    // Chart area reference for updates
    lv_obj_t* chart_container_;
    lv_obj_t* coin_info_container_;
    
    // Price indicator (blue line + price label on click)
    lv_obj_t* price_indicator_line_;
    lv_obj_t* price_indicator_label_;
    
    void createStatusLabel();
    void createCoinDisplay(const CoinData* coin_data, int coin_count, 
                          const String& error_message, int& y_offset,
                          bool is_websocket_connected = true);
    String formatPrice(float price) const;
    void parseTradingPair(const String& symbol, String& base_symbol, String& quote_symbol) const;
    
    // Chart rendering methods
    void drawCandlestickChart(const CandlestickData* candles, int count, lv_coord_t width, lv_coord_t height, const String& interval);
    void drawSingleCandle(lv_obj_t* parent, const CandlestickData& candle, 
                         lv_coord_t x_pos, lv_coord_t chart_top, lv_coord_t chart_height, 
                         float price_min, float price_max);
    void drawChartLabels(const CandlestickData* candles, int count, float price_min, float price_max, const String& interval);
};

#endif // DISPLAY_MANAGER_H