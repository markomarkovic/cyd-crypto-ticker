#include "ApplicationController.h"
#include "constants.h"
#include <lvgl.h>

ApplicationController::ApplicationController() 
    : network_manager_(nullptr),
      crypto_manager_(nullptr),
      display_manager_(nullptr),
      hardware_controller_(nullptr),
      state_manager_(nullptr) {
}

ApplicationController::~ApplicationController() {
    delete network_manager_;
    delete crypto_manager_;
    delete display_manager_;
    delete hardware_controller_;
    delete state_manager_;
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
    
    // Initialize timing
    state_manager_->updateWiFiTimestamp();
    state_manager_->updateCryptoTimestamp();
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
    crypto_manager_ = new CryptoDataManager(*network_manager_);
    display_manager_ = new DisplayManager();
    hardware_controller_ = new HardwareController(
        LED_RED_PIN,
        LED_GREEN_PIN,
        LED_BLUE_PIN,
        LIGHT_SENSOR_PIN
    );
}

void ApplicationController::initializeHardware() {
    hardware_controller_->initialize();
    display_manager_->initialize();
}

bool ApplicationController::attemptWiFiConnection() {
    // Check if reconfiguration was requested
    if (network_manager_->isReconfigurationRequested()) {
        Serial.println("Reconfiguration requested - forcing AP mode");
        return false;
    }
    
    String stored_ssid, stored_password;
    bool has_stored_config = network_manager_->loadStoredWiFiConfig(stored_ssid, stored_password);
    
    if (has_stored_config) {
        Serial.print("Found stored WiFi credentials. SSID: ");
        Serial.println(stored_ssid);
        
        display_manager_->showConnectingMessage("Connecting to WiFi:\n" + stored_ssid);
        return network_manager_->connect(stored_ssid.c_str(), stored_password.c_str(), 20000);
    }
    
    Serial.println("No stored WiFi credentials found");
    return false;
}

void ApplicationController::startAPModeWithScan() {
    display_manager_->showConnectingMessage("Scanning WiFi networks...");
    
    bool scan_success = network_manager_->scanWiFiNetworks();
    if (!scan_success) {
        Serial.println("WiFi scan failed, but continuing with AP mode");
    }
    
    Serial.println("Starting AP mode with pre-scanned networks");
    if (network_manager_->startAPMode()) {
        display_manager_->showAPModeScreen(network_manager_->getAPSSID());
    } else {
        display_manager_->showErrorMessage("Failed to start AP mode");
    }
}

void ApplicationController::performInitialSetup() {
    Serial.println();
    Serial.print("Connected to WiFi! IP address: ");
    Serial.println(network_manager_->getLocalIP());
    
    // Load API configuration and handle new configuration if available
    if (network_manager_->hasNewAPIConfig()) {
        String new_api_key = network_manager_->getNewAPIKey();
        String new_coin_ids = network_manager_->getNewCoinIds();
        network_manager_->saveAPIConfig(new_api_key, new_coin_ids);
        network_manager_->clearNewAPIConfig();
        Serial.println("New API configuration saved");
    }
    
    // Load stored API configuration
    String api_key, coin_ids;
    if (!network_manager_->loadStoredAPIConfig(api_key, coin_ids)) {
        display_manager_->showErrorMessage("No API configuration found.\nPlease configure via web portal.");
        return;
    }
    
    // Parse coin IDs and fetch initial crypto data
    crypto_manager_->parseCoinIds(coin_ids.c_str());
    
    display_manager_->showConnectingMessage("Fetching crypto data...");
    
    if (crypto_manager_->fetchCoinData(api_key.c_str())) {
        showInitialSetupComplete();
    } else {
        display_manager_->showErrorMessage("Failed to fetch crypto data");
    }
    
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
            Serial.println("Failed to connect with new credentials, restarting AP mode");
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
            Serial.println("Factory reset requested - clearing all stored data...");
            
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
            
            Serial.println("Factory reset complete. Restarting...");
            delay(1000);
            ESP.restart();
            
        } else if (button_press_time == 0) {
            // Button was released before 10 seconds - this was just a 5-9 second press
            Serial.println("Reconfiguration requested in AP mode - ignoring (already in config mode)");
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
            Serial.println("WiFi disconnected - starting silent background reconnection");
        }
        state_manager_->setAppState(ApplicationStateManager::AppState::WIFI_RECONNECTING);
        return;
    } else {
        // WiFi is connected - reset disconnection state if it was set
        if (state_manager_->isWiFiDisconnected()) {
            state_manager_->resetWiFiDisconnection();
            Serial.println("WiFi connection restored");
        }
    }
    
    updatePeriodicTasks();
    handlePullToRefresh();
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
            Serial.println("WiFi reconnected successfully!");
            state_manager_->resetWiFiDisconnection();
            
            // Update display with current crypto data
            String sync_status = "Reconnected to:\n" + network_manager_->getCurrentSSID();
            updateMainDisplay(sync_status);
            
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

