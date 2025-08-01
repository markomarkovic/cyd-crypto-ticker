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
    bool handleTouch(lv_coord_t x, lv_coord_t y, BinanceDataManager& crypto_manager);
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
     * @brief Shows blue crosshair with timestamp and price at clicked chart coordinates
     * @param x_pos X coordinate of click position
     * @param y_pos Y coordinate of click position (relative to chart area)
     * @param price_min Minimum price in current chart range for calculation
     * @param price_max Maximum price in current chart range for calculation
     * @note Creates blue crosshair (vertical + horizontal lines) and timestamp/price label in top-left corner
     */
    void showPriceIndicator(lv_coord_t x_pos, lv_coord_t y_pos, float price_min, float price_max);
    
    /**
     * @brief Hide price indicator crosshair and label
     * @note Removes blue crosshair and timestamp/price display
     */
    void hidePriceIndicator();
    
    /**
     * @brief Check if price indicator should be auto-hidden after timeout
     * @note Call this regularly to auto-hide crosshair after 2 seconds
     */
    void checkPriceIndicatorTimeout();
    
    /**
     * @brief Shows interval selection overlay with grid of buttons
     * @note Creates 3x5 grid of interval buttons over the chart area
     */
    void showIntervalSelection();
    
    /**
     * @brief Hides interval selection overlay
     * @note Cleans up interval selection UI elements
     */
    void hideIntervalSelection();
    
    /**
     * @brief Checks if interval selection overlay is currently visible
     * @return true if interval selection is shown, false otherwise
     */
    bool isIntervalSelectionVisible() const;
    
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
    
    // Price indicator (blue crosshair + price label on click)
    lv_obj_t* price_indicator_line_;
    lv_obj_t* price_indicator_horizontal_line_;
    lv_obj_t* price_indicator_label_;
    unsigned long price_indicator_show_time_;
    
    // Interval selection overlay
    lv_obj_t* interval_overlay_;
    lv_obj_t* interval_buttons_[INTERVAL_COUNT];
    
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
    
    /**
     * @brief Calculate timestamp at clicked X position in chart
     * @param x_pos X coordinate of click position
     * @return Formatted timestamp string (YYYY-MM-DD HH:MM)
     */
    String calculateTimestampAtPosition(lv_coord_t x_pos);
    
    // Reference to crypto manager for accessing candlestick data
    const BinanceDataManager* crypto_manager_ref_;
};

#endif // DISPLAY_MANAGER_H