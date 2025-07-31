#ifndef STATUS_CALCULATOR_H
#define STATUS_CALCULATOR_H

#include <Arduino.h>
#include "BinanceDataManager.h"

class StatusCalculator {
public:
    struct CoinStatus {
        int coins_up;
        int coins_down;
        int total_valid;
        
        CoinStatus() : coins_up(0), coins_down(0), total_valid(0) {}
    };
    
    // Calculate coin performance statistics
    static CoinStatus calculateCoinStatus(const BinanceDataManager& crypto_manager);
    

private:
    StatusCalculator() = delete; // Static utility class
};

#endif // STATUS_CALCULATOR_H