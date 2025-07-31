/**
 * @file ApplicationController.h
 * @brief Main application controller for the CYD Crypto Ticker project
 * @author Claude Code Assistant
 * @date 2024
 * 
 * This file contains the main ApplicationController class that orchestrates
 * the entire cryptocurrency display application. It manages the lifecycle,
 * state transitions, and coordinates all subsystem managers.
 */

#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include <Arduino.h>
#include "constants.h"
#include "NetworkManager.h"
#include "BinanceDataManager.h"
#include "DisplayManager.h"
#include "HardwareController.h"
#include "ApplicationStateManager.h"
#include "StatusCalculator.h"
#include "WebSocketManager.h"

/**
 * @class ApplicationController
 * @brief Main application controller that manages the entire cryptocurrency ticker system
 * 
 * The ApplicationController is the central orchestrator of the CYD Crypto Ticker application.
 * It manages the application lifecycle, coordinates between different managers (Network, Display,
 * Hardware, etc.), handles state transitions, and processes real-time WebSocket data updates.
 * 
 * Key responsibilities:
 * - Application initialization and setup
 * - WiFi connection management and recovery
 * - WebSocket connection setup and monitoring  
 * - Display updates and status management
 * - Hardware control (LED indicators, buttons)
 * - Error handling and recovery mechanisms
 * 
 * @see NetworkManager
 * @see BinanceDataManager
 * @see DisplayManager
 * @see HardwareController
 * @see ApplicationStateManager
 * @see WebSocketManager
 */
class ApplicationController {
public:
    /**
     * @brief Constructor for ApplicationController
     * 
     * Initializes the ApplicationController instance and sets up initial state.
     * All manager instances are initialized to nullptr and will be created
     * during the initialize() call.
     */
    ApplicationController();
    
    /**
     * @brief Destructor for ApplicationController
     * 
     * Cleans up all allocated manager instances and releases resources.
     * Ensures proper shutdown of WebSocket connections and hardware controls.
     */
    ~ApplicationController();
    
    /**
     * @brief Initialize the entire application system
     * 
     * Sets up all manager instances, initializes hardware peripherals,
     * attempts WiFi connection, and prepares the system for operation.
     * This method must be called once during system startup.
     * 
     * @note This method handles initial WiFi connection attempts and
     *       will start AP mode if no stored credentials are found.
     */
    void initialize();
    
    /**
     * @brief Main application update loop
     * 
     * Processes the current application state and handles all ongoing
     * operations including WebSocket reconnection, hardware monitoring,
     * and display updates. This method should be called continuously
     * from the main loop.
     * 
     * @note This method is non-blocking and handles different application
     *       states (AP mode, normal operation, reconnection, etc.)
     */
    void update();
    
private:
    // Initialization methods
    /**
     * @brief Initialize all manager instances
     * 
     * Creates and initializes all subsystem managers (Network, Display,
     * Hardware, State, Crypto, WebSocket). Sets up initial configurations
     * and establishes communication between managers.
     */
    void initializeManagers();
    
    /**
     * @brief Initialize hardware peripherals
     * 
     * Sets up RGB LEDs, light sensor, display backlight, and other
     * hardware components. Configures GPIO pins and initial states.
     */
    void initializeHardware();
    
    /**
     * @brief Attempt to connect to WiFi using stored credentials
     * 
     * Tries to connect to WiFi using previously saved SSID and password.
     * Handles connection timeout and failure scenarios.
     * 
     * @return true if WiFi connection successful, false otherwise
     */
    bool attemptWiFiConnection();
    
    /**
     * @brief Start Access Point mode with network scanning
     * 
     * Activates the device as a WiFi access point and starts the web
     * configuration interface. Scans for available networks to present
     * to the user for easier configuration.
     */
    void startAPModeWithScan();
    
    /**
     * @brief Perform initial system setup after successful WiFi connection
     * 
     * Sets up WebSocket connection, initializes display with crypto data,
     * and transitions the system to normal operation mode.
     */
    void performInitialSetup();
    
    // State handling methods
    /**
     * @brief Handle Access Point mode operations
     * 
     * Processes web configuration requests, monitors for new WiFi credentials,
     * and handles the transition from AP mode to normal operation when
     * valid credentials are provided.
     */
    void handleAPMode();
    
    /**
     * @brief Handle button presses while in AP mode
     * 
     * Monitors BOOT button for factory reset requests and other
     * configuration-related button interactions during AP mode.
     */
    void handleAPModeButtons();
    
