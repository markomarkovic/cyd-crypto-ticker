#ifndef CRYPTO_DATA_MANAGER_H
#define CRYPTO_DATA_MANAGER_H

#include <Arduino.h>
#include "NetworkManager.h"

struct CoinData {
    String id;
    String symbol;
    String name;
    float price;
    float change_24h;
    float change_1h;
    bool valid;
    unsigned long last_update;
};

class CryptoDataManager {
public:
    static const int MAX_COINS = 10;
    
    CryptoDataManager(NetworkManager& networkManager);
    ~CryptoDataManager() = default;
    
    void parseCoinIds(const char* coin_ids);
    bool fetchCoinData(const char* api_key);
    
    const CoinData* getCoinData() const;
    int getCoinCount() const;
    
    bool hasValidData() const;
    bool isDataStale() const;
    bool hasError() const;
    String getLastError() const;
    unsigned long getLastSuccessfulUpdate() const;
    
    String formatPrice(float price) const;
    
private:
    NetworkManager& network_manager_;
    CoinData coin_data_[MAX_COINS];
    String coin_ids_array_[MAX_COINS];
    int coin_count_;
    
    unsigned long last_successful_update_;
    String last_error_message_;
    
    void clearError();
    void setError(const String& error);
    bool parseJsonResponse(const String& payload);
};

#endif // CRYPTO_DATA_MANAGER_H