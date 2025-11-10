/**
 * @file ApplicationController.cpp
 * @brief Implementation of the main application controller
 * @author Claude Code Assistant
 * @date 2024
 * 
 * This file implements the ApplicationController class that orchestrates
 * the entire CYD Crypto Ticker application including initialization,
 * state management, WebSocket connections, and hardware control.
 */

#include "ApplicationController.h"
#include "constants.h"
#include "ScreenshotManager.h"
#include <lvgl.h>

ApplicationController::ApplicationController() 
    : network_manager_(nullptr),
      crypto_manager_(nullptr),
      display_manager_(nullptr),
      hardware_controller_(nullptr),
      state_manager_(nullptr),
      websocket_manager_(nullptr),
      last_auto_refresh_(0) {
}

ApplicationController::~ApplicationController() {
    delete network_manager_;
    delete crypto_manager_;
    delete display_manager_;
    delete hardware_controller_;
    delete state_manager_;
    delete websocket_manager_;
}

void ApplicationController::initialize() {
    Serial.begin(115200);
    
    initializeManagers();
    initializeHardware();
    
    // Try to connect to WiFi
    if (attemptWiFiConnection()) {
        performInitialSetup();
        state_manager_->setAppState(ApplicationStateManager::AppState::NORMAL_OPERATION);
        state_manager_->setWiFiState(ApplicationStateManager::WiFiState::CONNECTED);
    } else {
        startAPModeWithScan();
        state_manager_->setAppState(ApplicationStateManager::AppState::AP_MODE);
        state_manager_->setWiFiState(ApplicationStateManager::WiFiState::AP_MODE);
    }
    
}

void ApplicationController::update() {
    state_manager_->updateLVGLTick();
    
    // Display system statistics when INFO level logging is enabled
    displaySystemStats();
    
    switch (state_manager_->getAppState()) {
        case ApplicationStateManager::AppState::AP_MODE:
            handleAPMode();
            break;
        case ApplicationStateManager::AppState::NORMAL_OPERATION:
            handleNormalOperation();
            break;
        case ApplicationStateManager::AppState::WIFI_RECONNECTING:
            handleWiFiReconnection();
            break;
        default:
            break;
    }
}

void ApplicationController::initializeManagers() {
    state_manager_ = new ApplicationStateManager();
    network_manager_ = new NetworkManager();
    crypto_manager_ = new BinanceDataManager();
    display_manager_ = new DisplayManager();
    hardware_controller_ = new HardwareController(
        LED_RED_PIN,
        LED_GREEN_PIN,
        LED_BLUE_PIN,
        LIGHT_SENSOR_PIN
    );
    websocket_manager_ = new WebSocketManager();
}

void ApplicationController::initializeHardware() {
    hardware_controller_->initialize();
    display_manager_->initialize();
}

bool ApplicationController::attemptWiFiConnection() {
    // Check if reconfiguration was requested
    if (network_manager_->isReconfigurationRequested()) {
        LOG_DEBUG("Reconfiguration requested - forcing AP mode");
        return false;
    }
    
    String stored_ssid, stored_password;
    bool has_stored_config = network_manager_->loadStoredWiFiConfig(stored_ssid, stored_password);
    
    if (has_stored_config) {
        LOG_DEBUG("Found stored WiFi credentials. SSID: ");
        LOG_DEBUG(stored_ssid);
        
        display_manager_->showConnectingMessage("Connecting to WiFi:\n" + stored_ssid);
        return network_manager_->connect(stored_ssid.c_str(), stored_password.c_str(), 20000);
    }
    
    LOG_DEBUG("No stored WiFi credentials found");
    return false;
}

void ApplicationController::startAPModeWithScan() {
    display_manager_->showConnectingMessage("Scanning WiFi networks...");
    
    bool scan_success = network_manager_->scanWiFiNetworks();
    if (!scan_success) {
        LOG_DEBUG("WiFi scan failed, but continuing with AP mode");
    }
    
    LOG_DEBUG("Starting AP mode with pre-scanned networks");
    if (network_manager_->startAPMode()) {
        display_manager_->showAPModeScreen(network_manager_->getAPSSID());
    } else {
        display_manager_->showErrorMessage("Failed to start AP mode");
    }
}