    /**
     * @brief Handle normal operation mode
     * 
     * Manages WebSocket connections, processes real-time data updates,
     * handles display refresh, and monitors for connection issues.
     * This is the primary operational state of the application.
     */
    void handleNormalOperation();
    
    /**
     * @brief Handle WiFi reconnection scenarios
     * 
     * Manages WiFi disconnection detection, automatic reconnection attempts,
     * and user notification during network connectivity issues.
     */
    void handleWiFiReconnection();
    
    // Update methods
    /**
     * @brief Update RGB LED status indicators
     * 
     * Updates LED colors and patterns based on connection status,
     * cryptocurrency performance, and system state. Handles different
     * LED modes including connection status and data indicators.
     */
    void updateLEDStatus();
    
    /**
     * @brief Update hardware control monitoring
     * 
     * Monitors BOOT button presses, updates display brightness based
     * on ambient light sensor, and handles other hardware control tasks.
     */
    void updateHardwareControls();
    
    /**
     * @brief Handle touch input events for screen navigation
     * 
     * Processes LVGL touch events and coordinates screen transitions
     * between cryptocurrency list and chart detail views.
     */
    void handleTouchEvents();
    
    /**
     * @brief Fetch candlestick data for the selected cryptocurrency
     * 
     * Retrieves chart data from Binance API for the currently selected
     * cryptocurrency and updates the detail screen display.
     */
    void fetchCandlestickDataForSelectedCoin();
    
    /**
     * @brief Display system performance statistics
     * 
     * Shows CPU usage, free/used memory stats when log level is DEBUG or lower.
     * Called periodically to monitor system resource usage.
     */
    void displaySystemStats();
    
    
    // WiFi connection methods
    /**
     * @brief Connect to WiFi using newly provided credentials
     * 
     * Attempts connection using credentials provided through the web
     * configuration interface. Updates stored credentials on success.
     * 
     * @return true if connection successful, false otherwise
     */
    bool connectWithNewCredentials();
    
    /**
     * @brief Attempt silent WiFi reconnection in background
     * 
     * Tries to reconnect to WiFi without disrupting the user interface.
     * Used for automatic recovery from temporary network issues.
     * 
     * @return true if reconnection successful, false otherwise
     */
    bool attemptSilentReconnection();
    
    /**
     * @brief Display reconnection message to user
     * 
     * Shows user-friendly reconnection status and instructions on the
     * display when WiFi connectivity issues persist beyond the timeout.
     */
    void showReconnectionMessage();
    
    // Display update methods
    /**
     * @brief Show initial setup completion status
     * 
     * Updates the display to show that initial setup and WebSocket
     * connection have been completed successfully. Transitions display
     * to show real-time cryptocurrency data.
     */
    void showInitialSetupComplete();
    
    // WebSocket methods
    /**
     * @brief Set up WebSocket connection to Binance API
     * 
     * Initializes WebSocket connection to Binance for real-time
     * cryptocurrency price updates. Subscribes to specified trading pairs.
     * 
     * @param symbols Comma-separated string of cryptocurrency symbols to monitor
     *                (e.g., "BTCUSDT,ETHUSDT,SOLUSDT")
     */
    void setupWebSocketConnection(const String& symbols);
    
    // Manager instances
    /**
     * @brief Network and WiFi management instance
     * 
     * Handles WiFi connections, AP mode, web configuration interface,
     * and network-related operations.
     */
    NetworkManager* network_manager_;
    
    /**
     * @brief Cryptocurrency data management instance
     * 
     * Manages real-time cryptocurrency price data, symbol parsing,
     * and data formatting for display.
     */
    BinanceDataManager* crypto_manager_;
    
    /**
     * @brief Display and user interface management instance
     * 
     * Handles LVGL-based display operations, UI updates, and visual
     * presentation of cryptocurrency data.
     */
    DisplayManager* display_manager_;
    
    /**
     * @brief Hardware peripheral control instance
     * 
     * Manages RGB LEDs, light sensor, BOOT button, and other hardware
     * peripherals for status indication and user interaction.
     */
    HardwareController* hardware_controller_;
    
    /**
     * @brief Application state management instance
     * 
     * Tracks application state, manages state transitions, and handles
     * WiFi disconnection monitoring.
     */
    ApplicationStateManager* state_manager_;
    
    /**
     * @brief WebSocket connection management instance
     * 
     * Handles real-time WebSocket connections to Binance API,
     * automatic reconnection, and data stream processing.
     */
    WebSocketManager* websocket_manager_;
};

#endif // APPLICATION_CONTROLLER_H