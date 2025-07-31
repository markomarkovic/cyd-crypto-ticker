#include "StatusCalculator.h"

StatusCalculator::CoinStatus StatusCalculator::calculateCoinStatus(const BinanceDataManager& crypto_manager) {
    CoinStatus status;
    const CoinData* coin_data = crypto_manager.getCoinData();
    int coin_count = crypto_manager.getCoinCount();
    
    for (int i = 0; i < coin_count; i++) {
        if (coin_data[i].valid) {
            status.total_valid++;
            if (coin_data[i].change_percent_24h > 0) {
                status.coins_up++;
            } else if (coin_data[i].change_percent_24h < 0) {
                status.coins_down++;
            }
        }
    }
    
    return status;
}