void ApplicationController::performInitialSetup() {
    // Load symbols configuration and handle new configuration if available
    if (network_manager_->hasNewSymbolsConfig()) {
        String new_symbols = network_manager_->getNewSymbols();
        network_manager_->saveSymbolsConfig(new_symbols);
        network_manager_->clearNewSymbolsConfig();
        LOG_DEBUG("New symbols configuration saved");
    }

    // Load stored symbols configuration
    String symbols;
    if (!network_manager_->loadStoredSymbolsConfig(symbols)) {
        display_manager_->showErrorMessage("No symbols configuration found.\nPlease configure via web portal.");
        return;
    }

    // Parse symbols for crypto data
    crypto_manager_->parseSymbols(symbols.c_str());

    display_manager_->showConnectingMessage("Connecting to real-time data...");

    // Initialize WebSocket connection
    setupWebSocketConnection(symbols);

    showInitialSetupComplete();

    updateLEDStatus();
}

void ApplicationController::handleAPMode() {
    network_manager_->handleAPMode();
    
    // Check for new credentials from web portal
    if (network_manager_->hasNewCredentials()) {
        if (connectWithNewCredentials()) {
            performInitialSetup(); // This will also handle new API config
            state_manager_->setAppState(ApplicationStateManager::AppState::NORMAL_OPERATION);
            state_manager_->setWiFiState(ApplicationStateManager::WiFiState::CONNECTED);
        } else {
            // Failed to connect, restart AP mode
            LOG_DEBUG("Failed to connect with new credentials, restarting AP mode");
            network_manager_->startAPMode();
            display_manager_->showAPModeScreen(network_manager_->getAPSSID());
        }
    }
    
    // Handle button presses for AP mode (including factory reset)
    handleAPModeButtons();
}

void ApplicationController::handleAPModeButtons() {
    // Update custom adaptive brightness
    hardware_controller_->updateAdaptiveBrightness();

    // Check for button presses
    hardware_controller_->updateButtonStatus();

    // Check for short press (cancel configuration or screenshot)
    if (hardware_controller_->isShortPressDetected()) {
        unsigned long press_time = hardware_controller_->getButtonPressTime();

        // Clear the short press flag
        hardware_controller_->clearShortPressDetected();

        // Very short press (< 1 second) = screenshot
        // Longer short press (1-5 seconds) = cancel configuration
        if (press_time < 1000) {
            LOG_INFO("Screenshot requested via button press in AP mode");
            outputScreenshotToSerial(nullptr); // No WebSocket in AP mode
        } else {
            LOG_DEBUG("Short button press detected in AP mode - canceling configuration and restarting...");

            // Visual feedback - flash blue LED 2 times to indicate cancel
            for (int i = 0; i < 2; i++) {
                hardware_controller_->setLED(false, false, true); // Blue
                delay(150);
                hardware_controller_->setLED(false, false, false); // Off
                delay(150);
            }

            // Clear reconfiguration flag so it will try to connect with stored credentials
            network_manager_->clearReconfigurationFlag();

            LOG_DEBUG("Configuration canceled. Restarting...");
            delay(1000);
            ESP.restart();
        }
    }
    
    // Check if reconfiguration was requested (5+ second press detected)
    if (hardware_controller_->isReconfigurationRequested()) {
        // Get the current button press time to check if it's a factory reset (10+ seconds)
        unsigned long button_press_time = hardware_controller_->getButtonPressTime();
        
        if (button_press_time >= 10000) { // 10 seconds - factory reset
            LOG_DEBUG("Factory reset requested - clearing all stored data...");
            
            // Clear the button request to prevent repeated triggers
            hardware_controller_->clearReconfigurationRequest();
            
            // Visual feedback - flash green then red LEDs
            for (int i = 0; i < 3; i++) {
                hardware_controller_->setLED(false, true, false); // Green
                delay(200);
                hardware_controller_->setLED(false, false, false); // Off
                delay(200);
            }
            
            for (int i = 0; i < 3; i++) {
                hardware_controller_->setLED(true, false, false); // Red
                delay(200);
                hardware_controller_->setLED(false, false, false); // Off
                delay(200);
            }
            
            // Perform factory reset
            network_manager_->factoryReset();
            
            LOG_DEBUG("Factory reset complete. Restarting...");
            delay(1000);
            ESP.restart();
            
        } else if (button_press_time == 0) {
            // Button was released before 10 seconds - this was just a 5-9 second press
            LOG_DEBUG("Reconfiguration requested in AP mode - ignoring (already in config mode)");
            hardware_controller_->clearReconfigurationRequest();
        }
        // If button_press_time > 0 and < 10000, keep waiting for potential factory reset
    }
}

