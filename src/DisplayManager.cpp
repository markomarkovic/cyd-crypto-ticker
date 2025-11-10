#include "DisplayManager.h"
#include "constants.h"
#include "jetbrains_mono_fonts.h"

DisplayManager::DisplayManager() 
    : status_label_(nullptr), wifi_info_label_(nullptr), 
      current_screen_(LIST_SCREEN), selected_coin_index_(-1), last_touch_time_(0),
      chart_container_(nullptr), coin_info_container_(nullptr),
      price_indicator_line_(nullptr), price_indicator_horizontal_line_(nullptr), price_indicator_label_(nullptr),
      price_indicator_show_time_(0), interval_overlay_(nullptr), crypto_manager_ref_(nullptr) {
    // Initialize interval buttons array
    for (int i = 0; i < INTERVAL_COUNT; i++) {
        interval_buttons_[i] = nullptr;
    }
}

void DisplayManager::initialize() {
    smartdisplay_init();
    setupDarkTheme();
    
    // Optional rotation settings can be uncommented as needed
    // auto display = lv_display_get_default();
    // lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
}

void DisplayManager::setupDarkTheme() {
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, COLOR_DARK_BG, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    
    // Explicitly disable scrolling
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
}

void DisplayManager::updateCryptoDisplay(const BinanceDataManager& crypto_manager, 
                                        const String& wifi_info, 
                                        const String& sync_status,
                                        bool is_websocket_connected) {
    // Clear existing children
    lv_obj_clean(lv_screen_active());
    
    // Ensure scrolling stays disabled after clearing
    lv_obj_t* screen = lv_screen_active();
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    const CoinData* coin_data = crypto_manager.getCoinData();
    int coin_count = crypto_manager.getCoinCount();
    String error_message = crypto_manager.getLastError();
    int y_offset = 0; // Start position for crypto data (no top spacing)

    // Show error indicator at top if there's an error but we have old data
    if (!error_message.isEmpty() && crypto_manager.hasValidData()) {
        lv_obj_t* error_indicator = lv_label_create(lv_screen_active());
        lv_label_set_text(error_indicator, ("! " + error_message).c_str());
        lv_obj_set_pos(error_indicator, 0, y_offset);
        lv_obj_set_width(error_indicator, 240);
        lv_obj_set_style_text_align(error_indicator, LV_TEXT_ALIGN_CENTER, 0);
        
        lv_obj_set_style_text_color(error_indicator, COLOR_BRIGHT_RED, 0);
        lv_obj_set_style_bg_color(error_indicator, COLOR_DARK_BG, 0);
        lv_obj_set_style_bg_opa(error_indicator, LV_OPA_90, 0);
        lv_obj_set_style_pad_all(error_indicator, 3, 0);
        y_offset += 20; // Move down past error indicator
    }

    createCoinDisplay(coin_data, coin_count, error_message, y_offset, is_websocket_connected);
    
    // Check if price indicator should be auto-hidden (only in detail screen)
    if (current_screen_ == DETAIL_SCREEN) {
        checkPriceIndicatorTimeout();
    }
    
    // Force immediate display refresh
    lv_refr_now(NULL);
    lv_timer_handler();
}


void DisplayManager::showConnectingMessage(const String& message) {
    // Clear existing display content first to avoid conflicts
    lv_obj_clean(lv_screen_active());
    
    // Reset label pointer since we cleared the screen
    status_label_ = nullptr;
    wifi_info_label_ = nullptr;
    
    createStatusLabel();
    lv_label_set_text(status_label_, message.c_str());
    lv_obj_set_style_text_color(status_label_, COLOR_WHITE_TEXT, 0);
    
    // Force immediate display refresh
    lv_refr_now(NULL);
    lv_timer_handler();
}

void DisplayManager::showErrorMessage(const String& message) {
    // Clear existing display content first to avoid conflicts
    lv_obj_clean(lv_screen_active());
    
    // Reset label pointer since we cleared the screen
    status_label_ = nullptr;
    wifi_info_label_ = nullptr;
    
    createStatusLabel();
    lv_label_set_text(status_label_, message.c_str());
    lv_obj_set_style_text_color(status_label_, COLOR_BRIGHT_RED, 0);
    
    // Force immediate display refresh
    lv_refr_now(NULL);
    lv_timer_handler();
}

void DisplayManager::createStatusLabel() {
    if (!status_label_) {
        status_label_ = lv_label_create(lv_screen_active());
        lv_obj_set_width(status_label_, LV_SIZE_CONTENT);
        lv_obj_set_height(status_label_, LV_SIZE_CONTENT);
        lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(status_label_, &jetbrains_mono_12, 0);
    }
}


