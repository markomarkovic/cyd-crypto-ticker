#ifndef STATUS_CALCULATOR_H
#define STATUS_CALCULATOR_H

#include <Arduino.h>
#include "CryptoDataManager.h"
#include "NetworkManager.h"

class StatusCalculator {
public:
    struct CoinStatus {
        int coins_up;
        int coins_down;
        int total_valid;
        
        CoinStatus() : coins_up(0), coins_down(0), total_valid(0) {}
    };
    
    // Calculate coin performance statistics
    static CoinStatus calculateCoinStatus(const CryptoDataManager& crypto_manager);
    
    // Create formatted WiFi information string
    static String createWifiInfoString(const NetworkManager& network_manager);
    
    // Create formatted sync status string
    static String createSyncStatusString(const CryptoDataManager& crypto_manager, 
                                        unsigned long current_time);
    
    // Create complete WiFi info with sync status
    static String createCompleteWifiInfo(const NetworkManager& network_manager,
                                        const CryptoDataManager& crypto_manager,
                                        unsigned long current_time);

private:
    StatusCalculator() = delete; // Static utility class
};

#endif // STATUS_CALCULATOR_H