void ApplicationController::handleNormalOperation() {
    // Check WiFi connection status
    if (!network_manager_->isConnected()) {
        if (!state_manager_->isWiFiDisconnected()) {
            state_manager_->startWiFiDisconnection();
            LOG_DEBUG("WiFi disconnected - starting silent background reconnection");
            // Disconnect WebSocket when WiFi is lost
            if (websocket_manager_) {
                websocket_manager_->disconnect();
            }
            // Reset symbols display flag so they will be shown on next reconnection
            if (crypto_manager_) {
                crypto_manager_->resetSymbolsDisplay();
            }
        }
        state_manager_->setAppState(ApplicationStateManager::AppState::WIFI_RECONNECTING);
        return;
    } else {
        // WiFi is connected - reset disconnection state if it was set
        if (state_manager_->isWiFiDisconnected()) {
            state_manager_->resetWiFiDisconnection();
            LOG_DEBUG("WiFi connection restored");
            // Reconnect WebSocket when WiFi is restored
            String symbols;
            if (network_manager_->loadStoredSymbolsConfig(symbols)) {
                setupWebSocketConnection(symbols);
            }
        }
    }
    
    // Poll WebSocket for incoming messages and handle reconnection
    if (websocket_manager_) {
        websocket_manager_->poll();
        websocket_manager_->processReconnection();
    }

    updateLEDStatus();
    updateHardwareControls();

    // Check for screenshot request (short button press in normal mode)
    if (hardware_controller_->isShortPressDetected()) {
        LOG_INFO("Screenshot requested via button press");
        hardware_controller_->clearShortPressDetected();
        outputScreenshotToSerial(websocket_manager_);
    }

    handleTouchEvents();
    handleAutomaticChartRefresh();
}

void ApplicationController::handleWiFiReconnection() {
    // Try to reconnect silently every 10 seconds
    static unsigned long last_reconnect_attempt = 0;
    unsigned long now = millis();
    
    if (now - last_reconnect_attempt > RECONNECTION_RETRY_INTERVAL_MS) {
        last_reconnect_attempt = now;
        
        if (attemptSilentReconnection()) {
            LOG_DEBUG("WiFi reconnected successfully!");
            state_manager_->resetWiFiDisconnection();
            
            // Update display with current crypto data - now handled by WebSocket callback
            
            // Reconnect WebSocket after WiFi reconnection
            String symbols;
            if (network_manager_->loadStoredSymbolsConfig(symbols)) {
                setupWebSocketConnection(symbols);
            }
            
            state_manager_->setAppState(ApplicationStateManager::AppState::NORMAL_OPERATION);
            return;
        }
    }
    
    // After 1 minute of failed reconnection attempts, show user message
    if (!state_manager_->isReconnectionMessageShown() && 
        state_manager_->getWiFiDisconnectionDuration() > RECONNECTION_TIMEOUT_MS) {
        showReconnectionMessage();
    }
    
    updateHardwareControls();
}




