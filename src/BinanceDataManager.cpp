#include "BinanceDataManager.h"
#include "NetworkManager.h"
#include "constants.h"
#include <ArduinoJson.h>

// Global interval constants implementation
const char* SUPPORTED_INTERVALS[INTERVAL_COUNT] = {
    "1m", "3m", "5m", "15m", "30m", "1h", "2h", "4h", "6h", "8h", "12h", "1d", "1w", "1M"
};

const unsigned long INTERVAL_REFRESH_RATES[INTERVAL_COUNT] = {
    60000UL,        // 1m
    180000UL,       // 3m
    300000UL,       // 5m
    900000UL,       // 15m
    1800000UL,      // 30m
    3600000UL,      // 1h
    7200000UL,      // 2h
    14400000UL,     // 4h
    21600000UL,     // 6h
    28800000UL,     // 8h
    43200000UL,     // 12h
    86400000UL,     // 1d
    604800000UL,    // 1w
    2592000000UL    // 1M (30 days)
};

BinanceDataManager::BinanceDataManager() 
    : coin_count_(0), candlestick_count_(0), candlestick_last_update_(0), symbols_shown_(false), candlestick_interval_("1h") {
    // Initialize coin data array
    for (int i = 0; i < MAX_COINS; i++) {
        coin_data_[i].valid = false;
        coin_data_[i].last_update = 0;
        coin_data_[i].price = 0.0;
        coin_data_[i].change_24h = 0.0;
        coin_data_[i].change_percent_24h = 0.0;
    }
    
    // Initialize candlestick data array
    for (int i = 0; i < MAX_CANDLESTICKS; i++) {
        candlestick_data_[i].valid = false;
        candlestick_data_[i].timestamp = 0;
        candlestick_data_[i].open = 0.0;
        candlestick_data_[i].high = 0.0;
        candlestick_data_[i].low = 0.0;
        candlestick_data_[i].close = 0.0;
        candlestick_data_[i].volume = 0.0;
    }
}

