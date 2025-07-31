#include "DisplayManager.h"
#include "constants.h"

DisplayManager::DisplayManager() 
    : status_label_(nullptr), wifi_info_label_(nullptr) {
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
        lv_obj_set_style_text_font(status_label_, LV_FONT_DEFAULT, 0);
    }
}


void DisplayManager::createCoinDisplay(const CoinData* coin_data, int coin_count, 
                                      const String& error_message, int& y_offset,
                                      bool is_websocket_connected) {
    SAFE_SERIAL_PRINTF("createCoinDisplay: Processing %d coins\n", coin_count);

    for (int i = 0; i < coin_count; i++) {
        SAFE_SERIAL_PRINTF("Coin %d: valid=%s, symbol=%s\n", i, coin_data[i].valid ? "true" : "false", coin_data[i].symbol.c_str());
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
            lv_obj_set_style_text_font(base_symbol_label, &lv_font_montserrat_12, 0); // Same as change indicators
            
            // Quote symbol (below base symbol, more muted)
            lv_obj_t* quote_symbol_label = lv_label_create(coin_container);
            lv_label_set_text(quote_symbol_label, quote_symbol.c_str()); // No slash
            lv_obj_align(quote_symbol_label, LV_ALIGN_LEFT_MID, 3, 9); // 9px below center (same as bottom change indicator)
            // Make it even more muted (75% mix with background)
            lv_color_t very_muted_color = lv_color_mix(COLOR_GREY_TEXT, COLOR_DARK_BG, 64); // 25% grey, 75% background
            lv_obj_set_style_text_color(quote_symbol_label, very_muted_color, 0);
            lv_obj_set_style_text_font(quote_symbol_label, &lv_font_montserrat_12, 0); // Same as change indicators

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
            lv_obj_set_style_text_font(change_24h_label, &lv_font_montserrat_12, 0);

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
            lv_obj_set_style_text_font(change_percent_label, &lv_font_montserrat_12, 0);

            // Price with larger font and styling (centered) - remove dollar sign as requested
            lv_obj_t* price_label = lv_label_create(coin_container);
            String price_text = formatPrice(coin_data[i].price);
            lv_label_set_text(price_label, price_text.c_str());
            lv_obj_align(price_label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(price_label, is_websocket_connected ? COLOR_WHITE_TEXT : COLOR_MUTED_WHITE, 0);
            lv_obj_set_style_text_font(price_label, &lv_font_montserrat_22, 0);
        }
    }

}

String DisplayManager::formatPrice(float price) const {
    // Format price with comma separators for better readability
    String price_str = String(price, 2);

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
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    
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
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_14, 0);
    
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
    lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_14, 0);
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

