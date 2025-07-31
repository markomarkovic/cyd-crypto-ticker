/**
 * @file BinanceDataManager.h
 * @brief Cryptocurrency data management for Binance WebSocket integration
 * @author Claude Code Assistant
 * @date 2024
 * 
 * This file contains the BinanceDataManager class and CoinData structure for
 * managing real-time cryptocurrency price data from Binance WebSocket API.
 * Handles data parsing, storage, and formatting for display.
 */

#ifndef BINANCE_DATA_MANAGER_H
#define BINANCE_DATA_MANAGER_H

#include <Arduino.h>
#include "constants.h"

// Forward declarations
class NetworkManager;

/**
 * @struct CandlestickData
 * @brief Structure to hold OHLCV (Open, High, Low, Close, Volume) candlestick data
 * 
 * Contains candlestick chart data for a specific time period, typically used
 * for displaying price action over time in chart format. Each candlestick represents
 * price movement during a fixed time interval (1m, 5m, 1h, 1d, etc.).
 * 
 * @note All price values are in the quote currency (typically USDT)
 * @note Timestamp is Unix timestamp in milliseconds from Binance API
 * @note Volume represents trading volume during the time period
 */
struct CandlestickData {
    /**
     * @brief Timestamp of the candlestick period (Unix timestamp in milliseconds)
     */
    unsigned long timestamp;
    
    /**
     * @brief Opening price at the start of the time period
     */
    float open;
    
    /**
     * @brief Highest price during the time period
     */
    float high;
    
    /**
     * @brief Lowest price during the time period
     */
    float low;
    
    /**
     * @brief Closing price at the end of the time period
     */
    float close;
    
    /**
     * @brief Trading volume during the time period
     */
    float volume;
    
    /**
     * @brief Flag indicating if this candlestick data is valid
     */
    bool valid;
};

/**
 * @struct CoinData
 * @brief Structure to hold cryptocurrency data for a single trading pair
 * 
 * Contains all relevant information for displaying cryptocurrency prices
 * including current price, 24-hour changes, and metadata for tracking
 * data validity and update timestamps.
 */
struct CoinData {
    /**
     * @brief Trading pair symbol (e.g., "BTCUSDT", "ETHUSDT")
     */
    String symbol;
    
    /**
     * @brief Human-readable coin name (e.g., "Bitcoin", "Ethereum")
     */
    String name;
    
    /**
     * @brief Current price in USDT
     */
    float price;
    
    /**
     * @brief 24-hour price change in absolute USDT value
     */
    float change_24h;
    
    /**
     * @brief 24-hour price change as percentage
     */
    float change_percent_24h;
    
    /**
     * @brief Flag indicating if this data entry contains valid information
     */
    bool valid;
    
    /**
     * @brief Timestamp of last data update (milliseconds since boot)
     */
    unsigned long last_update;
};

/**
 * @class BinanceDataManager
 * @brief Manages cryptocurrency data from Binance WebSocket streams
 * 
 * The BinanceDataManager handles real-time cryptocurrency price data received
 * from Binance WebSocket API. It parses trading pair symbols, stores price data,
 * and provides formatted access to the information for display purposes.
 * 
 * Key features:
 * - Real-time data updates via WebSocket callbacks
 * - Support for up to 10 concurrent cryptocurrency pairs
 * - Automatic price formatting with comma separators
 * - Data validation and error tracking
 * - Symbol parsing and name generation
 * 
 * @see WebSocketManager
 * @see CoinData
 */
class BinanceDataManager {
public:
    
    /**
     * @brief Constructor initializes empty data arrays and counters
     */
    BinanceDataManager();
    
    /**
     * @brief Destructor - ensures proper cleanup of HTTP fetch task
     */
    ~BinanceDataManager();
    
    // Symbol management
    /**
     * @brief Parse comma-separated symbol string into individual trading pairs
     * 
     * Takes a comma-separated string of trading pairs and parses them into
     * individual symbols for WebSocket subscription. Automatically converts
     * to uppercase and validates format.
     * 
     * @param symbols Comma-separated string (e.g., "btcusdt,ethusdt,solusdt")
     */
    void parseSymbols(const char* symbols);
    
    /**
     * @brief Get array of parsed trading pair symbols
     * 
     * @return Pointer to array of symbol strings
     */
    const String* getSymbols() const;
    
    /**
     * @brief Get number of trading pairs currently configured
     * 
     * @return Number of symbols in the array
     */
    int getSymbolCount() const;
    
    // Data access
    /**
     * @brief Get array of cryptocurrency data structures
     * 
     * @return Pointer to array of CoinData structures containing current prices
     */
    const CoinData* getCoinData() const;
    
    /**
     * @brief Get total number of cryptocurrency entries
     * 
     * @return Number of CoinData entries (may include invalid entries)
     */
    int getCoinCount() const;
    