void ApplicationController::updateLEDStatus() {
    StatusCalculator::CoinStatus coin_status = StatusCalculator::calculateCoinStatus(*crypto_manager_);
    
    // Update connection status LED based on WebSocket state
    if (websocket_manager_) {
        static bool was_connected = false;
        bool currently_connected = websocket_manager_->isConnected();
        
        if (!currently_connected && websocket_manager_->shouldReconnect()) {
            // WebSocket is disconnected and trying to reconnect
            hardware_controller_->setConnectionStatus(HardwareController::ConnectionStatus::RECONNECTING);
        } else if (!currently_connected) {
            // WebSocket is disconnected and not trying to reconnect
            hardware_controller_->setConnectionStatus(HardwareController::ConnectionStatus::DISCONNECTED);
        } else if (!was_connected && currently_connected) {
            // WebSocket just connected - trigger the CONNECTED state (3x green blinks)
            hardware_controller_->setConnectionStatus(HardwareController::ConnectionStatus::CONNECTED);
            LOG_DEBUG("WebSocket connection established - LED will show 3x green blinks then switch to normal operation");
        }
        // Note: The HardwareController automatically transitions from CONNECTED to NORMAL_OPERATION
        // after the 3 green blinks are complete (see HardwareController::updateConnectionStatusLED)
        
        was_connected = currently_connected;
    }
    
    hardware_controller_->updateLEDStatus(coin_status.coins_up, 
                                         coin_status.coins_down,
                                         crypto_manager_->hasError(), 
                                         false);  // No more stale data with real-time WebSocket
}

void ApplicationController::updateHardwareControls() {
    // Update custom adaptive brightness
    hardware_controller_->updateAdaptiveBrightness();
    
    // Check for button presses (config clear request)
    hardware_controller_->updateButtonStatus();
    
    if (hardware_controller_->isReconfigurationRequested()) {
        LOG_DEBUG("Reconfiguration requested - setting persistent flag...");
        
        // Set reconfiguration flag instead of directly clearing WiFi config
        network_manager_->setReconfigurationRequested(true);
        hardware_controller_->clearReconfigurationRequest();
        
        // Visual feedback - flash green LED 3 times
        for (int i = 0; i < 3; i++) {
            hardware_controller_->setLED(false, true, false); // Green
            delay(200);
            hardware_controller_->setLED(false, false, false); // Off
            delay(200);
        }
        
        LOG_DEBUG("Reconfiguration flag set. Restarting...");
        delay(1000);
        ESP.restart();
    }
}


bool ApplicationController::connectWithNewCredentials() {
    String new_ssid = network_manager_->getNewSSID();
    String new_password = network_manager_->getNewPassword();
    
    display_manager_->showConnectingMessage("Connecting to WiFi:\n" + new_ssid);
    
    // Stop AP mode and try to connect with new credentials
    network_manager_->stopAPMode();
    network_manager_->clearNewCredentials();
    
    bool connected = network_manager_->connect(new_ssid.c_str(), new_password.c_str(), 20000);
    
    if (connected) {
        LOG_DEBUG("Connected with new credentials!");
        // Save the successful WiFi credentials
        network_manager_->saveWiFiConfig(new_ssid, new_password);
        // Clear reconfiguration flag since we successfully configured WiFi
        network_manager_->clearReconfigurationFlag();
        return true;
    }
    
    return false;
}

bool ApplicationController::attemptSilentReconnection() {
    String stored_ssid, stored_password;
    if (network_manager_->loadStoredWiFiConfig(stored_ssid, stored_password)) {
        LOG_DEBUG("Attempting silent WiFi reconnection...");
        return network_manager_->connect(stored_ssid.c_str(), stored_password.c_str(), 
                                       RECONNECTION_ATTEMPT_TIMEOUT_MS);
    }
    return false;
}

