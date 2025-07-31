#include "WebSocketManager.h"
#include "constants.h"

const char* WebSocketManager::BINANCE_WS_HOST = "stream.binance.com";
const char* WebSocketManager::BINANCE_WS_PATH = "/ws";

// Static instance pointer for callback
WebSocketManager* WebSocketManager::instance_ = nullptr;

WebSocketManager::WebSocketManager() 
    : symbol_count_(0), is_connected_(false), last_message_time_(0), connection_status_("Disconnected"),
      should_reconnect_(false), last_reconnect_attempt_(0), reconnect_attempts_(0), reconnect_interval_(WEBSOCKET_RECONNECT_INTERVAL),
      paused_for_memory_cleanup_(false), was_connected_before_pause_(false) {
    instance_ = this;
}

bool WebSocketManager::connect() {
    if (is_connected_) {
        return true;
    }
    
    updateConnectionStatus("Connecting...");
    LOG_INFOF("Connecting to Binance WebSocket: %s:%d%s", BINANCE_WS_HOST, BINANCE_WS_PORT, BINANCE_WS_PATH);
    
    // Add detailed debugging
    LOG_DEBUGF("Free heap before connection: %d", ESP.getFreeHeap());
    LOG_DEBUGF("WiFi status: %d", WiFi.status());
    LOG_DEBUGF("WiFi RSSI: %d", WiFi.RSSI());
    
    // Set up WebSocket connection using SSL
    webSocket.beginSSL(BINANCE_WS_HOST, BINANCE_WS_PORT, BINANCE_WS_PATH);
    webSocket.onEvent(webSocketEventWrapper);
    
    // Set WebSocket options for better stability
    webSocket.setReconnectInterval(5000);  // 5 seconds between reconnection attempts
    webSocket.enableHeartbeat(15000, 3000, 2);  // Ping every 15s, expect pong within 3s, disconnect after 2 missed pongs
    
    // The connection is established asynchronously, wait for event
    unsigned long start_time = millis();
    const unsigned long CONNECT_TIMEOUT = 15000; // 15 seconds
    
    while (!is_connected_ && (millis() - start_time) < CONNECT_TIMEOUT) {
        webSocket.loop();
        delay(10);
    }
    
    if (is_connected_) {
        LOG_INFO("Connected to Binance WebSocket successfully!");
        subscribeToSymbols();
        return true;
    } else {
        LOG_ERROR("Failed to connect to Binance WebSocket within timeout");
        setError("Connection timeout");
        return false;
    }
}

void WebSocketManager::disconnect() {
    if (is_connected_) {
        webSocket.disconnect();
        is_connected_ = false;
        updateConnectionStatus("Disconnected");
        LOG_INFO("Disconnected from Binance WebSocket");
    }
}

void WebSocketManager::pauseForMemoryCleanup() {
    LOG_INFO("Pausing WebSocket for memory cleanup (HTTPS operation)");
    LOG_DEBUG("Free heap before WebSocket pause: " + String(ESP.getFreeHeap()) + " bytes");
    
    // Store current connection state
    was_connected_before_pause_ = is_connected_;
    paused_for_memory_cleanup_ = true;
    
    // Stop any reconnection attempts
    should_reconnect_ = false;
    
    // Disconnect to free SSL memory
    if (is_connected_) {
        webSocket.disconnect();
        is_connected_ = false;
        updateConnectionStatus("Paused for HTTPS");
        LOG_DEBUG("WebSocket disconnected for memory cleanup");
    }
    
    LOG_DEBUG("Free heap after WebSocket pause: " + String(ESP.getFreeHeap()) + " bytes");
}

void WebSocketManager::resumeAfterMemoryCleanup() {
    if (!paused_for_memory_cleanup_) {
        LOG_WARN("resumeAfterMemoryCleanup called but WebSocket was not paused");
        return;
    }
    
    LOG_INFO("Resuming WebSocket after memory cleanup");
    LOG_DEBUG("Free heap before WebSocket resume: " + String(ESP.getFreeHeap()) + " bytes");
    
    paused_for_memory_cleanup_ = false;
    
    // If we were connected before pause, try to reconnect
    if (was_connected_before_pause_) {
        LOG_DEBUG("Attempting to reconnect WebSocket after memory cleanup");
        if (connect()) {
            LOG_INFO("WebSocket successfully reconnected after memory cleanup");
        } else {
            LOG_WARN("WebSocket reconnection failed after memory cleanup, will retry with normal reconnection logic");
            startReconnection();
        }
    } else {
        LOG_DEBUG("WebSocket was not connected before pause, not attempting reconnection");
    }
    
    was_connected_before_pause_ = false;
}

