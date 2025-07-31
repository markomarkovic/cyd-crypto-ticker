/**
 * @file constants.h
 * @brief Global constants and configuration definitions for CYD Crypto Ticker
 * @author Claude Code Assistant
 * @date 2024
 * 
 * This file contains all global constants, color definitions, hardware pin
 * assignments, timing intervals, and logging system configuration used
 * throughout the CYD Crypto Ticker application.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <lvgl.h>

/**
 * @defgroup Colors Color Definitions
 * @brief Color constants for LVGL display theming
 * @{
 */

/** @brief Primary dark background color for main display */
#define COLOR_DARK_BG lv_color_hex(0x1a1a2e)
/** @brief Subtle green background tint for positive price changes */        
#define COLOR_TWILIGHT_GREEN lv_color_hex(0x0d4f2d)
/** @brief Subtle red background tint for negative price changes */ 
#define COLOR_TWILIGHT_RED lv_color_hex(0x4f0d1a)
/** @brief Bright green for positive price change indicators */   
#define COLOR_BRIGHT_GREEN lv_color_hex(0x0be881)
/** @brief Bright red for negative price change indicators */   
#define COLOR_BRIGHT_RED lv_color_hex(0xee5a52)
/** @brief Primary white text color for prices and important info */     
#define COLOR_WHITE_TEXT lv_color_hex(0xffffff)
/** @brief Secondary gray text color for labels and less important info */     
#define COLOR_GREY_TEXT lv_color_hex(0xa0a0a0)

/** @brief Muted white for prices when WebSocket is disconnected */
#define COLOR_MUTED_WHITE lv_color_hex(0x666666)
/** @brief Muted green for positive changes when WebSocket is disconnected */
#define COLOR_MUTED_GREEN lv_color_hex(0x4a6741)
/** @brief Muted red for negative changes when WebSocket is disconnected */
#define COLOR_MUTED_RED lv_color_hex(0x6b4444)
/** @brief Muted grey for text when WebSocket is disconnected */
#define COLOR_MUTED_GREY lv_color_hex(0x505050)

/** @} */ // end of Colors group      

/**
 * @defgroup Hardware Hardware Pin Definitions
 * @brief GPIO pin assignments for hardware peripherals
 * @{
 */

/** @brief GPIO pin for RGB LED red channel */
#define LED_RED_PIN 4
/** @brief GPIO pin for RGB LED green channel */
#define LED_GREEN_PIN 16
/** @brief GPIO pin for RGB LED blue channel */
#define LED_BLUE_PIN 17
/** @brief ADC pin for CdS light sensor (for automatic brightness) */
#define LIGHT_SENSOR_PIN 34

/** @} */ // end of Hardware group

/**
 * @defgroup Timing Timing Intervals
 * @brief Time interval constants in milliseconds
 * @{
 */

/** @brief Interval for updating display brightness based on light sensor (1 second) */
#define BRIGHTNESS_UPDATE_INTERVAL 1000UL
/** @brief WiFi reconnection retry interval (10 seconds) */
#define RECONNECTION_RETRY_INTERVAL_MS 10000UL
/** @brief Maximum time to wait for WiFi reconnection before showing message (1 minute) */
#define RECONNECTION_TIMEOUT_MS 60000UL
/** @brief Timeout for individual WiFi connection attempts (5 seconds) */
#define RECONNECTION_ATTEMPT_TIMEOUT_MS 5000UL

/** @} */ // end of Timing group

/**
 * @defgroup Logging Logging System Configuration
 * @brief Configuration constants for the standardized logging system
 * @{
 */

/** @brief Global flag to enable/disable all logging output */
#define ENABLE_LOGGING true
/** @brief Serial communication timeout to prevent blocking when no monitor connected */
#define SERIAL_TIMEOUT_MS 10

