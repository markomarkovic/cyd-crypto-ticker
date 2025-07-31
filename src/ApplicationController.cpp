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
#include <lvgl.h>

ApplicationController::ApplicationController() 
    : network_manager_(nullptr),
      crypto_manager_(nullptr),
      display_manager_(nullptr),
      hardware_controller_(nullptr),
      state_manager_(nullptr),
      websocket_manager_(nullptr) {
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
        SAFE_SERIAL_PRINTLN("Reconfiguration requested - forcing AP mode");
        return false;
    }
    
    String stored_ssid, stored_password;
    bool has_stored_config = network_manager_->loadStoredWiFiConfig(stored_ssid, stored_password);
    
    if (has_stored_config) {
        SAFE_SERIAL_PRINT("Found stored WiFi credentials. SSID: ");
        SAFE_SERIAL_PRINTLN(stored_ssid);
        
        display_manager_->showConnectingMessage("Connecting to WiFi:\n" + stored_ssid);
        return network_manager_->connect(stored_ssid.c_str(), stored_password.c_str(), 20000);
    }
    
    SAFE_SERIAL_PRINTLN("No stored WiFi credentials found");
    return false;
}

void ApplicationController::startAPModeWithScan() {
    display_manager_->showConnectingMessage("Scanning WiFi networks...");
    
    bool scan_success = network_manager_->scanWiFiNetworks();
    if (!scan_success) {
        SAFE_SERIAL_PRINTLN("WiFi scan failed, but continuing with AP mode");
    }
    
    SAFE_SERIAL_PRINTLN("Starting AP mode with pre-scanned networks");
    if (network_manager_->startAPMode()) {
        display_manager_->showAPModeScreen(network_manager_->getAPSSID());
    } else {
        display_manager_->showErrorMessage("Failed to start AP mode");
    }
}

void ApplicationController::performInitialSetup() {
    SAFE_SERIAL_PRINTLN("");
    SAFE_SERIAL_PRINT("Connected to WiFi! IP address: ");
    SAFE_SERIAL_PRINTLN(network_manager_->getLocalIP());
    
    // Load symbols configuration and handle new configuration if available
    if (network_manager_->hasNewSymbolsConfig()) {
        String new_symbols = network_manager_->getNewSymbols();
        network_manager_->saveSymbolsConfig(new_symbols);
        network_manager_->clearNewSymbolsConfig();
        SAFE_SERIAL_PRINTLN("New symbols configuration saved");
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
            SAFE_SERIAL_PRINTLN("Failed to connect with new credentials, restarting AP mode");
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
    
    // Check if reconfiguration was requested (5+ second press detected)
    if (hardware_controller_->isReconfigurationRequested()) {
        // Get the current button press time to check if it's a factory reset (10+ seconds)
        unsigned long button_press_time = hardware_controller_->getButtonPressTime();
        
        if (button_press_time >= 10000) { // 10 seconds - factory reset
            SAFE_SERIAL_PRINTLN("Factory reset requested - clearing all stored data...");
            
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
            
            SAFE_SERIAL_PRINTLN("Factory reset complete. Restarting...");
            delay(1000);
            ESP.restart();
            
        } else if (button_press_time == 0) {
            // Button was released before 10 seconds - this was just a 5-9 second press
            SAFE_SERIAL_PRINTLN("Reconfiguration requested in AP mode - ignoring (already in config mode)");
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
            SAFE_SERIAL_PRINTLN("WiFi disconnected - starting silent background reconnection");
            // Disconnect WebSocket when WiFi is lost
            if (websocket_manager_) {
                websocket_manager_->disconnect();
            }
        }
        state_manager_->setAppState(ApplicationStateManager::AppState::WIFI_RECONNECTING);
        return;
    } else {
        // WiFi is connected - reset disconnection state if it was set
        if (state_manager_->isWiFiDisconnected()) {
            state_manager_->resetWiFiDisconnection();
            SAFE_SERIAL_PRINTLN("WiFi connection restored");
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
}

void ApplicationController::handleWiFiReconnection() {
    // Try to reconnect silently every 10 seconds
    static unsigned long last_reconnect_attempt = 0;
    unsigned long now = millis();
    
    if (now - last_reconnect_attempt > RECONNECTION_RETRY_INTERVAL_MS) {
        last_reconnect_attempt = now;
        
        if (attemptSilentReconnection()) {
            SAFE_SERIAL_PRINTLN("WiFi reconnected successfully!");
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
        SAFE_SERIAL_PRINTLN("Reconfiguration requested - setting persistent flag...");
        
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
        
        SAFE_SERIAL_PRINTLN("Reconfiguration flag set. Restarting...");
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
        SAFE_SERIAL_PRINTLN("Connected with new credentials!");
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
        SAFE_SERIAL_PRINTLN("Attempting silent WiFi reconnection...");
        return network_manager_->connect(stored_ssid.c_str(), stored_password.c_str(), 
                                       RECONNECTION_ATTEMPT_TIMEOUT_MS);
    }
    return false;
}

void ApplicationController::showReconnectionMessage() {
    state_manager_->setReconnectionMessageShown(true);
    SAFE_SERIAL_PRINTLN("1 minute timeout reached - showing user reconnection message");
    
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
    
    SAFE_SERIAL_PRINTLN("Setting up WebSocket connection to Binance...");
    
    // Parse symbols into array for WebSocket manager
    String symbols_array[10]; // Support up to 10 symbols
    int symbol_count = 0;
    String temp_symbols = symbols;
    temp_symbols.toUpperCase();
    
    // Split symbols by comma
    while (temp_symbols.length() > 0 && symbol_count < 10) {
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
        
        // Update display with new data
        display_manager_->updateCryptoDisplay(*crypto_manager_, "", "", websocket_manager_ ? websocket_manager_->isConnected() : false);
    });
    
    // Configure symbols and connect
    websocket_manager_->setSymbols(symbols_array, symbol_count);
    
    if (websocket_manager_->connect()) {
        SAFE_SERIAL_PRINTLN("WebSocket connected successfully");
    } else {
        SAFE_SERIAL_PRINTLN("Failed to connect to WebSocket");
        crypto_manager_->setError("WebSocket connection failed");
    }
}