    // Real-time data updates (called by WebSocket)
    /**
     * @brief Update cryptocurrency data with new price information
     * 
     * Called by WebSocket manager when new price data is received.
     * Updates the corresponding CoinData entry and marks it as valid.
     * 
     * @param symbol Trading pair symbol (e.g., "BTCUSDT")
     * @param price Current price in USDT
     * @param change24h 24-hour absolute price change in USDT
     * @param changePercent24h 24-hour percentage price change
     */
    void updateCoinData(const String& symbol, float price, float change24h, float changePercent24h);
    
    // Status
    /**
     * @brief Check if any valid cryptocurrency data is available
     * 
     * @return true if at least one CoinData entry is valid
     */
    bool hasValidData() const;
    
    /**
     * @brief Check if any errors have occurred
     * 
     * @return true if error condition exists
     */
    bool hasError() const;
    
    /**
     * @brief Get description of last error that occurred
     * 
     * @return Error message string, empty if no error
     */
    String getLastError() const;
    
    /**
     * @brief Get number of CoinData entries with valid data
     * 
     * @return Count of valid cryptocurrency entries
     */
    int getValidCoinCount() const;
    
    // Candlestick chart data management
    /**
     * @brief Synchronously fetch candlestick chart data for a trading pair
     * 
     * Fetches historical OHLCV data from Binance API with WebSocket paused
     * for memory management. Blocks until complete or failed.
     * 
     * @param symbol Trading pair symbol (e.g., "BTCUSDT")
     * @param interval Time interval for candlesticks (e.g., "1h", "4h", "1d")
     * @param limit Number of candlesticks to fetch (max 100)
     * @param network_manager NetworkManager reference for HTTP requests
     * @return true if fetch completed successfully, false on error
     */
    bool fetchCandlestickDataSync(const String& symbol, const String& interval, int limit, NetworkManager& network_manager);
    
    /**
     * @brief Get array of candlestick data
     * 
     * @return Pointer to array of CandlestickData structures
     */
    const CandlestickData* getCandlestickData() const;
    
    /**
     * @brief Get number of valid candlesticks in the array
     * 
     * @return Number of candlesticks with valid data
     */
    int getCandlestickCount() const;
    
    /**
     * @brief Check if candlestick data is available and recent
     * 
     * @return true if candlestick data exists and is not stale
     */
    bool hasCandlestickData() const;
    
    /**
     * @brief Get symbol for which candlestick data was last fetched
     * 
     * @return Trading pair symbol of current candlestick data
     */
    String getCurrentCandlestickSymbol() const;
    
    /**
     * @brief Get time interval for current candlestick data
     * 
     * @return Time interval string (e.g., "1h", "4h", "1d")
     */
    String getCurrentCandlestickInterval() const;
    
    // Error management
    /**
     * @brief Set error message for debugging and status reporting
     * 
     * @param error Error description string
     */
    void setError(const String& error);
    
    /**
     * @brief Reset the symbols display flag to show symbols on next reconnection
     */
    void resetSymbolsDisplay();
    
private:
    /**
     * @brief Array of cryptocurrency data entries
     */
    CoinData coin_data_[MAX_COINS];
    
    /**
     * @brief Array of trading pair symbols
     */
    String symbols_[MAX_COINS];
    
    /**
     * @brief Current number of configured cryptocurrency pairs
     */
    int coin_count_;
    
    /**
     * @brief Array of candlestick data for chart display
     */
    CandlestickData candlestick_data_[MAX_CANDLESTICKS];
    
    /**
     * @brief Number of valid candlesticks in the array
     */
    int candlestick_count_;
    
    /**
     * @brief Symbol for which candlestick data was last fetched
     */
    String candlestick_symbol_;
    
    /**
     * @brief Timestamp when candlestick data was last fetched
     */
    unsigned long candlestick_last_update_;
    
    /**
     * @brief Time interval for candlestick data (e.g., "1h", "4h", "1d")
     */
    String candlestick_interval_;
    
    /**
     * @brief Last error message for debugging
     */
    String last_error_message_;
    
    /**
     * @brief Flag to track if symbols have been shown after reconnection
     */
    bool symbols_shown_;
    
    /**
     * @brief Clear any existing error condition
     */
    void clearError();
    
    /**
     * @brief Find array index for given trading pair symbol
     * 
     * @param symbol Trading pair symbol to search for
     * @return Array index if found, -1 if not found
     */
    int findCoinIndex(const String& symbol) const;
    
    /**
     * @brief Generate human-readable coin name from trading pair symbol
     * 
     * Converts trading pair symbols like "BTCUSDT" to readable names
     * like "Bitcoin" for display purposes.
     * 
     * @param symbol Trading pair symbol (e.g., "BTCUSDT")
     * @return Human-readable coin name
     */
    String generateCoinName(const String& symbol) const;
    
    /**
     * @brief Parse Binance klines JSON response into candlestick data
     * 
     * Processes the JSON array response from Binance /api/v3/klines endpoint
     * and populates the internal candlestick_data_ array.
     * 
     * @param payload JSON response string from Binance API
     * @return true if parsing was successful, false on error
     */
    bool parseCandlestickJson(const String& payload);
    
};

#endif // BINANCE_DATA_MANAGER_H