bool WebSocketManager::isConnected() const {
    return is_connected_;
}

void WebSocketManager::poll() {
    webSocket.loop();
}

void WebSocketManager::setSymbols(const String symbols[], int count) {
    symbol_count_ = min(count, 10);
    for (int i = 0; i < symbol_count_; i++) {
        symbols_[i] = symbols[i];
        symbols_[i].toLowerCase(); // Binance WebSocket expects lowercase
    }
    
    LOG_INFOF("WebSocketManager: Set %d symbols for subscription", symbol_count_);
}

void WebSocketManager::setPriceUpdateCallback(PriceUpdateCallback callback) {
    price_callback_ = callback;
}

void WebSocketManager::subscribeToSymbols() {
    if (symbol_count_ == 0) {
        LOG_WARN("No symbols to subscribe to");
        return;
    }
    
    // Create subscription message for multiple symbols
    JsonDocument doc;
    doc["method"] = "SUBSCRIBE";
    doc["id"] = 1;
    
    JsonArray params = doc["params"].to<JsonArray>();
    for (int i = 0; i < symbol_count_; i++) {
        String stream = symbols_[i] + "@ticker";
        params.add(stream);
        LOG_DEBUGF("Subscribing to: %s", stream.c_str());
    }
    
    String subscription_message;
    serializeJson(doc, subscription_message);
    
    LOG_DEBUGF("Sending subscription: %s", subscription_message.c_str());
    webSocket.sendTXT(subscription_message);
    
    updateConnectionStatus("Subscribed");
}

String WebSocketManager::getConnectionStatus() const {
    return connection_status_;
}

unsigned long WebSocketManager::getLastMessageTime() const {
    return last_message_time_;
}

bool WebSocketManager::hasError() const {
    return !last_error_.isEmpty();
}

String WebSocketManager::getLastError() const {
    return last_error_;
}

// Static wrapper function
void WebSocketManager::webSocketEventWrapper(WStype_t type, uint8_t *payload, size_t length) {
    if (instance_) {
        instance_->onWebSocketEvent(type, payload, length);
    }
}

void WebSocketManager::onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            LOG_INFOF("WebSocket Disconnected (was connected for %lu ms)", 
                     last_message_time_ > 0 ? millis() - last_message_time_ : 0);
            LOG_DEBUGF("Free heap at disconnect: %d", ESP.getFreeHeap());
            is_connected_ = false;
            updateConnectionStatus("Disconnected");
            startReconnection();
            break;
            
        case WStype_CONNECTED:
            LOG_INFOF("WebSocket Connected to: %s", payload);
            is_connected_ = true;
            last_message_time_ = millis();
            updateConnectionStatus("Connected");
            clearError();
            break;
            
        case WStype_TEXT:
            {
                last_message_time_ = millis();
                String message = String((char*)payload);
                LOG_TRACEF("Received WebSocket message: %s", message.c_str());
                
                // Parse JSON message
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, message);
                
                if (error) {
                    LOG_ERRORF("JSON parsing failed: %s", error.c_str());
                    return;
                }
                
                // Handle direct ticker messages (individual stream format)
                if (doc["e"].is<const char*>() && doc["e"] == "24hrTicker") {
                    String symbol = doc["s"];
                    float price = doc["c"].as<float>();  // Current price
                    float change24h = doc["p"].as<float>();  // 24h price change
                    float changePercent24h = doc["P"].as<float>();  // 24h change percent
                    
                    // Price update will be logged by BinanceDataManager in compact format
                    
                    if (price_callback_) {
                        price_callback_(symbol, price, change24h, changePercent24h);
                    }
                }
                // Handle multiplexed stream format (if using /stream endpoint)
                else if (doc["stream"].is<const char*>() && doc["data"].is<JsonObject>()) {
                    String stream = doc["stream"];
                    JsonObject data = doc["data"];
                    
                    if (stream.endsWith("@ticker") && data["e"].is<const char*>() && data["e"] == "24hrTicker") {
                        String symbol = data["s"];
                        float price = data["c"].as<float>();  // Current price
                        float change24h = data["p"].as<float>();  // 24h price change
                        float changePercent24h = data["P"].as<float>();  // 24h change percent
                        
                        // Price update will be logged by BinanceDataManager in compact format
                        
                        if (price_callback_) {
                            price_callback_(symbol, price, change24h, changePercent24h);
                        }
                    }
                }
                // Handle subscription confirmations
                else if (doc["result"].is<JsonVariant>() && doc["id"].is<int>()) {
                    if (doc["result"].isNull()) {
                        LOG_INFO("Successfully subscribed to streams");
                        updateConnectionStatus("Active");
                    } else {
                        LOG_ERROR("Subscription error");
                        setError("Subscription failed");
                    }
                }
            }
            break;
            
        case WStype_ERROR:
            LOG_ERRORF("WebSocket Error: %s", payload);
            setError("WebSocket error: " + String((char*)payload));
            break;
            
        case WStype_BIN:
            LOG_WARN("WebSocket received binary data (unexpected)");
            break;
            
        case WStype_PING:
            LOG_TRACE("WebSocket received ping");
            last_message_time_ = millis(); // Update activity timestamp
            break;
            
        case WStype_PONG:
            LOG_TRACE("WebSocket received pong");
            last_message_time_ = millis(); // Update activity timestamp
            break;
            
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            LOG_TRACE("WebSocket received fragmented message (handling not implemented)");
            break;
            
        default:
            // Only log unknown event types at DEBUG level to reduce noise
            LOG_DEBUGF("WebSocket unhandled event type: %d", type);
            break;
    }
}

