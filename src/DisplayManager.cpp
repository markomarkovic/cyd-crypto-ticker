#include "DisplayManager.h"
#include "constants.h"

DisplayManager::DisplayManager() 
    : status_label_(nullptr), wifi_info_label_(nullptr), refresh_label_(nullptr), is_refreshing_(false) {
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

    // Enable scrolling with invisible scrollbar
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Make scrollbar transparent by default
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    // Make scrollbar visible only when scrolling
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x808080), LV_PART_SCROLLBAR | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(screen, LV_OPA_50, LV_PART_SCROLLBAR | LV_STATE_PRESSED);
}

void DisplayManager::updateCryptoDisplay(const CryptoDataManager& crypto_manager, 
                                        const String& wifi_info, 
                                        const String& sync_status) {
    // Clear existing children
    lv_obj_clean(lv_screen_active());

    const CoinData* coin_data = crypto_manager.getCoinData();
    int coin_count = crypto_manager.getCoinCount();
    bool has_stale_data = crypto_manager.isDataStale();
    String error_message = crypto_manager.getLastError();
    int y_offset = 0; // Start position for crypto data (no top spacing)

    // Show error indicator at top if there's an error but we have old data
    if (!error_message.isEmpty() && crypto_manager.getLastSuccessfulUpdate() > 0) {
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

    createCoinDisplay(coin_data, coin_count, has_stale_data, error_message, y_offset);
    
    // Create WiFi info label after all crypto cards
    createWifiInfoLabel(y_offset, wifi_info, sync_status);
    
    if (!crypto_manager.hasValidData()) {
        showErrorMessage(error_message.isEmpty() ? "No crypto data available" : error_message);
    }
    
    // Force immediate display refresh
    lv_refr_now(NULL);
    lv_timer_handler();
}

bool DisplayManager::checkPullToRefresh() {
    lv_obj_t* screen = lv_screen_active();
    lv_coord_t scroll_y = lv_obj_get_scroll_y(screen);

    // Show "pull to refresh" message when pulling down (but not too deep yet)
    if (scroll_y < -10 && scroll_y > -40 && !is_refreshing_ && !refresh_label_) {
        // Create label as child of display (not screen) to make it truly fixed
        lv_display_t* display = lv_display_get_default();
        lv_obj_t* display_layer = lv_display_get_layer_top(display);
        refresh_label_ = lv_label_create(display_layer);
        lv_label_set_text(refresh_label_, "Pull to refresh...");
        lv_obj_set_pos(refresh_label_, 0, 5); // Fixed position at top
        lv_obj_set_width(refresh_label_, 240);
        lv_obj_set_style_text_align(refresh_label_, LV_TEXT_ALIGN_CENTER, 0);
        
        lv_obj_set_style_text_color(refresh_label_, COLOR_WHITE_TEXT, 0);
        lv_obj_set_style_bg_color(refresh_label_, COLOR_DARK_BG, 0);
        lv_obj_set_style_bg_opa(refresh_label_, LV_OPA_90, 0);
        lv_obj_set_style_pad_all(refresh_label_, 5, 0);
        Serial.println("Showing pull-to-refresh hint");
    }

    // Update message when pulling deeper
    if (scroll_y < -40 && !is_refreshing_ && refresh_label_) {
        lv_label_set_text(refresh_label_, "Keep pulling...");
        Serial.println("Keep pulling to trigger refresh");
    }

    // Trigger refresh when pulling deep enough
    if (scroll_y < -50 && !is_refreshing_) {
        is_refreshing_ = true;

        if (refresh_label_) {
            lv_label_set_text(refresh_label_, "Refreshing...");
        }

        Serial.println("Pull-to-refresh triggered!");

        // Force UI update to show refresh message
        lv_timer_handler();

        // Reset scroll position
        lv_obj_scroll_to_y(screen, 0, LV_ANIM_ON);

        // Hide refresh label after refresh is complete
        if (refresh_label_) {
            lv_obj_del(refresh_label_);
            refresh_label_ = nullptr;
        }

        // Reset refresh state
        is_refreshing_ = false;
        
        return true; // Indicates refresh was triggered
    }

    // Hide message when scrolling back up
    if (scroll_y >= -5 && refresh_label_ && !is_refreshing_) {
        lv_obj_del(refresh_label_);
        refresh_label_ = nullptr;
        Serial.println("Hiding pull-to-refresh hint");
    }
    
    return false; // No refresh triggered
}

void DisplayManager::showConnectingMessage(const String& message) {
    // Clear existing display content first to avoid conflicts
    lv_obj_clean(lv_screen_active());
    
    // Reset label pointer since we cleared the screen
    status_label_ = nullptr;
    wifi_info_label_ = nullptr;
    refresh_label_ = nullptr;
    
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
    refresh_label_ = nullptr;
    
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

void DisplayManager::createWifiInfoLabel(int y_offset, const String& wifi_info, const String& sync_status) {
    wifi_info_label_ = lv_label_create(lv_screen_active());
    lv_obj_set_width(wifi_info_label_, LV_SIZE_CONTENT);
    lv_obj_set_height(wifi_info_label_, LV_SIZE_CONTENT);
    lv_obj_align(wifi_info_label_, LV_ALIGN_TOP_MID, 0, y_offset + 10);
    
    lv_obj_set_style_text_color(wifi_info_label_, COLOR_GREY_TEXT, 0);
    lv_obj_set_style_text_font(wifi_info_label_, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_align(wifi_info_label_, LV_TEXT_ALIGN_CENTER, 0);
    
    // Combine wifi info and sync status
    String combined_info = wifi_info;
    if (!sync_status.isEmpty()) {
        combined_info += "\n" + sync_status;
    }
    lv_label_set_text(wifi_info_label_, combined_info.c_str());
}

void DisplayManager::createCoinDisplay(const CoinData* coin_data, int coin_count, 
                                      bool has_stale_data, const String& error_message, int& y_offset) {
    unsigned long now = millis();

    for (int i = 0; i < coin_count; i++) {
        if (coin_data[i].valid) {
            // Create container for this coin
            lv_obj_t* coin_container = lv_obj_create(lv_screen_active());
            int container_height = (i < 3) ? 55 : 50; // First 3 items taller
            lv_obj_set_size(coin_container, 240, container_height);
            lv_obj_set_pos(coin_container, 0, y_offset);

            // Set background color based on 1h price change
            lv_color_t bg_color = (coin_data[i].change_1h >= 0) ? 
                COLOR_TWILIGHT_GREEN : COLOR_TWILIGHT_RED;
            lv_obj_set_style_bg_color(coin_container, bg_color, 0);
            lv_obj_set_style_bg_opa(coin_container, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(coin_container, 0, 0);
            lv_obj_set_style_radius(coin_container, 0, 0);
            lv_obj_set_style_pad_all(coin_container, 8, 0);

            // Add bottom separator line (except for last coin)
            if (i < coin_count - 1) {
                lv_obj_t* separator = lv_obj_create(lv_screen_active());
                lv_obj_set_size(separator, 240, 1);
                lv_obj_set_pos(separator, 0, y_offset + container_height);
                lv_obj_set_style_bg_color(separator, COLOR_GREY_TEXT, 0);
                lv_obj_set_style_bg_opa(separator, LV_OPA_30, 0);
                lv_obj_set_style_border_width(separator, 0, 0);
                lv_obj_set_style_radius(separator, 0, 0);
            }

            // Coin symbol (left side, vertically centered)
            lv_obj_t* symbol_label = lv_label_create(coin_container);

            // Add exclamation mark if data is stale
            String symbol_text = coin_data[i].symbol;
            if (has_stale_data || (coin_data[i].last_update > 0 && (now - coin_data[i].last_update > 300000))) {
                symbol_text += " !";
            }

            lv_label_set_text(symbol_label, symbol_text.c_str());
            lv_obj_align(symbol_label, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_set_style_text_color(symbol_label, COLOR_GREY_TEXT, 0);
            lv_obj_set_style_text_font(symbol_label, LV_FONT_DEFAULT, 0);

            // 24h change (top right) - fixed position from center
            lv_obj_t* change_24h_label = lv_label_create(coin_container);
            String change_24h_text = (coin_data[i].change_24h >= 0 ? "+" : "") + String(coin_data[i].change_24h, 2) + "%";
            lv_label_set_text(change_24h_label, change_24h_text.c_str());
            lv_obj_align(change_24h_label, LV_ALIGN_RIGHT_MID, 0, -9); // 9px above center

            lv_color_t change_24h_color = (coin_data[i].change_24h >= 0) ? 
                COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED;
            lv_obj_set_style_text_color(change_24h_label, change_24h_color, 0);
            lv_obj_set_style_text_font(change_24h_label, &lv_font_montserrat_12, 0);

            // 1h change (bottom right) - fixed position from center
            lv_obj_t* change_1h_label = lv_label_create(coin_container);
            String change_1h_text = (coin_data[i].change_1h >= 0 ? "+" : "") + String(coin_data[i].change_1h, 2) + "%";
            lv_label_set_text(change_1h_label, change_1h_text.c_str());
            lv_obj_align(change_1h_label, LV_ALIGN_RIGHT_MID, 0, 9); // 9px below center

            lv_color_t change_1h_color = (coin_data[i].change_1h >= 0) ? 
                COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED;
            lv_obj_set_style_text_color(change_1h_label, change_1h_color, 0);
            lv_obj_set_style_text_font(change_1h_label, &lv_font_montserrat_12, 0);

            // Price with larger font and styling (centered) - remove dollar sign as requested
            lv_obj_t* price_label = lv_label_create(coin_container);
            String price_text = formatPrice(coin_data[i].price);
            lv_label_set_text(price_label, price_text.c_str());
            lv_obj_align(price_label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(price_label, COLOR_WHITE_TEXT, 0);
            lv_obj_set_style_text_font(price_label, &lv_font_montserrat_22, 0);

            y_offset += container_height + 1; // Move down for next coin (height + 1px separator)
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

