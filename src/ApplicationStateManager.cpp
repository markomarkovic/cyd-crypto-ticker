#include "ApplicationStateManager.h"
#include <lvgl.h>

ApplicationStateManager::ApplicationStateManager() 
    : current_app_state_(AppState::INITIALIZING),
      current_wifi_state_(WiFiState::DISCONNECTED),
      wifi_disconnection_detected_(false),
      wifi_disconnection_start_(0),
      reconnection_message_shown_(false),
      last_wifi_update_(0),
      last_crypto_update_(0),
      lv_last_tick_(0) {
}

void ApplicationStateManager::setAppState(AppState state) {
    current_app_state_ = state;
}

ApplicationStateManager::AppState ApplicationStateManager::getAppState() const {
    return current_app_state_;
}

void ApplicationStateManager::setWiFiState(WiFiState state) {
    current_wifi_state_ = state;
}

ApplicationStateManager::WiFiState ApplicationStateManager::getWiFiState() const {
    return current_wifi_state_;
}

void ApplicationStateManager::startWiFiDisconnection() {
    wifi_disconnection_detected_ = true;
    wifi_disconnection_start_ = millis();
    reconnection_message_shown_ = false;
}

void ApplicationStateManager::resetWiFiDisconnection() {
    wifi_disconnection_detected_ = false;
    reconnection_message_shown_ = false;
    wifi_disconnection_start_ = 0;
}

bool ApplicationStateManager::isWiFiDisconnected() const {
    return wifi_disconnection_detected_;
}

unsigned long ApplicationStateManager::getWiFiDisconnectionDuration() const {
    if (!wifi_disconnection_detected_) {
        return 0;
    }
    return millis() - wifi_disconnection_start_;
}

void ApplicationStateManager::setReconnectionMessageShown(bool shown) {
    reconnection_message_shown_ = shown;
}

bool ApplicationStateManager::isReconnectionMessageShown() const {
    return reconnection_message_shown_;
}

void ApplicationStateManager::updateWiFiTimestamp() {
    last_wifi_update_ = millis();
}

void ApplicationStateManager::updateCryptoTimestamp() {
    last_crypto_update_ = millis();
}

bool ApplicationStateManager::shouldUpdateWiFi(unsigned long wifi_update_interval) const {
    return (millis() - last_wifi_update_) > wifi_update_interval;
}

bool ApplicationStateManager::shouldUpdateCrypto(unsigned long crypto_update_interval) const {
    return (millis() - last_crypto_update_) > crypto_update_interval;
}

unsigned long ApplicationStateManager::getTimeSinceLastWiFiUpdate() const {
    return millis() - last_wifi_update_;
}

unsigned long ApplicationStateManager::getTimeSinceLastCryptoUpdate() const {
    return millis() - last_crypto_update_;
}

void ApplicationStateManager::updateLVGLTick() {
    unsigned long now = millis();
    lv_tick_inc(now - lv_last_tick_);
    lv_last_tick_ = now;
}