/** @brief Trace level - most verbose, detailed execution flow and raw data */
#define LOG_LEVEL_TRACE 0
/** @brief Debug level - function entry/exit, data processing details */
#define LOG_LEVEL_DEBUG 1
/** @brief Info level - connection status, major state changes */
#define LOG_LEVEL_INFO  2
/** @brief Warning level - retry attempts, temporary failures */
#define LOG_LEVEL_WARN  3
/** @brief Error level - connection failures, API errors */
#define LOG_LEVEL_ERROR 4
/** @brief Fatal level - system crashes, unrecoverable errors */
#define LOG_LEVEL_FATAL 5

/** @brief Current active log level - only messages at this level or higher will be output */
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO

/**
 * @brief Safe logging macro for TRACE level messages
 * 
 * Outputs trace-level messages with automatic timeout protection.
 * Only outputs if logging is enabled, current log level permits, and Serial is available.
 * 
 * @param x Message to log
 */
#define LOG_TRACE(x) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_TRACE && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[TRACE] "); Serial.println(x); } } while(0)

/**
 * @brief Safe logging macro for DEBUG level messages
 * 
 * @param x Message to log
 */
#define LOG_DEBUG(x) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[DEBUG] "); Serial.println(x); } } while(0)

/** @brief Safe logging macro for INFO level messages */
#define LOG_INFO(x) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[INFO] "); Serial.println(x); } } while(0)

/** @brief Safe logging macro for WARNING level messages */
#define LOG_WARN(x) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_WARN && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[WARN] "); Serial.println(x); } } while(0)

/** @brief Safe logging macro for ERROR level messages */
#define LOG_ERROR(x) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_ERROR && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[ERROR] "); Serial.println(x); } } while(0)

/** @brief Safe logging macro for FATAL level messages */
#define LOG_FATAL(x) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_FATAL && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[FATAL] "); Serial.println(x); } } while(0)

#define LOG_TRACEF(format, ...) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_TRACE && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[TRACE] "); Serial.printf(format, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_DEBUGF(format, ...) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[DEBUG] "); Serial.printf(format, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_INFOF(format, ...) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[INFO] "); Serial.printf(format, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_WARNF(format, ...) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_WARN && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[WARN] "); Serial.printf(format, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_ERRORF(format, ...) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_ERROR && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[ERROR] "); Serial.printf(format, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_FATALF(format, ...) do { if (ENABLE_LOGGING && CURRENT_LOG_LEVEL <= LOG_LEVEL_FATAL && Serial) { Serial.setTimeout(SERIAL_TIMEOUT_MS); Serial.print("[FATAL] "); Serial.printf(format, ##__VA_ARGS__); Serial.println(); } } while(0)

/**
 * @deprecated Use LOG_DEBUG instead
 * @brief Legacy macro for backward compatibility - mapped to LOG_DEBUG
 */
#define SAFE_SERIAL_PRINT(x) LOG_DEBUG(x)

/**
 * @deprecated Use LOG_DEBUG instead  
 * @brief Legacy macro for backward compatibility - mapped to LOG_DEBUG
 */
#define SAFE_SERIAL_PRINTLN(x) LOG_DEBUG(x)

/**
 * @deprecated Use LOG_DEBUGF instead
 * @brief Legacy macro for backward compatibility - mapped to LOG_DEBUGF
 */
#define SAFE_SERIAL_PRINTF(format, ...) LOG_DEBUGF(format, ##__VA_ARGS__)

/** @} */ // end of Logging group

/**
 * @defgroup WebSocket WebSocket Configuration
 * @brief Configuration constants for Binance WebSocket connection management
 * @{
 */

/** @brief Initial reconnection interval for WebSocket (5 seconds) */
#define WEBSOCKET_RECONNECT_INTERVAL 5000UL
/** @brief Maximum number of consecutive reconnection attempts */
#define WEBSOCKET_MAX_RETRY_ATTEMPTS 10
/** @brief Timeout for detecting stale WebSocket connections (1 minute) */
#define WEBSOCKET_MESSAGE_TIMEOUT 60000UL
/** @brief Interval for WebSocket connection health checks (30 seconds) */
#define WEBSOCKET_HEARTBEAT_INTERVAL 30000UL

/** @} */ // end of WebSocket group

#endif // CONSTANTS_H