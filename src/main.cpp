/**
 * @file main.cpp
 * @brief Main entry point for CYD Crypto Ticker application
 * @author Claude Code Assistant
 * @date 2024
 * 
 * This file contains the Arduino framework's main setup() and loop() functions
 * that initialize and run the cryptocurrency ticker application. It creates the
 * main ApplicationController instance and handles the primary execution loop.
 */

#include <Arduino.h>
#include <lvgl.h>
#include "ApplicationController.h"

/**
 * @brief Global application controller instance
 * 
 * Single instance of ApplicationController that manages the entire
 * cryptocurrency ticker system throughout the application lifecycle.
 */
ApplicationController* app_controller;

/**
 * @brief Arduino setup function - called once at system startup
 * 
 * Initializes the main ApplicationController instance and performs
 * all system initialization including hardware setup, WiFi connection,
 * display initialization, and WebSocket connection establishment.
 * 
 * This function is called automatically by the Arduino framework
 * when the ESP32 starts up or resets.
 */
void setup() {
    app_controller = new ApplicationController();
    app_controller->initialize();
}

/**
 * @brief Arduino main loop function - called continuously
 * 
 * Executes the main application update cycle and LVGL timer handling.
 * This function runs continuously after setup() completes and handles:
 * - Application state management
 * - WebSocket connection monitoring and reconnection
 * - Hardware control updates (LEDs, buttons, sensors)
 * - Display refresh and user interface updates
 * - LVGL graphics library timer processing
 * 
 * This function is called automatically by the Arduino framework
 * in an infinite loop.
 */
void loop() {
    app_controller->update();
    lv_timer_handler();
}