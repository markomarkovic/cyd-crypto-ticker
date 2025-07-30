#include "CryptoDataManager.h"
#include <ArduinoJson.h>

CryptoDataManager::CryptoDataManager(NetworkManager& networkManager) 
    : network_manager_(networkManager), coin_count_(0), last_successful_update_(0) {
    // Initialize coin data array
    for (int i = 0; i < MAX_COINS; i++) {
        coin_data_[i].valid = false;
        coin_data_[i].last_update = 0;
    }
}

void CryptoDataManager::parseCoinIds(const char* coin_ids) {
    String ids_str = String(coin_ids);
    coin_count_ = 0;

    int start = 0;
    int end = ids_str.indexOf(',');

    while (end != -1 && coin_count_ < MAX_COINS) {
        coin_ids_array_[coin_count_] = ids_str.substring(start, end);
        coin_ids_array_[coin_count_].trim();
        coin_count_++;
        start = end + 1;
        end = ids_str.indexOf(',', start);
    }

    // Add the last ID
    if (start < ids_str.length() && coin_count_ < MAX_COINS) {
        coin_ids_array_[coin_count_] = ids_str.substring(start);
        coin_ids_array_[coin_count_].trim();
        coin_count_++;
    }

    Serial.printf("Parsed %d coin IDs: ", coin_count_);
    for (int i = 0; i < coin_count_; i++) {
        Serial.printf("%s ", coin_ids_array_[i].c_str());
    }
    Serial.println();
}

bool CryptoDataManager::fetchCoinData(const char* api_key) {
    String url_params = "?id=" + String(coin_ids_array_[0]);
    for (int i = 1; i < coin_count_; i++) {
        url_params += "," + coin_ids_array_[i];
    }
    
    String url = "https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest" + url_params;
    String response;
    int httpCode;
    
    Serial.println("Fetching crypto data...");
    
    if (network_manager_.httpGet(url, api_key, response, httpCode)) {
        Serial.println("API Response received");
        return parseJsonResponse(response);
    } else {
        Serial.printf("HTTP request failed: %d\n", httpCode);
        
        // Set appropriate error message based on HTTP code
        switch (httpCode) {
            // ESP32 HTTPClient specific error codes
            case -1:
                setError("Connection failed");
                break;
            case -2:
                setError("Send header failed");
                break;
            case -3:
                setError("Send payload failed");
                break;
            case -4:
                setError("No response");
                break;
            case -5:
                setError("No data");
                break;
            case -6:
                setError("No HTTP server");
                break;
            case -7:
                setError("Too less RAM");
                break;
            case -8:
                setError("Transfer encoding");
                break;
            case -9:
                setError("Decompress");
                break;
            case -10:
                setError("Stream write");
                break;
            case -11:
                setError("Read timeout");
                break;
            case -12:
                setError("HTTP header missing");
                break;
            
            // Standard HTTP status codes
            case 400:
                setError("Bad request");
                break;
            case 401:
                setError("API key invalid");
                break;
            case 403:
                setError("API access denied");
                break;
            case 404:
                setError("API endpoint not found");
                break;
            case 429:
                setError("Rate limit exceeded");
                break;
            case 500:
                setError("Server error");
                break;
            case 502:
                setError("Bad gateway");
                break;
            case 503:
                setError("Service unavailable");
                break;
            case 504:
                setError("Gateway timeout");
                break;
            
            default:
                if (httpCode < 0) {
                    setError("HTTP client error " + String(httpCode));
                } else {
                    setError("HTTP error " + String(httpCode));
                }
                break;
        }
        
        return false;
    }
}

const CoinData* CryptoDataManager::getCoinData() const {
    return coin_data_;
}

int CryptoDataManager::getCoinCount() const {
    return coin_count_;
}

bool CryptoDataManager::hasValidData() const {
    for (int i = 0; i < coin_count_; i++) {
        if (coin_data_[i].valid) {
            return true;
        }
    }
    return false;
}

bool CryptoDataManager::isDataStale() const {
    if (last_successful_update_ == 0) return false;
    return (millis() - last_successful_update_) > 300000; // 5 minutes
}

bool CryptoDataManager::hasError() const {
    return !last_error_message_.isEmpty();
}

String CryptoDataManager::getLastError() const {
    return last_error_message_;
}

unsigned long CryptoDataManager::getLastSuccessfulUpdate() const {
    return last_successful_update_;
}

String CryptoDataManager::formatPrice(float price) const {
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

void CryptoDataManager::clearError() {
    last_error_message_ = "";
}

void CryptoDataManager::setError(const String& error) {
    last_error_message_ = error;
}

bool CryptoDataManager::parseJsonResponse(const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.println("JSON parsing failed!");
        setError("JSON parsing failed");
        return false;
    }

    unsigned long now = millis();
    bool any_updated = false;

    // Parse all coin data in order
    for (int i = 0; i < coin_count_; i++) {
        String coin_id = coin_ids_array_[i];

        if (doc["data"][coin_id]) {
            coin_data_[i].id = coin_id;
            coin_data_[i].symbol = doc["data"][coin_id]["symbol"].as<String>();
            coin_data_[i].name = doc["data"][coin_id]["name"].as<String>();
            coin_data_[i].price = doc["data"][coin_id]["quote"]["USD"]["price"].as<float>();
            coin_data_[i].change_24h = doc["data"][coin_id]["quote"]["USD"]["percent_change_24h"].as<float>();
            coin_data_[i].change_1h = doc["data"][coin_id]["quote"]["USD"]["percent_change_1h"].as<float>();
            coin_data_[i].valid = true;
            coin_data_[i].last_update = now;
            any_updated = true;

            Serial.printf("%s: $%.2f (24h: %.2f%%, 1h: %.2f%%)\n",
                          coin_data_[i].symbol.c_str(),
                          coin_data_[i].price,
                          coin_data_[i].change_24h,
                          coin_data_[i].change_1h);
        } else {
            Serial.printf("No data found for coin ID: %s\n", coin_id.c_str());
            // Don't mark as invalid if we have previous data - keep the old data
            if (!coin_data_[i].valid) {
                coin_data_[i].valid = false;
            }
        }
    }

    if (any_updated) {
        last_successful_update_ = now;
        clearError();
    }

    return any_updated;
}