void WebSocketManager::setError(const String& error) {
    last_error_ = error;
    LOG_ERRORF("WebSocketManager error: %s", error.c_str());
}

void WebSocketManager::clearError() {
    last_error_ = "";
}

void WebSocketManager::updateConnectionStatus(const String& status) {
    connection_status_ = status;
    LOG_DEBUGF("WebSocket status: %s", status.c_str());
}

bool WebSocketManager::shouldReconnect() const {
    return should_reconnect_ && reconnect_attempts_ < WEBSOCKET_MAX_RETRY_ATTEMPTS;
}

void WebSocketManager::processReconnection() {
    // Skip reconnection processing if paused for memory cleanup
    if (paused_for_memory_cleanup_) {
        return;
    }
    
    // Check for stale connection (no messages for a while)
    if (is_connected_ && isConnectionStale()) {
        LOG_WARN("WebSocket connection is stale, forcing disconnect");
        disconnect();
        startReconnection();
        return;
    }
    
    // Handle reconnection attempts
    if (should_reconnect_ && millis() - last_reconnect_attempt_ >= reconnect_interval_) {
        if (reconnect_attempts_ < WEBSOCKET_MAX_RETRY_ATTEMPTS) {
            LOG_INFOF("Attempting WebSocket reconnection (%d/%d)", reconnect_attempts_ + 1, WEBSOCKET_MAX_RETRY_ATTEMPTS);
            if (attemptReconnection()) {
                LOG_INFO("WebSocket reconnection successful");
                should_reconnect_ = false;
                reconnect_attempts_ = 0;
                reconnect_interval_ = WEBSOCKET_RECONNECT_INTERVAL; // Reset interval
            } else {
                reconnect_attempts_++;
                last_reconnect_attempt_ = millis();
                // Exponential backoff: double the interval up to 60 seconds
                reconnect_interval_ = min(reconnect_interval_ * 2, 60000UL);
                LOG_WARNF("WebSocket reconnection failed, retry in %lu seconds", reconnect_interval_ / 1000);
            }
        } else {
            LOG_ERROR("WebSocket reconnection failed after maximum attempts");
            should_reconnect_ = false;
            setError("WebSocket reconnection failed after maximum attempts");
        }
    }
}

void WebSocketManager::startReconnection() {
    if (!should_reconnect_) {
        LOG_INFO("Starting WebSocket reconnection process");
        should_reconnect_ = true;
        reconnect_attempts_ = 0;
        reconnect_interval_ = WEBSOCKET_RECONNECT_INTERVAL;
        last_reconnect_attempt_ = 0; // Attempt immediately
        updateConnectionStatus("Reconnecting");
    }
}

bool WebSocketManager::attemptReconnection() {
    if (connect()) {
        return true;
    }
    return false;
}

bool WebSocketManager::isConnectionStale() const {
    return is_connected_ && 
           last_message_time_ > 0 && 
           (millis() - last_message_time_) > WEBSOCKET_MESSAGE_TIMEOUT;
}