#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include <Arduino.h>
#include "constants.h"
#include "NetworkManager.h"
#include "CryptoDataManager.h"
#include "DisplayManager.h"
#include "HardwareController.h"
#include "ApplicationStateManager.h"
#include "StatusCalculator.h"

class ApplicationController {
public:
    ApplicationController();
    ~ApplicationController();
    
    void initialize();
    void update();
    
private:
    // Initialization methods
    void initializeManagers();
    void initializeHardware();
    bool attemptWiFiConnection();
    void startAPModeWithScan();
    void performInitialSetup();
    
    // State handling methods
    void handleAPMode();
    void handleAPModeButtons();
    void handleNormalOperation();
    void handleWiFiReconnection();
    
    // Update methods
    void updatePeriodicTasks();
    void updateWiFiStatus();
    void updateCryptoData();
    void updateLEDStatus();
    void updateHardwareControls();
    void handlePullToRefresh();
    
    // WiFi connection methods
    bool connectWithNewCredentials();
    bool attemptSilentReconnection();
    void showReconnectionMessage();
    
    // Display update methods
    void updateMainDisplay(const String& sync_status);
    void showInitialSetupComplete();
    
    // Manager instances
    NetworkManager* network_manager_;
    CryptoDataManager* crypto_manager_;
    DisplayManager* display_manager_;
    HardwareController* hardware_controller_;
    ApplicationStateManager* state_manager_;
};

#endif // APPLICATION_CONTROLLER_H