void ApplicationController::showReconnectionMessage() {
    state_manager_->setReconnectionMessageShown(true);
    LOG_DEBUG("1 minute timeout reached - showing user reconnection message");
    
    String wifi_info = "WiFi connection lost\n";
    wifi_info += "Reconnecting in background...\n";
    wifi_info += "To reset WiFi: Hold BOOT button\n";
    wifi_info += "for 5 seconds until LED blinks";
    
    display_manager_->updateCryptoDisplay(*crypto_manager_, wifi_info, "", websocket_manager_ ? websocket_manager_->isConnected() : false);
}


void ApplicationController::showInitialSetupComplete() {
    display_manager_->updateCryptoDisplay(*crypto_manager_, "", "", websocket_manager_ ? websocket_manager_->isConnected() : false);
}

void ApplicationController::setupWebSocketConnection(const String& symbols) {
    if (!websocket_manager_ || !network_manager_->isConnected()) {
        return;
    }
    
    LOG_DEBUG("Setting up WebSocket connection to Binance...");
    
    // Parse symbols into array for WebSocket manager
    String symbols_array[MAX_COINS]; // Support up to MAX_COINS symbols
    int symbol_count = 0;
    String temp_symbols = symbols;
    temp_symbols.toUpperCase();
    
    // Split symbols by comma
    while (temp_symbols.length() > 0 && symbol_count < MAX_COINS) {
        int comma_pos = temp_symbols.indexOf(',');
        if (comma_pos == -1) {
            symbols_array[symbol_count] = temp_symbols;
            symbol_count++;
            break;
        } else {
            symbols_array[symbol_count] = temp_symbols.substring(0, comma_pos);
            temp_symbols = temp_symbols.substring(comma_pos + 1);
            symbol_count++;
        }
    }
    
    // Set up price update callback
    websocket_manager_->setPriceUpdateCallback([this](const String& symbol, float price, float change24h, float changePercent24h) {
        // Update crypto data in real-time
        crypto_manager_->updateCoinData(symbol, price, change24h, changePercent24h);
        
        // Update display based on current screen
        if (display_manager_->getScreenState() == LIST_SCREEN) {
            display_manager_->updateCryptoDisplay(*crypto_manager_, "", "", websocket_manager_ ? websocket_manager_->isConnected() : false);
        } else if (display_manager_->getScreenState() == DETAIL_SCREEN) {
            // Update coin info in detail view for real-time price updates
            display_manager_->updateDetailCoinInfo(*crypto_manager_);
        }
    });
    
    // Configure symbols and connect
    websocket_manager_->setSymbols(symbols_array, symbol_count);
    
    if (websocket_manager_->connect()) {
        LOG_DEBUG("WebSocket connected successfully");
    } else {
        LOG_DEBUG("Failed to connect to WebSocket");
        crypto_manager_->setError("WebSocket connection failed");
    }
}

