#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <lvgl.h>

// Color definitions
#define COLOR_DARK_BG lv_color_hex(0x1a1a2e)        
#define COLOR_TWILIGHT_GREEN lv_color_hex(0x0d4f2d) 
#define COLOR_TWILIGHT_RED lv_color_hex(0x4f0d1a)   
#define COLOR_BRIGHT_GREEN lv_color_hex(0x0be881)   
#define COLOR_BRIGHT_RED lv_color_hex(0xee5a52)     
#define COLOR_WHITE_TEXT lv_color_hex(0xffffff)     
#define COLOR_GREY_TEXT lv_color_hex(0xa0a0a0)      

// Hardware pin definitions
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17
#define LIGHT_SENSOR_PIN 34

// Timing intervals (in milliseconds)
#define WIFI_UPDATE_INTERVAL 10000UL        // 10 seconds
#define CRYPTO_UPDATE_INTERVAL 60000UL      // 60 seconds
#define BRIGHTNESS_UPDATE_INTERVAL 1000UL   // 1 second
#define DATA_STALE_THRESHOLD 300000UL       // 5 minutes
#define RECONNECTION_RETRY_INTERVAL_MS 10000UL      // 10 seconds
#define RECONNECTION_TIMEOUT_MS 60000UL             // 1 minute
#define RECONNECTION_ATTEMPT_TIMEOUT_MS 5000UL      // 5 seconds

#endif // CONSTANTS_H