/**
 * @file ScreenshotManager.h
 * @brief Screenshot capture and BMP conversion for LVGL displays
 *
 * Provides functionality to capture the current LVGL screen and convert it to
 * BMP format suitable for HTTP transfer and browser display.
 */

#ifndef SCREENSHOT_MANAGER_H
#define SCREENSHOT_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

/**
 * @brief Captures current LVGL screen and converts to BMP format
 *
 * This function:
 * 1. Takes a snapshot of the active LVGL screen using lv_snapshot_take()
 * 2. Converts RGB565 pixels to RGB888 (24-bit BMP format)
 * 3. Adds BMP file header and info header
 * 4. Returns dynamically allocated buffer containing complete BMP file
 *
 * Memory considerations:
 * - Allocates ~225KB for 240x320 BMP image
 * - Caller MUST free returned buffer after use
 * - Checks available heap before allocation
 *
 * @param out_size Reference to store the size of returned BMP data
 * @return uint8_t* Pointer to BMP data (caller must free), or nullptr on failure
 *
 * @note Returns nullptr if:
 *       - Insufficient memory available
 *       - LVGL snapshot fails
 *       - Memory allocation fails
 */
uint8_t* captureScreenshotBMP(size_t& out_size);

/**
 * @brief Captures LVGL snapshot for chunked streaming (memory efficient)
 *
 * Takes LVGL snapshot and stores it for chunked BMP generation.
 * Much more memory efficient than captureScreenshotBMP() as it only
 * stores the RGB565 snapshot (~150KB) instead of full BMP (~230KB).
 *
 * @return lv_draw_buf_t* LVGL snapshot buffer (caller must destroy with lv_draw_buf_destroy)
 */
lv_draw_buf_t* captureScreenshotSnapshot();

// Forward declaration
class WebSocketManager;

/**
 * @brief Output screenshot as hex to serial console
 *
 * Captures screenshot and outputs it line-by-line as hex to serial.
 * Temporarily disconnects WebSocket to free memory for snapshot.
 * Output can be captured from serial monitor and decoded.
 *
 * @param ws_manager WebSocket manager to disconnect/reconnect (optional)
 */
void outputScreenshotToSerial(WebSocketManager* ws_manager = nullptr);

#endif // SCREENSHOT_MANAGER_H