void DisplayManager::createCoinDisplay(const CoinData* coin_data, int coin_count, 
                                      const String& error_message, int& y_offset,
                                      bool is_websocket_connected) {
    // Build compact coin status display - only show when there are errors
    String coin_status = "";
    bool has_errors = false;
    for (int i = 0; i < coin_count; i++) {
        if (!coin_data[i].valid) {
            if (has_errors) coin_status += " ";
            coin_status += coin_data[i].symbol + ":ERR";
            has_errors = true;
        }
    }
    if (has_errors) {
        LOG_DEBUG("Coin errors: " + coin_status);
    }

    for (int i = 0; i < coin_count; i++) {
        if (coin_data[i].valid) {
            // Create container for this coin
            lv_obj_t* coin_container = lv_obj_create(lv_screen_active());
            int container_height = (i < 2) ? 54 : 53; // First 2 items slightly taller to fill screen (2×54 + 4×53 = 320px)
            lv_obj_set_size(coin_container, 240, container_height);
            lv_obj_set_pos(coin_container, 0, y_offset);

            // Set background color based on 24h price change
            lv_color_t bg_color = (coin_data[i].change_percent_24h >= 0) ? 
                COLOR_TWILIGHT_GREEN : COLOR_TWILIGHT_RED;
            lv_obj_set_style_bg_color(coin_container, bg_color, 0);
            lv_obj_set_style_bg_opa(coin_container, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(coin_container, 0, 0);
            lv_obj_set_style_radius(coin_container, 0, 0);
            lv_obj_set_style_pad_all(coin_container, 8, 0);

            y_offset += container_height; // Move down for next coin

            // Parse trading pair symbols
            String base_symbol, quote_symbol;
            parseTradingPair(coin_data[i].symbol, base_symbol, quote_symbol);
            
            // Base symbol (top-left, aligned with change indicators)
            lv_obj_t* base_symbol_label = lv_label_create(coin_container);
            lv_label_set_text(base_symbol_label, base_symbol.c_str());
            lv_obj_align(base_symbol_label, LV_ALIGN_LEFT_MID, 3, -9); // Same vertical position as change indicators (9px above center)
            lv_obj_set_style_text_color(base_symbol_label, is_websocket_connected ? COLOR_GREY_TEXT : COLOR_MUTED_GREY, 0);
            lv_obj_set_style_text_font(base_symbol_label, &jetbrains_mono_12, 0); // Same as change indicators
            
            // Quote symbol (below base symbol, more muted)
            lv_obj_t* quote_symbol_label = lv_label_create(coin_container);
            lv_label_set_text(quote_symbol_label, quote_symbol.c_str()); // No slash
            lv_obj_align(quote_symbol_label, LV_ALIGN_LEFT_MID, 3, 9); // 9px below center (same as bottom change indicator)
            // Make it even more muted (75% mix with background)
            lv_color_t very_muted_color = lv_color_mix(COLOR_GREY_TEXT, COLOR_DARK_BG, 64); // 25% grey, 75% background
            lv_obj_set_style_text_color(quote_symbol_label, very_muted_color, 0);
            lv_obj_set_style_text_font(quote_symbol_label, &jetbrains_mono_12, 0); // Same as change indicators

            // 24h change (top right) - fixed position from center
            lv_obj_t* change_24h_label = lv_label_create(coin_container);
            String change_24h_text = (coin_data[i].change_24h >= 0 ? "" : "-") + formatPrice(abs(coin_data[i].change_24h));
            lv_label_set_text(change_24h_label, change_24h_text.c_str());
            lv_obj_align(change_24h_label, LV_ALIGN_RIGHT_MID, 0, -9); // 9px above center

            lv_color_t change_24h_color;
            if (is_websocket_connected) {
                change_24h_color = (coin_data[i].change_24h >= 0) ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED;
            } else {
                change_24h_color = (coin_data[i].change_24h >= 0) ? COLOR_MUTED_GREEN : COLOR_MUTED_RED;
            }
            lv_obj_set_style_text_color(change_24h_label, change_24h_color, 0);
            lv_obj_set_style_text_font(change_24h_label, &jetbrains_mono_12, 0);

            // 24h change percent (bottom right) - fixed position from center
            lv_obj_t* change_percent_label = lv_label_create(coin_container);
            String change_percent_text = (coin_data[i].change_percent_24h >= 0 ? "" : "-") + String(abs(coin_data[i].change_percent_24h), 2) + "%";
            lv_label_set_text(change_percent_label, change_percent_text.c_str());
            lv_obj_align(change_percent_label, LV_ALIGN_RIGHT_MID, 0, 9); // 9px below center

            lv_color_t change_percent_color;
            if (is_websocket_connected) {
                change_percent_color = (coin_data[i].change_percent_24h >= 0) ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED;
            } else {
                change_percent_color = (coin_data[i].change_percent_24h >= 0) ? COLOR_MUTED_GREEN : COLOR_MUTED_RED;
            }
            lv_obj_set_style_text_color(change_percent_label, change_percent_color, 0);
            lv_obj_set_style_text_font(change_percent_label, &jetbrains_mono_12, 0);

            // Price with larger font and styling (centered) - remove dollar sign as requested
            lv_obj_t* price_label = lv_label_create(coin_container);
            String price_text = formatPrice(coin_data[i].price);
            lv_label_set_text(price_label, price_text.c_str());
            lv_obj_align(price_label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(price_label, is_websocket_connected ? COLOR_WHITE_TEXT : COLOR_MUTED_WHITE, 0);
            lv_obj_set_style_text_font(price_label, &jetbrains_mono_22, 0);
        }
    }

}

String DisplayManager::formatPrice(float price) const {
    // Format price with comma separators for better readability
    // Don't show decimal places if price >= 100
    int decimal_places = (price >= 100.0f) ? 0 : 2;
    String price_str = String(price, decimal_places);

    // Find the decimal point
    int decimal_pos = price_str.indexOf('.');
    if (decimal_pos == -1)
        decimal_pos = price_str.length();

    // Add commas from right to left in the integer part
    String result = "";
    int digits_count = 0;

    // Process integer part (before decimal)
    for (int i = decimal_pos - 1; i >= 0; i--) {
        if (digits_count > 0 && digits_count % 3 == 0) {
            result = "," + result;
        }
        result = price_str.charAt(i) + result;
        digits_count++;
    }

    // Add decimal part if it exists
    if (decimal_pos < price_str.length()) {
        result += price_str.substring(decimal_pos);
    }

    return result;
}

void DisplayManager::showAPModeScreen(const String& ssid) {
    // Clear existing children
    lv_obj_clean(lv_screen_active());
    
    
    // Title
    lv_obj_t* title_label = lv_label_create(lv_screen_active());
    lv_label_set_text(title_label, "WiFi Configuration Mode");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(title_label, COLOR_WHITE_TEXT, 0);
    lv_obj_set_style_text_font(title_label, &jetbrains_mono_16, 0);
    
    // Instructions
    lv_obj_t* instruction_label = lv_label_create(lv_screen_active());
    lv_label_set_text(instruction_label, "Connect to this WiFi network:");
    lv_obj_align(instruction_label, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_color(instruction_label, COLOR_GREY_TEXT, 0);
    lv_obj_set_style_text_font(instruction_label, LV_FONT_DEFAULT, 0);
    
    // SSID
    lv_obj_t* ssid_label = lv_label_create(lv_screen_active());
    lv_label_set_text(ssid_label, ("" + ssid).c_str());
    lv_obj_align(ssid_label, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_text_color(ssid_label, COLOR_BRIGHT_GREEN, 0);
    lv_obj_set_style_text_font(ssid_label, &jetbrains_mono_14, 0);
    
    // Web portal instruction
    lv_obj_t* web_label = lv_label_create(lv_screen_active());
    lv_label_set_text(web_label, "Open your browser to configure\nWiFi settings. The setup page\nshould open automatically.");
    lv_obj_align(web_label, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_text_color(web_label, COLOR_GREY_TEXT, 0);
    lv_obj_set_style_text_font(web_label, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_align(web_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // IP address info
    lv_obj_t* ip_label = lv_label_create(lv_screen_active());
    lv_label_set_text(ip_label, "Configuration URL:\n192.168.4.1");
    lv_obj_align(ip_label, LV_ALIGN_TOP_MID, 0, 200);
    lv_obj_set_style_text_color(ip_label, COLOR_BRIGHT_GREEN, 0);
    lv_obj_set_style_text_font(ip_label, &jetbrains_mono_14, 0);
    lv_obj_set_style_text_align(ip_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Force immediate display refresh
    lv_refr_now(NULL);
    lv_timer_handler();
}



void DisplayManager::parseTradingPair(const String& symbol, String& base_symbol, String& quote_symbol) const {
    // Handle common Binance trading pairs
    if (symbol.endsWith("USDT")) {
        base_symbol = symbol.substring(0, symbol.length() - 4);
        quote_symbol = "USDT";
    } else if (symbol.endsWith("USDC")) {
        base_symbol = symbol.substring(0, symbol.length() - 4);
        quote_symbol = "USDC";
    } else if (symbol.endsWith("BTC")) {
        base_symbol = symbol.substring(0, symbol.length() - 3);
        quote_symbol = "BTC";
    } else if (symbol.endsWith("ETH")) {
        base_symbol = symbol.substring(0, symbol.length() - 3);
        quote_symbol = "ETH";
    } else if (symbol.endsWith("BNB")) {
        base_symbol = symbol.substring(0, symbol.length() - 3);
        quote_symbol = "BNB";
    } else if (symbol.endsWith("BUSD")) {
        base_symbol = symbol.substring(0, symbol.length() - 4);
        quote_symbol = "BUSD";
    } else if (symbol.endsWith("FDUSD")) {
        base_symbol = symbol.substring(0, symbol.length() - 5);
        quote_symbol = "FDUSD";
    } else {
        // Fallback: try to detect common patterns
        // For pairs like BTCETH, ETHBTC, etc. - this is more complex
        // For now, just show the full symbol as base and empty quote
        base_symbol = symbol;
        quote_symbol = "";
    }
}

// Screen state management methods
void DisplayManager::setScreenState(ScreenState state) {
    current_screen_ = state;
}

ScreenState DisplayManager::getScreenState() const {
    return current_screen_;
}

void DisplayManager::showDetailScreen(int coinIndex, const BinanceDataManager& crypto_manager) {
    LOG_INFO("showDetailScreen called for coin " + String(coinIndex));
    current_screen_ = DETAIL_SCREEN;
    selected_coin_index_ = coinIndex;
    
    // Clear existing display content
    lv_obj_clean(lv_screen_active());
    
    // Clear price indicator references since screen was cleaned
    price_indicator_line_ = nullptr;
    price_indicator_horizontal_line_ = nullptr;
    price_indicator_label_ = nullptr;
    price_indicator_show_time_ = 0;
    
    // Ensure scrolling stays disabled
    lv_obj_t* screen = lv_screen_active();
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    
    const CoinData* coin_data = crypto_manager.getCoinData();
    int coin_count = crypto_manager.getCoinCount();
    
    if (coinIndex < 0 || coinIndex >= coin_count || !coin_data[coinIndex].valid) {
        showErrorMessage("Invalid coin selected");
        return;
    }
    
    const CoinData& coin = coin_data[coinIndex];
    
    // Create coin info container at top (height 60px as per COIN_INFO_HEIGHT)
    // Store reference for real-time updates
    coin_info_container_ = lv_obj_create(screen);
    lv_obj_set_size(coin_info_container_, 240, COIN_INFO_HEIGHT);
    lv_obj_set_pos(coin_info_container_, 0, 0);
    
    // Set background color based on 24h change
    lv_color_t bg_color = (coin.change_percent_24h >= 0) ? 
        COLOR_TWILIGHT_GREEN : COLOR_TWILIGHT_RED;
    lv_obj_set_style_bg_color(coin_info_container_, bg_color, 0);
    lv_obj_set_style_bg_opa(coin_info_container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(coin_info_container_, 0, 0);
    lv_obj_set_style_radius(coin_info_container_, 0, 0); // No rounded corners
    lv_obj_set_style_pad_all(coin_info_container_, 8, 0);
    
    // Parse trading pair
    String base_symbol, quote_symbol;
    parseTradingPair(coin.symbol, base_symbol, quote_symbol);
    
    // Base symbol (top-left, aligned with change indicators)
    lv_obj_t* base_label = lv_label_create(coin_info_container_);
    lv_label_set_text(base_label, base_symbol.c_str());
    lv_obj_align(base_label, LV_ALIGN_LEFT_MID, 3, -9); // Same vertical position as change indicators
    lv_obj_set_style_text_color(base_label, COLOR_GREY_TEXT, 0);
    lv_obj_set_style_text_font(base_label, &jetbrains_mono_12, 0);
    
    // Quote symbol (below base symbol, more muted)
    lv_obj_t* quote_label = lv_label_create(coin_info_container_);
    lv_label_set_text(quote_label, quote_symbol.c_str());
    lv_obj_align(quote_label, LV_ALIGN_LEFT_MID, 3, 9); // 9px below center
    // Make it even more muted (75% mix with background)
    lv_color_t very_muted_color = lv_color_mix(COLOR_GREY_TEXT, COLOR_DARK_BG, 64); // 25% grey, 75% background
    lv_obj_set_style_text_color(quote_label, very_muted_color, 0);
    lv_obj_set_style_text_font(quote_label, &jetbrains_mono_12, 0);
    
    // Price with larger font and styling (centered)
    lv_obj_t* price_label = lv_label_create(coin_info_container_);
    lv_label_set_text(price_label, formatPrice(coin.price).c_str());
    lv_obj_align(price_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(price_label, COLOR_WHITE_TEXT, 0);
    lv_obj_set_style_text_font(price_label, &jetbrains_mono_22, 0);
    
    // 24h change (top right) - fixed position from center
    lv_obj_t* change_label = lv_label_create(coin_info_container_);
    String change_text = (coin.change_24h >= 0 ? "" : "-") + formatPrice(abs(coin.change_24h));
    lv_label_set_text(change_label, change_text.c_str());
    lv_obj_align(change_label, LV_ALIGN_RIGHT_MID, 0, -9); // 9px above center
    lv_obj_set_style_text_color(change_label, coin.change_24h >= 0 ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED, 0);
    lv_obj_set_style_text_font(change_label, &jetbrains_mono_12, 0);
    
    // 24h change percent (bottom right) - fixed position from center
    lv_obj_t* percent_label = lv_label_create(coin_info_container_);
    String percent_text = (coin.change_percent_24h >= 0 ? "" : "-") + String(abs(coin.change_percent_24h), 2) + "%";
    lv_label_set_text(percent_label, percent_text.c_str());
    lv_obj_align(percent_label, LV_ALIGN_RIGHT_MID, 0, 9); // 9px below center
    lv_obj_set_style_text_color(percent_label, coin.change_percent_24h >= 0 ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED, 0);
    lv_obj_set_style_text_font(percent_label, &jetbrains_mono_12, 0);
    
    // Create chart container below coin info - no padding, fill exact space
    chart_container_ = lv_obj_create(screen);
    lv_obj_set_size(chart_container_, 240, 320 - COIN_INFO_HEIGHT);
    lv_obj_set_pos(chart_container_, 0, COIN_INFO_HEIGHT);
    lv_obj_set_style_bg_color(chart_container_, COLOR_DARK_BG, 0);
    lv_obj_set_style_border_width(chart_container_, 0, 0);
    lv_obj_set_style_pad_all(chart_container_, 0, 0);
    // Disable scrolling on chart container
    lv_obj_clear_flag(chart_container_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(chart_container_, LV_SCROLLBAR_MODE_OFF);
    
    // Check if we have candlestick data for this symbol
    if (crypto_manager.hasCandlestickData() && 
        crypto_manager.getCurrentCandlestickSymbol() == coin.symbol) {
        
        const CandlestickData* candles = crypto_manager.getCandlestickData();
        int candle_count = crypto_manager.getCandlestickCount();
        
        if (candle_count > 0) {
            String interval = crypto_manager.getCurrentCandlestickInterval();
            drawCandlestickChart(candles, candle_count, 240, 320 - COIN_INFO_HEIGHT, interval); // Full width and height
        } else {
            // Show loading message
            lv_obj_t* loading_label = lv_label_create(chart_container_);
            lv_label_set_text(loading_label, "Loading chart data...");
            lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(loading_label, COLOR_GREY_TEXT, 0);
        }
    } else {
        // Show loading message
        lv_obj_t* loading_label = lv_label_create(chart_container_);
        lv_label_set_text(loading_label, "Loading chart data...");
        lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(loading_label, COLOR_GREY_TEXT, 0);
        lv_obj_set_style_text_font(loading_label, &jetbrains_mono_12, 0);
    }
    
    // Force immediate display refresh
    lv_refr_now(NULL);
    lv_timer_handler();
    
    LOG_INFO("Detail screen displayed successfully");
}

void DisplayManager::showListScreen() {
    LOG_INFO("showListScreen called - switching back to list view");
    
    // Hide interval selection if visible
    hideIntervalSelection();
    
    current_screen_ = LIST_SCREEN;
    selected_coin_index_ = -1;
    
    // Clear any chart container reference
    chart_container_ = nullptr;
    coin_info_container_ = nullptr;
    price_indicator_line_ = nullptr;
    price_indicator_horizontal_line_ = nullptr;
    price_indicator_label_ = nullptr;
    price_indicator_show_time_ = 0;
    
    // List screen will be updated by the regular updateCryptoDisplay call
    LOG_INFO("Screen state changed to LIST_SCREEN");
}

// Touch event handling
bool DisplayManager::handleTouch(lv_coord_t x, lv_coord_t y, BinanceDataManager& crypto_manager) {
    unsigned long now = millis();
    
    // Debounce touch events
    if (now - last_touch_time_ < TOUCH_DEBOUNCE_MS) {
        return false;
    }
    last_touch_time_ = now;
    
    LOG_DEBUG("Touch detected at (" + String(x) + ", " + String(y) + ")");
    
    if (current_screen_ == LIST_SCREEN) {
        // Calculate which coin was touched in list view
        const CoinData* coin_data = crypto_manager.getCoinData();
        int coin_count = crypto_manager.getCoinCount();
        
        // Each coin container is approximately 53-54px high
        // First 2 items: 54px each, last 4 items: 53px each
        int touched_coin = -1;
        int cumulative_height = 0;
        
        for (int i = 0; i < coin_count && i < 6; i++) {  // Max 6 coins displayed
            int item_height = (i < 2) ? 54 : 53;
            
            if (y >= cumulative_height && y < cumulative_height + item_height) {
                touched_coin = i;
                break;
            }
            cumulative_height += item_height;
        }
        
        if (touched_coin >= 0 && touched_coin < coin_count && coin_data[touched_coin].valid) {
            LOG_INFO("Coin " + String(touched_coin) + " (" + coin_data[touched_coin].symbol + ") touched, switching to detail view");
            showDetailScreen(touched_coin, crypto_manager);
            return true;
        }
    } else if (current_screen_ == DETAIL_SCREEN) {
        LOG_DEBUG("Touch in detail screen at y=" + String(y) + ", COIN_INFO_HEIGHT=" + String(COIN_INFO_HEIGHT));
        
        // Check if interval selection overlay is visible
        if (isIntervalSelectionVisible()) {
            
            // Check if any interval button was clicked
            for (int i = 0; i < INTERVAL_COUNT; i++) {
                if (interval_buttons_[i]) {
                    lv_coord_t btn_x = lv_obj_get_x(interval_buttons_[i]);
                    lv_coord_t btn_y = lv_obj_get_y(interval_buttons_[i]) + COIN_INFO_HEIGHT; // Adjust for chart container offset
                    lv_coord_t btn_w = lv_obj_get_width(interval_buttons_[i]);
                    lv_coord_t btn_h = lv_obj_get_height(interval_buttons_[i]);
                    
                    
                    if (x >= btn_x && x < btn_x + btn_w && y >= btn_y && y < btn_y + btn_h) {
                        LOG_INFO("Interval changed to: " + String(SUPPORTED_INTERVALS[i]));
                        
                        // Set new interval and hide overlay
                        crypto_manager.setCurrentCandlestickInterval(SUPPORTED_INTERVALS[i]);
                        hideIntervalSelection();
                        
                        // Return special value to indicate interval changed (need data refetch)
                        return true; // Will trigger data refetch in ApplicationController
                    }
                }
            }
            
            // If clicked outside buttons, hide interval selection
            hideIntervalSelection();
            return false;
        }
        
        // Check if price area was touched (top COIN_INFO_HEIGHT pixels)
        if (y <= COIN_INFO_HEIGHT) {
            LOG_INFO("Price area touched, returning to list view");
            showListScreen();
            return true;
        } else {
            // Check if interval label area was clicked (bottom-left corner)
            lv_coord_t chart_y = y - COIN_INFO_HEIGHT; // Convert to chart-relative coordinates
            lv_coord_t chart_height = lv_obj_get_height(chart_container_);
            
            // Expand clickable area: 50x45px instead of 35x28px for easier clicking  
            // Interval container now at absolute bottom: x: 0 to 35, y: chart_height-28 to chart_height
            // Expanded clickable area: x: 0 to 50, y: chart_height-45 to chart_height
            
            if (x >= 0 && x <= 50 && chart_y >= chart_height - 45 && chart_y <= chart_height) {
                LOG_DEBUG("Interval selection opened");
                showIntervalSelection();
                return false; // No screen change
            }
            
            // Touch in chart area - show price indicator
            LOG_DEBUG("Touch in chart area at y=" + String(y) + ", showing price indicator");
            
            // Get the current price range from candlestick data
            const CandlestickData* candles = crypto_manager.getCandlestickData();
            int candle_count = crypto_manager.getCandlestickCount();
            
            if (candles && candle_count > 0) {
                // Find actual data price range
                float data_min = candles[0].low;
                float data_max = candles[0].high;
                
                for (int i = 1; i < candle_count; i++) {
                    if (candles[i].valid) {
                        if (candles[i].low < data_min) data_min = candles[i].low;
                        if (candles[i].high > data_max) data_max = candles[i].high;
                    }
                }
                
                // Create padded range for price indicator calculation (matches chart scaling)
                float price_range = data_max - data_min;
                float chart_min = data_min - (price_range * CHART_PRICE_PADDING);
                float chart_max = data_max + (price_range * CHART_PRICE_PADDING);
                
                // Store reference to crypto manager for timestamp calculation
                crypto_manager_ref_ = &crypto_manager;
                
                // Convert touch coordinates to chart-relative coordinates
                lv_coord_t chart_x = x;
                lv_coord_t chart_y_adj = y - COIN_INFO_HEIGHT;
                showPriceIndicator(chart_x, chart_y_adj, chart_min, chart_max);
                return false; // No screen change, just showing price indicator
            }
            return false; // No valid candlestick data
        }
    }
    
    return false;
}

int DisplayManager::getSelectedCoinIndex() const {
    return selected_coin_index_;
}

// Chart rendering methods
void DisplayManager::drawCandlestickChart(const CandlestickData* candles, int count, lv_coord_t width, lv_coord_t height, const String& interval) {
    if (count == 0 || !chart_container_) return;
    
    lv_obj_t* parent = chart_container_;
    
    // Find actual data price range
    float data_min = candles[0].low;
    float data_max = candles[0].high;
    
    for (int i = 1; i < count; i++) {
        if (candles[i].valid) {
            if (candles[i].low < data_min) data_min = candles[i].low;
            if (candles[i].high > data_max) data_max = candles[i].high;
        }
    }
    
    // Create padded range for chart scaling
    float price_range = data_max - data_min;
    float price_min = data_min - (price_range * CHART_PRICE_PADDING);
    float price_max = data_max + (price_range * CHART_PRICE_PADDING);
    
    // Calculate chart dimensions - use full available space
    lv_coord_t chart_top = 0; // Start at top of chart container
    lv_coord_t chart_height = height; // Use full height
    
    // Calculate optimal number of candles that fit on screen
    // Screen width: 240px, margins: 20px (10px each side), available: 220px
    // Minimum spacing: CANDLE_BODY_WIDTH (5px) + 2px gap = 7px per candle
    lv_coord_t available_width = width - 20; // 220px available
    lv_coord_t min_candle_spacing = CANDLE_BODY_WIDTH + 2; // 7px minimum
    int max_visible_candles = available_width / min_candle_spacing; // ~31 candles max
    
    // Use optimal spacing for the actual candle count we have
    int visible_candles = (count < max_visible_candles) ? count : max_visible_candles;
    int extended_candles = visible_candles + 2; // Add 2 more candles extending past left edge
    lv_coord_t candle_spacing = available_width / visible_candles;
    
    // Track the highest and lowest points in the VISIBLE/EXTENDED data only
    float actual_high = -1;
    float actual_low = -1;
    int highest_index = -1;
    int lowest_index = -1;
    
    // Find highest/lowest within the candles we'll actually display
    for (int i = 0; i < extended_candles && i < count; i++) {
        int actual_index = count - 1 - i; // Start from newest
        if (actual_index >= 0 && candles[actual_index].valid) {
            if (actual_high < 0 || candles[actual_index].high > actual_high) {
                actual_high = candles[actual_index].high;
                highest_index = actual_index;
            }
            if (actual_low < 0 || candles[actual_index].low < actual_low) {
                actual_low = candles[actual_index].low;
                lowest_index = actual_index;
            }
        }
    }
    
    // Draw each candlestick from right to left (newest on right) - add 2 extra to show continuity
    for (int i = 0; i < extended_candles && i < count; i++) {
        int actual_index = count - 1 - i; // Start from newest
        if (actual_index >= 0 && candles[actual_index].valid) {
            // Calculate position (rightmost candle is the newest)
            lv_coord_t x_pos = width - 10 - (i * candle_spacing) - (CANDLE_BODY_WIDTH / 2);
            
            // Draw even if extends past left edge (x_pos < 10) to show historical continuity
            if (x_pos >= -CANDLE_BODY_WIDTH) { // Only skip if completely off-screen
                drawSingleCandle(parent, candles[actual_index], x_pos, chart_top, chart_height, price_min, price_max);
                
                // Draw white line from highest point to top edge of chart
                if (actual_index == highest_index) {
                    lv_coord_t high_y = chart_top + (lv_coord_t)((price_max - actual_high) / (price_max - price_min) * chart_height);
                    if (high_y > chart_top) {
                        lv_obj_t* high_line = lv_obj_create(parent);
                        lv_obj_set_size(high_line, 1, high_y - chart_top);
                        lv_obj_set_pos(high_line, x_pos, chart_top);
                        lv_obj_set_style_bg_color(high_line, COLOR_WHITE_TEXT, 0);
                        lv_obj_set_style_bg_opa(high_line, LV_OPA_COVER, 0);
                        lv_obj_set_style_border_width(high_line, 0, 0);
                        lv_obj_set_style_pad_all(high_line, 0, 0);
                        lv_obj_set_style_radius(high_line, 0, 0);
                    }
                }
                
                // Draw white line from lowest point to bottom edge of chart
                if (actual_index == lowest_index) {
                    lv_coord_t low_y = chart_top + (lv_coord_t)((price_max - actual_low) / (price_max - price_min) * chart_height);
                    if (low_y < chart_top + chart_height) {
                        lv_obj_t* low_line = lv_obj_create(parent);
                        lv_obj_set_size(low_line, 1, (chart_top + chart_height) - low_y);
                        lv_obj_set_pos(low_line, x_pos, low_y);
                        lv_obj_set_style_bg_color(low_line, COLOR_WHITE_TEXT, 0);
                        lv_obj_set_style_bg_opa(low_line, LV_OPA_COVER, 0);
                        lv_obj_set_style_border_width(low_line, 0, 0);
                        lv_obj_set_style_pad_all(low_line, 0, 0);
                        lv_obj_set_style_radius(low_line, 0, 0);
                    }
                }
            }
        }
    }
    
    // Calculate moving average line using all available data for better accuracy
    int ma_period = 7; // 7-period moving average
    if (ma_period > count) ma_period = count; // Use all available data if less than period
    
    // Calculate MA points, but only for visible candles (to save memory)
    static lv_point_precise_t ma_points[35]; // Enough for max visible candles
    int ma_point_count = 0;
    
    // Calculate MA for visible + extended candles (from newest backwards)
    for (int display_index = 0; display_index < extended_candles && display_index < count && ma_point_count < 35; display_index++) {
        int actual_index = count - 1 - display_index; // Start from newest
        
        if (actual_index >= ma_period - 1) { // Only if we have enough data for MA
            // Calculate moving average for this point using all available history
            float ma_sum = 0.0f;
            int valid_count = 0;
            for (int j = actual_index - ma_period + 1; j <= actual_index; j++) {
                if (j >= 0 && candles[j].valid) {
                    ma_sum += (candles[j].open + candles[j].close) / 2.0f; // Use typical price
                    valid_count++;
                }
            }
            
            if (valid_count >= ma_period * 0.7f) { // Require at least 70% valid data
                float ma_price = ma_sum / valid_count;
                
                // Calculate position (matches the candle position)
                lv_coord_t x_pos = width - 10 - (display_index * candle_spacing) - (CANDLE_BODY_WIDTH / 2);
                lv_coord_t ma_y = chart_top + (lv_coord_t)((price_max - ma_price) / (price_max - price_min) * chart_height);
                
                // Include MA points for extended candles (can go past left edge)
                if (x_pos >= -CANDLE_BODY_WIDTH && x_pos < width - 10) { // Match extended candle boundary
                    ma_points[ma_point_count].x = x_pos;
                    ma_points[ma_point_count].y = ma_y;
                    ma_point_count++;
                }
            }
        }
    }
    
    // Draw smooth moving average line using LVGL line widget
    if (ma_point_count > 1) {
        // Create a single line object that can handle multiple points
        lv_obj_t* ma_line = lv_line_create(parent);
        
        // Set line points (LVGL line widget handles smooth drawing)
        lv_line_set_points(ma_line, ma_points, ma_point_count);
        
        // Style the line
        lv_obj_set_style_line_width(ma_line, 1, 0);
        lv_obj_set_style_line_color(ma_line, lv_color_hex(0xFFA500), 0); // Orange color
        lv_obj_set_style_line_opa(ma_line, LV_OPA_COVER, 0);
        
        // Position the line widget to cover the chart area
        lv_obj_set_size(ma_line, width, chart_height);
        lv_obj_set_pos(ma_line, 0, chart_top);
    }
    
    // Draw chart labels using actual data range (not padded chart range)
    drawChartLabels(candles, count, data_min, data_max, interval);
}

void DisplayManager::drawSingleCandle(lv_obj_t* parent, const CandlestickData& candle, 
                                     lv_coord_t x_pos, lv_coord_t chart_top, lv_coord_t chart_height, 
                                     float price_min, float price_max) {
    
    float price_range = price_max - price_min;
    if (price_range <= 0) return;
    
    // Calculate Y positions
    lv_coord_t high_y = chart_top + (lv_coord_t)((price_max - candle.high) / price_range * chart_height);
    lv_coord_t low_y = chart_top + (lv_coord_t)((price_max - candle.low) / price_range * chart_height);
    lv_coord_t open_y = chart_top + (lv_coord_t)((price_max - candle.open) / price_range * chart_height);
    lv_coord_t close_y = chart_top + (lv_coord_t)((price_max - candle.close) / price_range * chart_height);
    
    // Determine candle color
    bool is_bullish = candle.close >= candle.open;
    lv_color_t candle_color = is_bullish ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED;
    
    // Draw high-low line (wick) - 1px wide, same color as candle
    lv_obj_t* wick = lv_obj_create(parent);
    lv_obj_set_size(wick, CANDLE_WICK_WIDTH, low_y - high_y);
    lv_obj_set_pos(wick, x_pos - (CANDLE_WICK_WIDTH / 2), high_y);
    lv_obj_set_style_bg_color(wick, candle_color, 0);
    lv_obj_set_style_bg_opa(wick, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wick, 0, 0);
    lv_obj_set_style_pad_all(wick, 0, 0);
    lv_obj_set_style_radius(wick, 0, 0); // No rounded corners
    
    // White lines are now drawn only for highest/lowest points in drawCandlestickChart
    
    // Draw open-close body - 5px wide
    lv_coord_t body_top = (open_y < close_y) ? open_y : close_y;
    lv_coord_t body_height = abs(close_y - open_y);
    if (body_height < 1) body_height = 1; // Minimum height for doji candles
    
    lv_obj_t* body = lv_obj_create(parent);
    lv_obj_set_size(body, CANDLE_BODY_WIDTH, body_height);
    lv_obj_set_pos(body, x_pos - (CANDLE_BODY_WIDTH / 2), body_top);
    lv_obj_set_style_bg_color(body, candle_color, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 0, 0);
    lv_obj_set_style_radius(body, 0, 0); // No rounded corners - rectangular
}

void DisplayManager::drawChartLabels(const CandlestickData* candles, int count, float price_min, float price_max, const String& interval) {
    if (!chart_container_) return;
    
    // Max price label (top-right corner of chart area)
    lv_obj_t* max_label = lv_label_create(chart_container_);
    lv_label_set_text(max_label, formatPrice(price_max).c_str());
    lv_obj_align(max_label, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_set_style_text_color(max_label, COLOR_WHITE_TEXT, 0);
    
    // Min price label (bottom-right corner of chart area)
    lv_obj_t* min_label = lv_label_create(chart_container_);
    lv_label_set_text(min_label, formatPrice(price_min).c_str());
    lv_obj_align(min_label, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
    lv_obj_set_style_text_color(min_label, COLOR_WHITE_TEXT, 0);
    
    // Time interval label (bottom-left corner of chart area)
    if (!interval.isEmpty()) {
        // Create interval label first (will be behind the container)
        lv_obj_t* interval_label = lv_label_create(chart_container_);
        lv_label_set_text(interval_label, interval.c_str());
        lv_obj_align(interval_label, LV_ALIGN_BOTTOM_LEFT, 5, -5); // Original label position
        lv_obj_set_style_text_color(interval_label, COLOR_MUTED_GREY, 0);
        
        // Create clickable container on top - starts from screen edge
        lv_obj_t* interval_container = lv_obj_create(chart_container_);
        lv_obj_set_size(interval_container, 35, 28); // Rectangular 35x28px
        lv_obj_align(interval_container, LV_ALIGN_BOTTOM_LEFT, 0, 0); // Absolute bottom-left corner
        
        // Style the container - dark transparent background
        lv_obj_set_style_bg_color(interval_container, COLOR_DARK_BG, 0);
        lv_obj_set_style_bg_opa(interval_container, LV_OPA_60, 0); // Dark transparent
        lv_obj_set_style_radius(interval_container, 3, 0); // Slightly rounded corners
        lv_obj_set_style_pad_all(interval_container, 0, 0);
        lv_obj_clear_flag(interval_container, LV_OBJ_FLAG_SCROLLABLE);
    }
}


void DisplayManager::updateChartArea(const BinanceDataManager& crypto_manager) {
    if (current_screen_ != DETAIL_SCREEN || !chart_container_) {
        LOG_WARN("updateChartArea called but not in detail screen or no chart container");
        return;
    }
    
    LOG_DEBUG("Updating chart area with new candlestick data");
    
    // Hide interval selection if visible before clearing chart
    hideIntervalSelection();
    
    // Clear existing chart content
    lv_obj_clean(chart_container_);
    
    // Reset price indicator pointers since chart was cleaned
    price_indicator_line_ = nullptr;
    price_indicator_horizontal_line_ = nullptr;
    price_indicator_label_ = nullptr;
    price_indicator_show_time_ = 0;
    
    // Get candlestick data
    const CandlestickData* candles = crypto_manager.getCandlestickData();
    int candle_count = crypto_manager.getCandlestickCount();
    
    if (candle_count > 0) {
        LOG_DEBUG("Drawing " + String(candle_count) + " candlesticks in chart area");
        String interval = crypto_manager.getCurrentCandlestickInterval();
        drawCandlestickChart(candles, candle_count, 240, 320 - COIN_INFO_HEIGHT, interval); // Full width and height
    } else {
        // Show loading message
        lv_obj_t* loading_label = lv_label_create(chart_container_);
        lv_label_set_text(loading_label, "Loading chart data...");
        lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(loading_label, COLOR_GREY_TEXT, 0);
        lv_obj_set_style_text_font(loading_label, &jetbrains_mono_12, 0);
    }
    
    // Check if price indicator should be auto-hidden
    checkPriceIndicatorTimeout();
    
    // Force display refresh
    lv_refr_now(NULL);
    LOG_DEBUG("Chart area update completed");
}

void DisplayManager::updateDetailCoinInfo(const BinanceDataManager& crypto_manager) {
    if (current_screen_ != DETAIL_SCREEN || !coin_info_container_ || selected_coin_index_ < 0) {
        return;
    }
    
    const CoinData* coin_data = crypto_manager.getCoinData();
    int coin_count = crypto_manager.getCoinCount();
    
    if (selected_coin_index_ >= coin_count || !coin_data[selected_coin_index_].valid) {
        return;
    }
    
    const CoinData& coin = coin_data[selected_coin_index_];
    
    // Update background color based on 24h change
    lv_color_t bg_color = (coin.change_percent_24h >= 0) ? 
        COLOR_TWILIGHT_GREEN : COLOR_TWILIGHT_RED;
    lv_obj_set_style_bg_color(coin_info_container_, bg_color, 0);
    
    // Update all text labels (they're children of coin_info_container_)
    uint32_t child_count = lv_obj_get_child_count(coin_info_container_);
    if (child_count >= 5) { // We expect at least 5 labels
        // Parse trading pair
        String base_symbol, quote_symbol;
        parseTradingPair(coin.symbol, base_symbol, quote_symbol);
        
        // Update labels by index (order they were created)
        lv_label_set_text(lv_obj_get_child(coin_info_container_, 0), base_symbol.c_str()); // Base symbol
        lv_label_set_text(lv_obj_get_child(coin_info_container_, 1), quote_symbol.c_str()); // Quote symbol
        lv_label_set_text(lv_obj_get_child(coin_info_container_, 2), formatPrice(coin.price).c_str()); // Price
        
        // 24h change amount
        String change_text = (coin.change_24h >= 0 ? "" : "-") + formatPrice(abs(coin.change_24h));
        lv_label_set_text(lv_obj_get_child(coin_info_container_, 3), change_text.c_str());
        lv_obj_set_style_text_color(lv_obj_get_child(coin_info_container_, 3), 
                                   coin.change_24h >= 0 ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED, 0);
        
        // 24h change percentage
        String percent_text = (coin.change_percent_24h >= 0 ? "" : "-") + String(abs(coin.change_percent_24h), 2) + "%";
        lv_label_set_text(lv_obj_get_child(coin_info_container_, 4), percent_text.c_str());
        lv_obj_set_style_text_color(lv_obj_get_child(coin_info_container_, 4), 
                                   coin.change_percent_24h >= 0 ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED, 0);
    }
    
    // Check if price indicator should be auto-hidden
    checkPriceIndicatorTimeout();
}

void DisplayManager::showPriceIndicator(lv_coord_t x_pos, lv_coord_t y_pos, float price_min, float price_max) {
    if (!chart_container_) return;
    
    // Remove previous indicator if it exists
    if (price_indicator_line_ && lv_obj_is_valid(price_indicator_line_)) {
        lv_obj_del(price_indicator_line_);
    }
    price_indicator_line_ = nullptr;
    
    if (price_indicator_horizontal_line_ && lv_obj_is_valid(price_indicator_horizontal_line_)) {
        lv_obj_del(price_indicator_horizontal_line_);
    }
    price_indicator_horizontal_line_ = nullptr;
    
    if (price_indicator_label_ && lv_obj_is_valid(price_indicator_label_)) {
        lv_obj_del(price_indicator_label_);
    }
    price_indicator_label_ = nullptr;
    
    // Calculate the price at the clicked Y position
    lv_coord_t chart_height = lv_obj_get_height(chart_container_);
    float price_range = price_max - price_min;
    float clicked_price = price_max - ((float)(y_pos) / chart_height * price_range);
    
    // Calculate timestamp at the clicked X position
    String timestamp_text = calculateTimestampAtPosition(x_pos);
    
    // Create vertical blue line (crosshair vertical component) - 30px centered on click
    price_indicator_line_ = lv_obj_create(chart_container_);
    lv_obj_set_size(price_indicator_line_, 1, 30);
    lv_obj_set_pos(price_indicator_line_, x_pos, y_pos - 15); // Center vertically on click Y
    lv_obj_set_style_bg_color(price_indicator_line_, lv_color_hex(0x0080FF), 0); // Blue color
    lv_obj_set_style_bg_opa(price_indicator_line_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(price_indicator_line_, 0, 0);
    lv_obj_set_style_pad_all(price_indicator_line_, 0, 0);
    lv_obj_set_style_radius(price_indicator_line_, 0, 0);
    
    // Create horizontal blue line (crosshair horizontal component) - 30px centered on click
    price_indicator_horizontal_line_ = lv_obj_create(chart_container_);
    lv_obj_set_size(price_indicator_horizontal_line_, 30, 1);
    lv_obj_set_pos(price_indicator_horizontal_line_, x_pos - 15, y_pos); // Center horizontally on click X
    lv_obj_set_style_bg_color(price_indicator_horizontal_line_, lv_color_hex(0x0080FF), 0); // Blue color
    lv_obj_set_style_bg_opa(price_indicator_horizontal_line_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(price_indicator_horizontal_line_, 0, 0);
    lv_obj_set_style_pad_all(price_indicator_horizontal_line_, 0, 0);
    lv_obj_set_style_radius(price_indicator_horizontal_line_, 0, 0);
    
    // Set timer for auto-hide after 2 seconds
    price_indicator_show_time_ = millis();
    
    // Create timestamp and price label with dark background in top-left corner
    price_indicator_label_ = lv_obj_create(chart_container_);
    
    // Create timestamp label (top line)
    lv_obj_t* timestamp_label = lv_label_create(price_indicator_label_);
    lv_label_set_text(timestamp_label, timestamp_text.c_str());
    lv_obj_align(timestamp_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(timestamp_label, lv_color_hex(0x0080FF), 0); // Blue text
    lv_obj_set_style_text_font(timestamp_label, &jetbrains_mono_14, 0);
    
    // Create price label (bottom line)
    lv_obj_t* price_label = lv_label_create(price_indicator_label_);
    lv_label_set_text(price_label, formatPrice(clicked_price).c_str());
    lv_obj_align(price_label, LV_ALIGN_TOP_LEFT, 0, 16); // 16px below timestamp
    lv_obj_set_style_text_color(price_label, lv_color_hex(0x0080FF), 0); // Blue text
    lv_obj_set_style_text_font(price_label, &jetbrains_mono_14, 0);
    
    // Style the container with dark background (no border)
    lv_obj_set_style_bg_color(price_indicator_label_, lv_color_hex(0x000000), 0); // Black background
    lv_obj_set_style_bg_opa(price_indicator_label_, LV_OPA_80, 0); // 80% opacity
    lv_obj_set_style_border_width(price_indicator_label_, 0, 0); // No border
    lv_obj_set_style_radius(price_indicator_label_, 3, 0); // Small rounded corners
    lv_obj_set_style_pad_all(price_indicator_label_, 4, 0); // Small padding
    
    // Position in top-left corner
    lv_obj_set_pos(price_indicator_label_, 0, 1); // Touch left edge, 1px from top
    lv_obj_set_size(price_indicator_label_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
}

void DisplayManager::showIntervalSelection() {
    if (!chart_container_ || interval_overlay_) {
        LOG_WARN("Cannot show interval selection: no chart container or overlay already exists");
        return;
    }
    
    
    // Create overlay container covering the entire chart area
    interval_overlay_ = lv_obj_create(chart_container_);
    lv_obj_set_size(interval_overlay_, 240, lv_obj_get_height(chart_container_));
    lv_obj_set_pos(interval_overlay_, 0, 0);
    lv_obj_set_style_bg_color(interval_overlay_, COLOR_DARK_BG, 0);
    lv_obj_set_style_bg_opa(interval_overlay_, LV_OPA_80, 0); // Dark transparent background
    lv_obj_set_style_border_width(interval_overlay_, 0, 0);
    lv_obj_set_style_pad_all(interval_overlay_, 10, 0);
    lv_obj_clear_flag(interval_overlay_, LV_OBJ_FLAG_SCROLLABLE);
    
    // Calculate button dimensions
    int available_width = 240 - 20; // Container width minus padding
    int available_height = lv_obj_get_height(chart_container_) - 20; // Container height minus padding
    
    int button_width = (available_width - (INTERVAL_GRID_COLS - 1) * INTERVAL_BUTTON_SPACING) / INTERVAL_GRID_COLS;
    int grid_height = INTERVAL_GRID_ROWS * INTERVAL_BUTTON_HEIGHT + (INTERVAL_GRID_ROWS - 1) * INTERVAL_BUTTON_SPACING;
    int start_y = (available_height - grid_height) / 2; // Center vertically
    
    // Create interval buttons in 3x5 grid
    for (int i = 0; i < INTERVAL_COUNT; i++) {
        int row = i / INTERVAL_GRID_COLS;
        int col = i % INTERVAL_GRID_COLS;
        
        // Special handling for last row (2 buttons) - expand to fill width
        int btn_width = button_width;
        int btn_x = col * (button_width + INTERVAL_BUTTON_SPACING);
        
        if (row == INTERVAL_GRID_ROWS - 1) { // Last row
            btn_width = (available_width - INTERVAL_BUTTON_SPACING) / 2; // Two buttons sharing width
            btn_x = col * (btn_width + INTERVAL_BUTTON_SPACING);
        }
        
        int btn_y = start_y + row * (INTERVAL_BUTTON_HEIGHT + INTERVAL_BUTTON_SPACING);
        
        // Create button
        interval_buttons_[i] = lv_obj_create(interval_overlay_);
        lv_obj_set_size(interval_buttons_[i], btn_width, INTERVAL_BUTTON_HEIGHT);
        lv_obj_set_pos(interval_buttons_[i], btn_x, btn_y);
        
        // Style button - dark transparent background with subtle border
        lv_obj_set_style_bg_color(interval_buttons_[i], COLOR_DARK_BG, 0);
        lv_obj_set_style_bg_opa(interval_buttons_[i], LV_OPA_60, 0); // Dark transparent
        lv_obj_set_style_border_width(interval_buttons_[i], 1, 0); // Thin border
        lv_obj_set_style_border_color(interval_buttons_[i], COLOR_WHITE_TEXT, 0); // White border  
        lv_obj_set_style_border_opa(interval_buttons_[i], LV_OPA_20, 0); // Subtle transparent border (less than bg)
        lv_obj_set_style_radius(interval_buttons_[i], 4, 0);
        lv_obj_set_style_pad_all(interval_buttons_[i], 2, 0);
        lv_obj_clear_flag(interval_buttons_[i], LV_OBJ_FLAG_SCROLLABLE);
        
        // Add text label
        lv_obj_t* label = lv_label_create(interval_buttons_[i]);
        lv_label_set_text(label, SUPPORTED_INTERVALS[i]);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, COLOR_WHITE_TEXT, 0);
        lv_obj_set_style_text_font(label, &jetbrains_mono_14, 0);
        
        // Store interval index as user data for click handling
        lv_obj_set_user_data(interval_buttons_[i], (void*)(intptr_t)i);
    }
    
}

void DisplayManager::hideIntervalSelection() {
    if (!interval_overlay_) {
        return;
    }
    
    
    // Clean up overlay and buttons
    lv_obj_del(interval_overlay_);
    interval_overlay_ = nullptr;
    
    // Clear button references (they're deleted with overlay)
    for (int i = 0; i < INTERVAL_COUNT; i++) {
        interval_buttons_[i] = nullptr;
    }
}

bool DisplayManager::isIntervalSelectionVisible() const {
    return interval_overlay_ != nullptr;
}

String DisplayManager::calculateTimestampAtPosition(lv_coord_t x_pos) {
    if (!crypto_manager_ref_) {
        LOG_ERROR("calculateTimestampAtPosition: crypto_manager_ref_ is null");
        return "No Data";
    }
    
    const CandlestickData* candles = crypto_manager_ref_->getCandlestickData();
    int candle_count = crypto_manager_ref_->getCandlestickCount();
    String current_interval = crypto_manager_ref_->getCurrentCandlestickInterval();
    
    LOG_DEBUG("calculateTimestampAtPosition: candle_count=" + String(candle_count) + ", interval=" + current_interval);
    
    if (!candles || candle_count == 0) {
        LOG_ERROR("calculateTimestampAtPosition: No candlestick data available");
        return "Loading...";
    }
    
    // Calculate chart dimensions (matching drawCandlestickChart logic)
    lv_coord_t width = 240;
    lv_coord_t available_width = width - 20; // 220px available
    lv_coord_t min_candle_spacing = CANDLE_BODY_WIDTH + 2; // 7px minimum
    int max_visible_candles = available_width / min_candle_spacing; // ~31 candles max
    
    int visible_candles = (candle_count < max_visible_candles) ? candle_count : max_visible_candles;
    int extended_candles = visible_candles + 2; // Add 2 more candles extending past left edge
    lv_coord_t candle_spacing = available_width / visible_candles;
    
    // Determine which candle the click corresponds to
    // Candles are drawn from right to left, newest on right
    // x_pos=230 (width-10) = newest candle, x_pos=10 = oldest visible candle
    
    // Calculate which candle index the click corresponds to
    lv_coord_t right_edge = width - 10; // 230px
    lv_coord_t distance_from_right = right_edge - x_pos;
    int candle_index_from_right = distance_from_right / candle_spacing;
    
    // Clamp to valid range
    if (candle_index_from_right < 0) candle_index_from_right = 0;
    if (candle_index_from_right >= extended_candles) candle_index_from_right = extended_candles - 1;
    
    // Convert to actual candle array index (newest candles are at higher indices)
    int actual_candle_index = candle_count - 1 - candle_index_from_right;
    
    if (actual_candle_index < 0 || actual_candle_index >= candle_count) {
        LOG_ERROR("calculateTimestampAtPosition: Invalid candle index " + String(actual_candle_index) + " (count=" + String(candle_count) + ")");
        return "Invalid";
    }
    
    // Get timestamp and convert to readable format
    uint64_t timestamp_ms = candles[actual_candle_index].timestamp;
    uint64_t timestamp_s = timestamp_ms / 1000;
    
    LOG_DEBUG("calculateTimestampAtPosition: candle[" + String(actual_candle_index) + "] timestamp_ms=" + String((unsigned long)(timestamp_ms & 0xFFFFFFFF)) + ", timestamp_s=" + String((unsigned long)(timestamp_s & 0xFFFFFFFF)));
    
    // Convert Unix timestamp to date/time components
    // Simple implementation for UTC time
    uint64_t days = timestamp_s / 86400; // Seconds per day
    uint64_t remaining = timestamp_s % 86400;
    uint64_t hours = remaining / 3600;
    uint64_t minutes = (remaining % 3600) / 60;
    
    // Calculate date from days since Unix epoch (Jan 1, 1970)
    // Simple approximation - more accurate date calculation would be complex
    uint64_t year = 1970;
    uint64_t days_remaining = days;
    
    // Approximate years (accounting for leap years)
    while (days_remaining >= 365) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            if (days_remaining >= 366) {
                days_remaining -= 366;
                year++;
            } else {
                break;
            }
        } else {
            days_remaining -= 365;
            year++;
        }
    }
    
    // Approximate month and day (simplified)
    uint64_t month = 1;
    uint64_t day = days_remaining + 1;
    
    // Simple month calculation
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        days_in_month[1] = 29; // Leap year
    }
    
    for (int m = 0; m < 12; m++) {
        if (day <= days_in_month[m]) {
            month = m + 1;
            break;
        }
        day -= days_in_month[m];
        month++;
    }
    
    // Get current date for comparison using the newest candlestick timestamp
    // This gives us an approximation of "today" based on the latest market data
    uint64_t newest_timestamp_s = 0;
    if (candle_count > 0) {
        newest_timestamp_s = candles[candle_count - 1].timestamp / 1000;
    }
    
    uint64_t newest_days = newest_timestamp_s / 86400;
    uint64_t clicked_days = timestamp_s / 86400;
    bool is_today = (newest_days == clicked_days);
    
    LOG_DEBUG("Timestamp formatting: interval=" + current_interval + ", is_today=" + String(is_today ? "true" : "false"));

    // Format based on interval and whether it's today
    char timestamp_buffer[64];
    
    if (current_interval == "1m" || current_interval == "3m" || current_interval == "5m" || 
        current_interval == "15m" || current_interval == "30m" || current_interval == "1h" || current_interval == "2h") {
        // Short intervals: show only HH:MM if same day as today
        if (is_today) {
            snprintf(timestamp_buffer, sizeof(timestamp_buffer), "%02llu:%02llu", hours, minutes);
        } else {
            snprintf(timestamp_buffer, sizeof(timestamp_buffer), "%04llu-%02llu-%02llu %02llu:%02llu", 
                     year, month, day, hours, minutes);
        }
    } else if (current_interval == "4h" || current_interval == "6h" || current_interval == "8h") {
        // Medium intervals: always show full date and time
        snprintf(timestamp_buffer, sizeof(timestamp_buffer), "%04llu-%02llu-%02llu %02llu:%02llu", 
                 year, month, day, hours, minutes);
    } else {
        // Long intervals (12h, 1d, 1w, 1M): show date only without time
        snprintf(timestamp_buffer, sizeof(timestamp_buffer), "%04llu-%02llu-%02llu", 
                 year, month, day);
    }
    
    return String(timestamp_buffer);
}

void DisplayManager::hidePriceIndicator() {
    // Remove price indicator elements if they exist
    if (price_indicator_line_ && lv_obj_is_valid(price_indicator_line_)) {
        lv_obj_del(price_indicator_line_);
    }
    price_indicator_line_ = nullptr;
    
    if (price_indicator_horizontal_line_ && lv_obj_is_valid(price_indicator_horizontal_line_)) {
        lv_obj_del(price_indicator_horizontal_line_);
    }
    price_indicator_horizontal_line_ = nullptr;
    
    if (price_indicator_label_ && lv_obj_is_valid(price_indicator_label_)) {
        lv_obj_del(price_indicator_label_);
    }
    price_indicator_label_ = nullptr;
    
    // Reset show time
    price_indicator_show_time_ = 0;
    
    LOG_DEBUG("Price indicator hidden");
}

void DisplayManager::checkPriceIndicatorTimeout() {
    // Only check timeout if price indicator is currently shown
    if (price_indicator_show_time_ > 0 && price_indicator_line_ != nullptr) {
        unsigned long current_time = millis();
        if (current_time - price_indicator_show_time_ >= 2000) { // 2 seconds timeout
            hidePriceIndicator();
            LOG_DEBUG("Price indicator auto-hidden after timeout");
        }
    }
}