void ApplicationController::handleTouchEvents() {
    // Get touch input from LVGL
    lv_indev_t* indev = lv_indev_get_next(NULL);
    if (indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        lv_indev_state_t state = lv_indev_get_state(indev);
        
        // Handle touch press and release events differently
        static bool touch_press_handled = false;
        
        if (state == LV_INDEV_STATE_PRESSED && !touch_press_handled) {
            LOG_DEBUG("Touch press detected at (" + String(point.x) + ", " + String(point.y) + ")");
            ScreenState current_screen_before = display_manager_->getScreenState();
            bool action_occurred = display_manager_->handleTouch(point.x, point.y, *crypto_manager_);
            ScreenState current_screen_after = display_manager_->getScreenState();
            
            if (action_occurred) {
                touch_press_handled = true; // Prevent handling same press
                static unsigned long last_chart_fetch = 0;
                
                // Check if screen changed or if we're still in detail screen (interval change)
                bool screen_changed = (current_screen_before != current_screen_after);
                bool interval_changed = (current_screen_before == DETAIL_SCREEN && 
                                       current_screen_after == DETAIL_SCREEN && action_occurred);
                
                if (screen_changed && current_screen_after == DETAIL_SCREEN) {
                    // Switching to detail screen - detail screen is already shown, now fetch data
                    unsigned long now = millis();
                    if (now - last_chart_fetch > 1000) { // Prevent multiple fetches within 1 second
                        LOG_INFO("Screen switched to detail view, fetching candlestick data...");
                        fetchCandlestickDataForSelectedCoin();
                        last_chart_fetch = now;
                    } else {
                        LOG_DEBUG("Ignoring duplicate chart fetch request (within 1 second)");
                    }
                } else if (interval_changed) {
                    // Interval was changed while in detail screen - immediate refresh
                    LOG_INFO("Interval changed, refreshing chart data immediately...");
                    fetchCandlestickDataForSelectedCoin();
                    last_chart_fetch = millis();
                    
                    // Reset automatic refresh timer to start fresh with new interval
                    resetAutomaticRefreshTimer();
                } else if (screen_changed && current_screen_after == LIST_SCREEN) {
                    // Switching back to list screen - update main display
                    LOG_INFO("Screen switched to list view");
                    String sync_status = websocket_manager_ && websocket_manager_->isConnected() ? 
                                       "Real-time updates active" : "Reconnecting...";
                    display_manager_->updateCryptoDisplay(*crypto_manager_, "", sync_status, 
                                                        websocket_manager_ ? websocket_manager_->isConnected() : false);
                }
            } else {
                LOG_DEBUG("Touch handled but no screen change");
            }
        } else if (state == LV_INDEV_STATE_RELEASED) {
            // Ignore spurious touch releases at (0, 0)
            if (!(point.x == 0 && point.y == 0)) {
                LOG_DEBUG("Touch released at (" + String(point.x) + ", " + String(point.y) + ")");
            }
            touch_press_handled = false; // Reset for next touch
        } else if (state == LV_INDEV_STATE_PRESSED && touch_press_handled) {
            LOG_DEBUG("Ignoring continued touch press (already handled screen transition)");
        }
    } else {
        // Check if touch device is still available
        static unsigned long last_touch_check = 0;
        if (millis() - last_touch_check > 5000) { // Check every 5 seconds
            if (!indev) {
                LOG_ERROR("Touch input device not found!");
            } else if (lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) {
                LOG_ERROR("Touch input device is not a pointer type!");
            }
            last_touch_check = millis();
        }
    }
}

/**
 * @brief Fetches candlestick data for the currently selected cryptocurrency
 * 
 * Called when user switches to detail view to load historical price data
 * for chart display. Implements duplicate request protection and proper
 * error handling with fallback to cached data.
 * 
 * @note Only fetches data if not already in progress (thread safety)
 * @note Fetches 40 candles to fill screen (~31 visible) plus moving average data
 * @note Updates chart area directly without regenerating entire detail screen
 */
void ApplicationController::fetchCandlestickDataForSelectedCoin() {
    int selected_coin = display_manager_->getSelectedCoinIndex();
    const CoinData* coin_data = crypto_manager_->getCoinData();
    int coin_count = crypto_manager_->getCoinCount();
    
    if (selected_coin < 0 || selected_coin >= coin_count || !coin_data[selected_coin].valid) {
        LOG_ERROR("Invalid selected coin index: " + String(selected_coin));
        return;
    }
    
    String symbol = coin_data[selected_coin].symbol;
    
    // Pause WebSocket to free SSL memory for HTTPS request
    if (websocket_manager_) {
        websocket_manager_->pauseForMemoryCleanup();
    }
    
    // Synchronously fetch candlestick data with WebSocket paused  
    // Screen fits ~31 candles, MA period is 7, so fetch 31 + 7 = 38 minimum
    // Round up to 40 for a small buffer
    String current_interval = crypto_manager_->getCurrentCandlestickInterval();
    bool fetch_success = crypto_manager_->fetchCandlestickDataSync(symbol, current_interval, 40, *network_manager_);
    
    // Resume WebSocket connection after fetch completes
    if (websocket_manager_) {
        websocket_manager_->resumeAfterMemoryCleanup();
    }
    
    if (fetch_success) {
        LOG_INFO("Sync candlestick data fetch completed for " + symbol + " - updating chart");
        // Update chart area immediately since data is now available
        if (display_manager_->getScreenState() == DETAIL_SCREEN) {
            display_manager_->updateChartArea(*crypto_manager_);
        }
    } else {
        LOG_ERROR("Failed to fetch candlestick data for " + symbol);
    }
}

