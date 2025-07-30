#include "StatusCalculator.h"

StatusCalculator::CoinStatus StatusCalculator::calculateCoinStatus(const CryptoDataManager& crypto_manager) {
    CoinStatus status;
    const CoinData* coin_data = crypto_manager.getCoinData();
    int coin_count = crypto_manager.getCoinCount();
    
    for (int i = 0; i < coin_count; i++) {
        if (coin_data[i].valid) {
            status.total_valid++;
            if (coin_data[i].change_1h > 0) {
                status.coins_up++;
            } else if (coin_data[i].change_1h < 0) {
                status.coins_down++;
            }
        }
    }
    
    return status;
}

String StatusCalculator::createWifiInfoString(const NetworkManager& network_manager) {
    String wifi_info = network_manager.getCurrentSSID() + "\n";
    wifi_info += network_manager.getSignalStrength() + "\n";
    wifi_info += network_manager.getLocalIP();
    return wifi_info;
}

String StatusCalculator::createSyncStatusString(const CryptoDataManager& crypto_manager, 
                                               unsigned long current_time) {
    String sync_status;
    
    if (crypto_manager.getLastSuccessfulUpdate() > 0) {
        unsigned long sync_age = (current_time - crypto_manager.getLastSuccessfulUpdate()) / 1000;
        sync_status = "Last sync: ";
        
        if (sync_age < 60) {
            sync_status += String(sync_age) + "s ago";
        } else if (sync_age < 3600) {
            sync_status += String(sync_age / 60) + "m ago";
        } else {
            sync_status += String(sync_age / 3600) + "h ago";
        }
        
        sync_status += crypto_manager.hasError() ? " ERR" : " OK";
        if (crypto_manager.hasError()) {
            sync_status += "\n" + crypto_manager.getLastError();
        }
    } else {
        sync_status = "No sync yet";
    }
    
    return sync_status;
}

String StatusCalculator::createCompleteWifiInfo(const NetworkManager& network_manager,
                                               const CryptoDataManager& crypto_manager,
                                               unsigned long current_time) {
    String wifi_info = createWifiInfoString(network_manager) + "\n";
    wifi_info += createSyncStatusString(crypto_manager, current_time);
    return wifi_info;
}