BinanceDataManager::~BinanceDataManager() {
    // Destructor - no cleanup needed for synchronous implementation
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

    LOG_DEBUGF("Parsed %d symbols: ", coin_count_);
    for (int i = 0; i < coin_count_; i++) {
        LOG_DEBUGF("%s ", symbols_[i].c_str());
    }
    LOG_DEBUG();
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
    // Show available symbols only once after reconnection
    if (!symbols_shown_) {
        String symbols_list = "Available symbols: ";
        for (int i = 0; i < coin_count_; i++) {
            symbols_list += symbols_[i];
            if (i < coin_count_ - 1) symbols_list += " ";
        }
        LOG_DEBUG(symbols_list);
        symbols_shown_ = true;
    }
    
    int index = findCoinIndex(symbol);
    if (index == -1) {
        // Symbol not found in our list
        LOG_DEBUGF("Symbol '%s' not found in configured list!\n", symbol.c_str());
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
    
    LOG_DEBUGF("%s: %.2f / %+.2f / %+.2f%%", 
                  symbol.c_str(), price, change24h, changePercent24h);
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


void BinanceDataManager::clearError() {
    last_error_message_ = "";
}

void BinanceDataManager::resetSymbolsDisplay() {
    symbols_shown_ = false;
}

void BinanceDataManager::setError(const String& error) {
    last_error_message_ = error;
    LOG_DEBUGF("CryptoDataManager error: %s\n", error.c_str());
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


const CandlestickData* BinanceDataManager::getCandlestickData() const {
    return candlestick_data_;
}

int BinanceDataManager::getCandlestickCount() const {
    return candlestick_count_;
}

bool BinanceDataManager::hasCandlestickData() const {
    return candlestick_count_ > 0 && !candlestick_symbol_.isEmpty();
}

String BinanceDataManager::getCurrentCandlestickSymbol() const {
    return candlestick_symbol_;
}

String BinanceDataManager::getCurrentCandlestickInterval() const {
    return candlestick_interval_;
}

bool BinanceDataManager::parseCandlestickJson(const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        LOG_ERROR("JSON parsing failed: " + String(error.c_str()));
        setError("Chart data parsing failed");
        return false;
    }
    
    if (!doc.is<JsonArray>()) {
        LOG_ERROR("Response is not a JSON array");
        setError("Invalid chart data format");
        return false;
    }
    
    JsonArray klines = doc.as<JsonArray>();
    candlestick_count_ = 0;
    
    // Parse each kline entry
    for (JsonArray kline : klines) {
        if (candlestick_count_ >= MAX_CANDLESTICKS) {
            LOG_WARN("Reached maximum candlestick limit");
            break;
        }
        
        if (kline.size() >= 6) {  // Binance klines have at least 6 elements
            candlestick_data_[candlestick_count_].timestamp = kline[0].as<uint64_t>();
            candlestick_data_[candlestick_count_].open = kline[1].as<float>();
            candlestick_data_[candlestick_count_].high = kline[2].as<float>();
            candlestick_data_[candlestick_count_].low = kline[3].as<float>();
            candlestick_data_[candlestick_count_].close = kline[4].as<float>();
            candlestick_data_[candlestick_count_].volume = kline[5].as<float>();
            candlestick_data_[candlestick_count_].valid = true;
            candlestick_count_++;
        }
    }
    
    LOG_DEBUG("Parsed " + String(candlestick_count_) + " candlesticks");
    return candlestick_count_ > 0;
}

bool BinanceDataManager::fetchCandlestickDataSync(const String& symbol, const String& interval, int limit, NetworkManager& network_manager) {
    // Validate symbol format (should be like BTCUSDT)
    if (symbol.isEmpty() || !symbol.endsWith("USDT")) {
        setError("Invalid symbol format");
        return false;
    }
    
    // Limit the number of candlesticks to prevent memory issues
    if (limit > MAX_CANDLESTICKS) {
        limit = MAX_CANDLESTICKS;
    }
    
    // Build URL for Binance API
    String url = "https://api.binance.com/api/v3/klines?symbol=" + symbol + 
                 "&interval=" + interval + "&limit=" + String(limit);
    
    LOG_INFO("Starting sync fetch of candlestick data for " + symbol + " (" + interval + ", " + String(limit) + " candles)...");
    LOG_DEBUG("Free heap before HTTPS request: " + String(ESP.getFreeHeap()) + " bytes");
    
    // Make synchronous HTTPS request
    String response;
    int http_code;
    bool success = network_manager.httpGet(url, "", response, http_code);
    
    if (!success) {
        String error_msg = "HTTP request failed with code: " + String(http_code);
        setError(error_msg);
        LOG_ERROR("Sync candlestick fetch failed: " + error_msg);
        return false;
    }
    
    // Parse the JSON response
    if (parseCandlestickJson(response)) {
        candlestick_last_update_ = millis();
        candlestick_interval_ = interval; // Store the interval for display
        clearError();
        LOG_INFO("Sync candlestick data fetch completed successfully (" + String(candlestick_count_) + " candles)");
        return true;
    } else {
        setError("Failed to parse candlestick data from API");
        LOG_ERROR("Sync candlestick fetch failed: JSON parsing error");
        return false;
    }
}

bool BinanceDataManager::setCurrentCandlestickInterval(const String& interval) {
    if (!isValidInterval(interval)) {
        LOG_WARN("Invalid interval provided: " + interval);
        return false;
    }
    
    candlestick_interval_ = interval;
    LOG_INFO("Candlestick interval changed to: " + interval);
    return true;
}

bool BinanceDataManager::isValidInterval(const String& interval) const {
    for (int i = 0; i < INTERVAL_COUNT; i++) {
        if (interval == SUPPORTED_INTERVALS[i]) {
            return true;
        }
    }
    return false;
}

unsigned long BinanceDataManager::getIntervalRefreshRate(const String& interval) const {
    for (int i = 0; i < INTERVAL_COUNT; i++) {
        if (interval == SUPPORTED_INTERVALS[i]) {
            return INTERVAL_REFRESH_RATES[i];
        }
    }
    return 0; // Invalid interval
}