unsigned long ApplicationController::getCurrentRefreshInterval() const {
    if (!crypto_manager_) {
        return 3600000UL; // Default to 1 hour if no manager
    }
    
    String current_interval = crypto_manager_->getCurrentCandlestickInterval();
    return crypto_manager_->getIntervalRefreshRate(current_interval);
}

void ApplicationController::handleAutomaticChartRefresh() {
    // Only refresh when in detail screen
    if (!display_manager_ || display_manager_->getScreenState() != DETAIL_SCREEN) {
        return;
    }
    
    unsigned long now = millis();
    unsigned long refresh_interval = getCurrentRefreshInterval();
    
    // Check if it's time for automatic refresh
    if (now - last_auto_refresh_ >= refresh_interval) {
        LOG_INFO("Automatic chart refresh triggered for interval: " + crypto_manager_->getCurrentCandlestickInterval());
        
        // Fetch fresh data without showing "loading" message (user's request)
        // This will just update the chart data silently
        fetchCandlestickDataForSelectedCoin();
        last_auto_refresh_ = now;
    }
}

void ApplicationController::resetAutomaticRefreshTimer() {
    last_auto_refresh_ = millis();
    LOG_DEBUG("Automatic refresh timer reset");
}

void ApplicationController::displaySystemStats() {
    static unsigned long last_stats_display = 0;
    static size_t last_free_heap = 0;
    static unsigned long last_millis = 0;
    
    unsigned long now = millis();
    
    // Display stats every 30 seconds when DEBUG level is enabled
    if (now - last_stats_display > 30000) {
        size_t free_heap = ESP.getFreeHeap();
        size_t min_free_heap = ESP.getMinFreeHeap();
        size_t heap_size = ESP.getHeapSize();
        size_t used_heap = heap_size - free_heap;
        
        // Calculate approximate CPU usage based on heap allocation changes
        unsigned long time_diff = now - last_millis;
        int heap_change = (int)(last_free_heap - free_heap);
        
        // Calculate memory usage percentage
        float memory_usage_percent = (float)used_heap / heap_size * 100.0f;
        
        LOG_DEBUGF("=== SYSTEM STATS ===");
        LOG_DEBUGF("Uptime: %lu seconds", now / 1000);
        LOG_DEBUGF("Free Heap: %u bytes (%.1f%% used)", free_heap, memory_usage_percent);
        LOG_DEBUGF("Min Free Heap: %u bytes", min_free_heap);
        LOG_DEBUGF("Heap Change: %+d bytes in %lu ms", heap_change, time_diff);
        
        if (websocket_manager_) {
            LOG_DEBUGF("WebSocket: %s", websocket_manager_->isConnected() ? "Connected" : "Disconnected");
        }
        
        if (crypto_manager_) {
            LOG_DEBUGF("Valid Coins: %d/%d", crypto_manager_->getValidCoinCount(), crypto_manager_->getCoinCount());
        }
        
        LOG_DEBUGF("===================");
        
        last_stats_display = now;
        last_free_heap = free_heap;
        last_millis = now;
    }
}

void ApplicationController::freeMaxMemoryForScreenshot() {
    LOG_INFO("Freeing maximum memory for screenshot...");
    LOG_INFOF("Free heap before: %u bytes", ESP.getFreeHeap());

    // Disconnect WebSocket completely
    if (websocket_manager_) {
        LOG_DEBUG("Disconnecting WebSocket");
        websocket_manager_->disconnect();
    }

    // Note: crypto_manager_ data is static arrays, can't free without realloc

    LOG_INFOF("Free heap after cleanup: %u bytes", ESP.getFreeHeap());
}

