#include "WebSocketManager.h"
#include "constants.h"

const char* WebSocketManager::BINANCE_WS_HOST = "stream.binance.com";
const char* WebSocketManager::BINANCE_WS_PATH = "/ws";

// Static instance pointer for callback
WebSocketManager* WebSocketManager::instance_ = nullptr;

WebSocketManager::WebSocketManager() 
    : symbol_count_(0), is_connected_(false), last_message_time_(0), connection_status_("Disconnected"),
      should_reconnect_(false), last_reconnect_attempt_(0), reconnect_attempts_(0), reconnect_interval_(WEBSOCKET_RECONNECT_INTERVAL) {
    instance_ = this;
}

void WebSocketManager::initialize() {
    LOG_INFO("WebSocketManager initialized with Links2004 WebSockets library");
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
    DynamicJsonDocument doc(1024);
    doc["method"] = "SUBSCRIBE";
    doc["id"] = 1;
    
    JsonArray params = doc.createNestedArray("params");
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
                DynamicJsonDocument doc(2048);
                DeserializationError error = deserializeJson(doc, message);
                
                if (error) {
                    LOG_ERRORF("JSON parsing failed: %s", error.c_str());
                    return;
                }
                
                // Handle direct ticker messages (individual stream format)
                if (doc.containsKey("e") && doc["e"] == "24hrTicker") {
                    String symbol = doc["s"];
                    float price = doc["c"].as<float>();  // Current price
                    float change24h = doc["p"].as<float>();  // 24h price change
                    float changePercent24h = doc["P"].as<float>();  // 24h change percent
                    
                    LOG_DEBUGF("Ticker: %s, Price: %.2f, Change: %.2f (%.2f%%)", 
                                 symbol.c_str(), price, change24h, changePercent24h);
                    
                    if (price_callback_) {
                        price_callback_(symbol, price, change24h, changePercent24h);
                    }
                }
                // Handle multiplexed stream format (if using /stream endpoint)
                else if (doc.containsKey("stream") && doc.containsKey("data")) {
                    String stream = doc["stream"];
                    JsonObject data = doc["data"];
                    
                    if (stream.endsWith("@ticker") && data.containsKey("e") && data["e"] == "24hrTicker") {
                        String symbol = data["s"];
                        float price = data["c"].as<float>();  // Current price
                        float change24h = data["p"].as<float>();  // 24h price change
                        float changePercent24h = data["P"].as<float>();  // 24h change percent
                        
                        LOG_DEBUGF("Ticker: %s, Price: %.2f, Change: %.2f (%.2f%%)", 
                                     symbol.c_str(), price, change24h, changePercent24h);
                        
                        if (price_callback_) {
                            price_callback_(symbol, price, change24h, changePercent24h);
                        }
                    }
                }
                // Handle subscription confirmations
                else if (doc.containsKey("result") && doc.containsKey("id")) {
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
                LOG_WARNF("WebSocket reconnection failed, retry in %d seconds", reconnect_interval_ / 1000);
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