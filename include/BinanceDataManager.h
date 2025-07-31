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
     * @brief Maximum number of cryptocurrency pairs supported
     */
    static const int MAX_COINS = 10;
    
    /**
     * @brief Constructor initializes empty data arrays and counters
     */
    BinanceDataManager();
    
    /**
     * @brief Default destructor
     */
    ~BinanceDataManager() = default;
    
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
    
    // Utility
    /**
     * @brief Format price value for display with comma separators
     * 
     * Formats floating-point price values with appropriate decimal places
     * and comma separators for thousands (e.g., "1,234.56").
     * 
     * @param price Price value to format
     * @return Formatted price string
     */
    String formatPrice(float price) const;
    
    // Error management
    /**
     * @brief Set error message for debugging and status reporting
     * 
     * @param error Error description string
     */
    void setError(const String& error);
    
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
     * @brief Last error message for debugging
     */
    String last_error_message_;
    
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
};

#endif // BINANCE_DATA_MANAGER_H