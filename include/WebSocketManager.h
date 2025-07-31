/**
 * @file WebSocketManager.h
 * @brief WebSocket connection management for Binance cryptocurrency data
 * @author Claude Code Assistant
 * @date 2024
 * 
 * This file contains the WebSocketManager class for handling real-time
 * WebSocket connections to Binance API. Manages connection lifecycle,
 * automatic reconnection, and real-time data streaming.
 */

#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <functional>
#include "constants.h"

// Forward declaration
struct CoinData;

/**
 * @class WebSocketManager
 * @brief Manages WebSocket connections to Binance API for real-time crypto data
 * 
 * The WebSocketManager handles all aspects of WebSocket communication with
 * Binance API including connection establishment, automatic reconnection with
 * exponential backoff, data parsing, and callback notifications.
 * 
 * Key features:
 * - Real-time WebSocket connection to Binance API
 * - Automatic reconnection with exponential backoff (5s to 60s)
 * - Connection health monitoring and stale connection detection
 * - JSON data parsing and callback notifications
 * - Error handling and status reporting
 * 
 * @see BinanceDataManager
 */
class WebSocketManager {
public:
    /**
     * @brief Callback function type for price update notifications
     * 
     * Function signature for callbacks that receive real-time price updates.
     * Called whenever new cryptocurrency price data is received via WebSocket.
     */
    using PriceUpdateCallback = std::function<void(const String& symbol, float price, float change24h, float changePercent24h)>;

    /**
     * @brief Constructor initializes WebSocket client and connection state
     */
    WebSocketManager();
    
    /**
     * @brief Default destructor
     */
    ~WebSocketManager() = default;

    // Connection management
    /**
     * @brief Establish WebSocket connection to Binance API
     * 
     * Attempts to connect to Binance WebSocket API and subscribe
     * to configured cryptocurrency data streams.
     * 
     * @return true if connection attempt initiated successfully
     */
    bool connect();
    
    /**
     * @brief Disconnect from WebSocket server
     * 
     * Cleanly closes WebSocket connection and updates connection state.
     */
    void disconnect();
    
    /**
     * @brief Temporarily pause WebSocket connection for memory management
     * 
     * Disconnects WebSocket to free SSL memory for HTTPS operations.
     * Connection state is preserved to enable resuming later.
     */
    void pauseForMemoryCleanup();
    
    /**
     * @brief Resume WebSocket connection after memory-intensive operations
     * 
     * Reconnects WebSocket using previously stored symbols and callbacks.
     * Restores normal real-time data streaming.
     */
    void resumeAfterMemoryCleanup();
    
    /**
     * @brief Check if WebSocket connection is currently active
     * 
     * @return true if connected and receiving data
     */
    bool isConnected() const;
    
    /**
     * @brief Process WebSocket events and maintain connection
     * 
     * Must be called regularly from main loop to process incoming
     * messages, handle disconnections, and maintain connection health.
     */
    void poll();

    // Symbol subscription
    /**
     * @brief Set cryptocurrency symbols to monitor
     * 
     * Configures which trading pairs to subscribe to for real-time updates.
     * Symbols should be in Binance format (e.g., "BTCUSDT", "ETHUSDT").
     * 
     * @param symbols Array of trading pair symbols
     * @param count Number of symbols in the array
     */
    void setSymbols(const String symbols[], int count);
    
    /**
     * @brief Set callback function for price update notifications
     * 
     * Registers a callback function that will be called whenever
     * new price data is received via WebSocket.
     * 
     * @param callback Function to call with price updates
     */
    void setPriceUpdateCallback(PriceUpdateCallback callback);

    // Connection status
    String getConnectionStatus() const;
    unsigned long getLastMessageTime() const;
    bool hasError() const;
    String getLastError() const;
    bool shouldReconnect() const;
    void processReconnection();  // Call in main loop

private:
    static const char* BINANCE_WS_HOST;
    static const int BINANCE_WS_PORT = 9443;
    static const char* BINANCE_WS_PATH;

    WebSocketsClient webSocket;
    String symbols_[MAX_COINS];  // Support up to MAX_COINS symbols
    int symbol_count_;
    
    // Callbacks
    PriceUpdateCallback price_callback_;
    
    // Connection state
    bool is_connected_;
    unsigned long last_message_time_;
    String connection_status_;
    String last_error_;
    
    // Reconnection state
    bool should_reconnect_;
    unsigned long last_reconnect_attempt_;
    int reconnect_attempts_;
    unsigned long reconnect_interval_;
    
    // Memory cleanup pause state
    bool paused_for_memory_cleanup_;
    bool was_connected_before_pause_;
    
    // WebSocket event handler
    void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);
    
    // Internal methods
    void subscribeToSymbols();
    void setError(const String& error);
    void clearError();
    void updateConnectionStatus(const String& status);
    void startReconnection();
    bool attemptReconnection();
    bool isConnectionStale() const;
    
    // Static wrapper for event handler
    static void webSocketEventWrapper(WStype_t type, uint8_t *payload, size_t length);
    static WebSocketManager* instance_;
};

#endif // WEBSOCKET_MANAGER_H