void ApplicationController::updatePeriodicTasks() {
    // Update WiFi status every 10 seconds
    if (network_manager_->isConnected() && 
        state_manager_->shouldUpdateWiFi(WIFI_UPDATE_INTERVAL)) {
        updateWiFiStatus();
    }
    
    // Update crypto data every 60 seconds
    if (network_manager_->isConnected() && 
        state_manager_->shouldUpdateCrypto(CRYPTO_UPDATE_INTERVAL)) {
        updateCryptoData();
    }
}

void ApplicationController::updateWiFiStatus() {
    String sync_status = StatusCalculator::createSyncStatusString(*crypto_manager_, millis());
    updateMainDisplay(sync_status);
    state_manager_->updateWiFiTimestamp();
}

void ApplicationController::updateCryptoData() {
    String api_key, coin_ids;
    if (!network_manager_->loadStoredAPIConfig(api_key, coin_ids)) {
        Serial.println("No API configuration found - skipping crypto data update");
        return;
    }
    
    bool fetch_success = crypto_manager_->fetchCoinData(api_key.c_str());
    String sync_status = fetch_success ? "Sync successful" : "Sync failed";
    updateMainDisplay(sync_status);
    state_manager_->updateCryptoTimestamp();
}

void ApplicationController::updateLEDStatus() {
    StatusCalculator::CoinStatus coin_status = StatusCalculator::calculateCoinStatus(*crypto_manager_);
    
    hardware_controller_->updateLEDStatus(coin_status.coins_up, 
                                         coin_status.coins_down,
                                         crypto_manager_->hasError(), 
                                         crypto_manager_->isDataStale());
}

void ApplicationController::updateHardwareControls() {
    // Update custom adaptive brightness
    hardware_controller_->updateAdaptiveBrightness();
    
    // Check for button presses (config clear request)
    hardware_controller_->updateButtonStatus();
    
    if (hardware_controller_->isReconfigurationRequested()) {
        Serial.println("Reconfiguration requested - setting persistent flag...");
        
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
        
        Serial.println("Reconfiguration flag set. Restarting...");
        delay(1000);
        ESP.restart();
    }
}

void ApplicationController::handlePullToRefresh() {
    if (display_manager_->checkPullToRefresh()) {
        // Pull-to-refresh was triggered, fetch new data
        String api_key, coin_ids;
        if (!network_manager_->loadStoredAPIConfig(api_key, coin_ids)) {
            Serial.println("No API configuration found for pull-to-refresh");
            return;
        }
        
        bool fetch_success = crypto_manager_->fetchCoinData(api_key.c_str());
        String sync_status = fetch_success ? "Refresh successful" : "Refresh failed";
        updateMainDisplay(sync_status);
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
        Serial.println("Connected with new credentials!");
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
        Serial.println("Attempting silent WiFi reconnection...");
        return network_manager_->connect(stored_ssid.c_str(), stored_password.c_str(), 
                                       RECONNECTION_ATTEMPT_TIMEOUT_MS);
    }
    return false;
}

void ApplicationController::showReconnectionMessage() {
    state_manager_->setReconnectionMessageShown(true);
    Serial.println("1 minute timeout reached - showing user reconnection message");
    
    String wifi_info = "WiFi connection lost\n";
    wifi_info += "Reconnecting in background...\n";
    wifi_info += "To reset WiFi: Hold BOOT button\n";
    wifi_info += "for 5 seconds until LED blinks";
    
    String sync_status = "Hold BOOT 5s to clear WiFi config";
    display_manager_->updateCryptoDisplay(*crypto_manager_, wifi_info, sync_status);
}

void ApplicationController::updateMainDisplay(const String& sync_status) {
    String wifi_info = StatusCalculator::createWifiInfoString(*network_manager_);
    display_manager_->updateCryptoDisplay(*crypto_manager_, wifi_info, sync_status);
}

void ApplicationController::showInitialSetupComplete() {
    String wifi_info = StatusCalculator::createWifiInfoString(*network_manager_);
    String sync_status = "Initial sync complete";
    display_manager_->updateCryptoDisplay(*crypto_manager_, wifi_info, sync_status);
}