#include "BinanceDataManager.h"
#include "constants.h"

BinanceDataManager::BinanceDataManager() 
    : coin_count_(0) {
    // Initialize coin data array
    for (int i = 0; i < MAX_COINS; i++) {
        coin_data_[i].valid = false;
        coin_data_[i].last_update = 0;
        coin_data_[i].price = 0.0;
        coin_data_[i].change_24h = 0.0;
        coin_data_[i].change_percent_24h = 0.0;
    }
}

void BinanceDataManager::parseSymbols(const char* symbols) {
    String symbols_str = String(symbols);
    coin_count_ = 0;

    int start = 0;
    int end = symbols_str.indexOf(',');

    while (end != -1 && coin_count_ < MAX_COINS) {
        symbols_[coin_count_] = symbols_str.substring(start, end);
        symbols_[coin_count_].trim();
        symbols_[coin_count_].toUpperCase();  // Ensure uppercase format
        
        // Initialize corresponding coin data
        coin_data_[coin_count_].symbol = symbols_[coin_count_];
        coin_data_[coin_count_].name = generateCoinName(symbols_[coin_count_]);
        coin_data_[coin_count_].valid = false;
        coin_data_[coin_count_].price = 0.0;
        coin_data_[coin_count_].change_24h = 0.0;
        coin_data_[coin_count_].change_percent_24h = 0.0;
        coin_data_[coin_count_].last_update = 0;
        
        coin_count_++;
        start = end + 1;
        end = symbols_str.indexOf(',', start);
    }

    // Add the last symbol
    if (start < symbols_str.length() && coin_count_ < MAX_COINS) {
        symbols_[coin_count_] = symbols_str.substring(start);
        symbols_[coin_count_].trim();
        symbols_[coin_count_].toUpperCase();
        
        // Initialize corresponding coin data
        coin_data_[coin_count_].symbol = symbols_[coin_count_];
        coin_data_[coin_count_].name = generateCoinName(symbols_[coin_count_]);
        coin_data_[coin_count_].valid = false;
        coin_data_[coin_count_].price = 0.0;
        coin_data_[coin_count_].change_24h = 0.0;
        coin_data_[coin_count_].change_percent_24h = 0.0;
        coin_data_[coin_count_].last_update = 0;
        
        coin_count_++;
    }

    SAFE_SERIAL_PRINTF("Parsed %d symbols: ", coin_count_);
    for (int i = 0; i < coin_count_; i++) {
        SAFE_SERIAL_PRINTF("%s ", symbols_[i].c_str());
    }
    SAFE_SERIAL_PRINTLN();
}

const String* BinanceDataManager::getSymbols() const {
    return symbols_;
}

int BinanceDataManager::getSymbolCount() const {
    return coin_count_;
}

const CoinData* BinanceDataManager::getCoinData() const {
    return coin_data_;
}

int BinanceDataManager::getCoinCount() const {
    return coin_count_;
}

void BinanceDataManager::updateCoinData(const String& symbol, float price, float change24h, float changePercent24h) {
    SAFE_SERIAL_PRINTF("Searching for symbol: '%s'\n", symbol.c_str());
    SAFE_SERIAL_PRINTF("Available symbols: ");
    for (int i = 0; i < coin_count_; i++) {
        SAFE_SERIAL_PRINTF("'%s' ", symbols_[i].c_str());
    }
    SAFE_SERIAL_PRINTLN();
    
    int index = findCoinIndex(symbol);
    if (index == -1) {
        // Symbol not found in our list
        SAFE_SERIAL_PRINTF("Symbol '%s' not found in configured list!\n", symbol.c_str());
        return;
    }
    
    unsigned long now = millis();
    
    coin_data_[index].price = price;
    coin_data_[index].change_24h = change24h;
    coin_data_[index].change_percent_24h = changePercent24h;
    coin_data_[index].valid = true;
    coin_data_[index].last_update = now;
    
    // Clear any previous errors when we receive valid data
    if (hasError()) {
        clearError();
    }
    
    SAFE_SERIAL_PRINTF("Updated %s: $%.2f (%.2f%%)\n", 
                  symbol.c_str(), price, changePercent24h);
}

bool BinanceDataManager::hasValidData() const {
    for (int i = 0; i < coin_count_; i++) {
        if (coin_data_[i].valid) {
            return true;
        }
    }
    return false;
}

bool BinanceDataManager::hasError() const {
    return !last_error_message_.isEmpty();
}

String BinanceDataManager::getLastError() const {
    return last_error_message_;
}

int BinanceDataManager::getValidCoinCount() const {
    int count = 0;
    for (int i = 0; i < coin_count_; i++) {
        if (coin_data_[i].valid) {
            count++;
        }
    }
    return count;
}

String BinanceDataManager::formatPrice(float price) const {
    // Format price with comma separators for better readability
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

void BinanceDataManager::clearError() {
    last_error_message_ = "";
}

void BinanceDataManager::setError(const String& error) {
    last_error_message_ = error;
    SAFE_SERIAL_PRINTF("CryptoDataManager error: %s\n", error.c_str());
}

int BinanceDataManager::findCoinIndex(const String& symbol) const {
    for (int i = 0; i < coin_count_; i++) {
        if (symbols_[i].equalsIgnoreCase(symbol)) {
            return i;
        }
    }
    return -1;
}

String BinanceDataManager::generateCoinName(const String& symbol) const {
    // Simple mapping from common Binance symbols to readable names
    if (symbol == "BTCUSDT") return "Bitcoin";
    if (symbol == "ETHUSDT") return "Ethereum";
    if (symbol == "ADAUSDT") return "Cardano";
    if (symbol == "SOLUSDT") return "Solana";
    if (symbol == "DOGEUSDT") return "Dogecoin";
    if (symbol == "DOTUSDT") return "Polkadot";
    if (symbol == "MATICUSDT") return "Polygon";
    if (symbol == "AVAXUSDT") return "Avalanche";
    if (symbol == "ATOMUSDT") return "Cosmos";
    if (symbol == "LINKUSDT") return "Chainlink";
    if (symbol == "UNIUSDT") return "Uniswap";
    if (symbol == "LTCUSDT") return "Litecoin";
    if (symbol == "BCHUSDT") return "Bitcoin Cash";
    if (symbol == "XRPUSDT") return "XRP";
    if (symbol == "BNBUSDT") return "BNB";
    
    // For unknown symbols, extract the base symbol (remove USDT)
    if (symbol.endsWith("USDT")) {
        return symbol.substring(0, symbol.length() - 4);
    }
    
    return symbol;
}