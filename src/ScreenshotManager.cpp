/**
 * @file ScreenshotManager.cpp
 * @brief Screenshot capture and BMP conversion implementation
 */

#include "ScreenshotManager.h"
#include "WebSocketManager.h"
#include "constants.h"
#include <esp32_smartdisplay.h>

// Display dimensions
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320

// BMP file format constants
#define BMP_FILE_HEADER_SIZE 14
#define BMP_INFO_HEADER_SIZE 40
#define BMP_HEADER_SIZE (BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE)
#define BMP_BITS_PER_PIXEL 24
#define BMP_BYTES_PER_PIXEL 3

// Calculate sizes
#define BMP_ROW_SIZE ((DISPLAY_WIDTH * BMP_BYTES_PER_PIXEL + 3) & ~3) // Align to 4 bytes
#define BMP_PIXEL_DATA_SIZE (BMP_ROW_SIZE * DISPLAY_HEIGHT)
#define BMP_TOTAL_SIZE (BMP_HEADER_SIZE + BMP_PIXEL_DATA_SIZE)

/**
 * @brief Writes BMP file header to buffer
 *
 * Creates 14-byte BMP file header with signature, file size, and pixel data offset
 *
 * @param buffer Pointer to buffer (must have at least 14 bytes)
 * @param file_size Total BMP file size in bytes
 */
static void writeBMPFileHeader(uint8_t* buffer, uint32_t file_size) {
    buffer[0] = 'B';
    buffer[1] = 'M';
    buffer[2] = file_size & 0xFF;
    buffer[3] = (file_size >> 8) & 0xFF;
    buffer[4] = (file_size >> 16) & 0xFF;
    buffer[5] = (file_size >> 24) & 0xFF;
    buffer[6] = 0; // Reserved
    buffer[7] = 0; // Reserved
    buffer[8] = 0; // Reserved
    buffer[9] = 0; // Reserved
    buffer[10] = BMP_HEADER_SIZE & 0xFF;
    buffer[11] = (BMP_HEADER_SIZE >> 8) & 0xFF;
    buffer[12] = (BMP_HEADER_SIZE >> 16) & 0xFF;
    buffer[13] = (BMP_HEADER_SIZE >> 24) & 0xFF;
}

/**
 * @brief Writes BMP info header to buffer
 *
 * Creates 40-byte BMP info header with image dimensions and format info
 *
 * @param buffer Pointer to buffer (must have at least 40 bytes)
 */
static void writeBMPInfoHeader(uint8_t* buffer) {
    uint32_t width = DISPLAY_WIDTH;
    uint32_t height = DISPLAY_HEIGHT;
    uint16_t planes = 1;
    uint16_t bpp = BMP_BITS_PER_PIXEL;
    uint32_t compression = 0; // BI_RGB (no compression)
    uint32_t image_size = BMP_PIXEL_DATA_SIZE;

    buffer[0] = BMP_INFO_HEADER_SIZE & 0xFF;
    buffer[1] = (BMP_INFO_HEADER_SIZE >> 8) & 0xFF;
    buffer[2] = (BMP_INFO_HEADER_SIZE >> 16) & 0xFF;
    buffer[3] = (BMP_INFO_HEADER_SIZE >> 24) & 0xFF;

    buffer[4] = width & 0xFF;
    buffer[5] = (width >> 8) & 0xFF;
    buffer[6] = (width >> 16) & 0xFF;
    buffer[7] = (width >> 24) & 0xFF;

    buffer[8] = height & 0xFF;
    buffer[9] = (height >> 8) & 0xFF;
    buffer[10] = (height >> 16) & 0xFF;
    buffer[11] = (height >> 24) & 0xFF;

    buffer[12] = planes & 0xFF;
    buffer[13] = (planes >> 8) & 0xFF;

    buffer[14] = bpp & 0xFF;
    buffer[15] = (bpp >> 8) & 0xFF;

    buffer[16] = compression & 0xFF;
    buffer[17] = (compression >> 8) & 0xFF;
    buffer[18] = (compression >> 16) & 0xFF;
    buffer[19] = (compression >> 24) & 0xFF;

    buffer[20] = image_size & 0xFF;
    buffer[21] = (image_size >> 8) & 0xFF;
    buffer[22] = (image_size >> 16) & 0xFF;
    buffer[23] = (image_size >> 24) & 0xFF;

    // X pixels per meter (2835 = 72 DPI)
    buffer[24] = 0x13;
    buffer[25] = 0x0B;
    buffer[26] = 0x00;
    buffer[27] = 0x00;

    // Y pixels per meter (2835 = 72 DPI)
    buffer[28] = 0x13;
    buffer[29] = 0x0B;
    buffer[30] = 0x00;
    buffer[31] = 0x00;

    // Colors used (0 = all)
    buffer[32] = 0x00;
    buffer[33] = 0x00;
    buffer[34] = 0x00;
    buffer[35] = 0x00;

    // Important colors (0 = all)
    buffer[36] = 0x00;
    buffer[37] = 0x00;
    buffer[38] = 0x00;
    buffer[39] = 0x00;
}

/**
 * @brief Converts RGB565 pixel to RGB888 format
 *
 * Expands 16-bit RGB565 color to 24-bit RGB888:
 * - R: 5 bits -> 8 bits
 * - G: 6 bits -> 8 bits
 * - B: 5 bits -> 8 bits
 *
 * @param rgb565 Input 16-bit RGB565 pixel
 * @param r Output red component (0-255)
 * @param g Output green component (0-255)
 * @param b Output blue component (0-255)
 */
static inline void rgb565ToRgb888(uint16_t rgb565, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = ((rgb565 & 0xF800) >> 11) << 3; // 5-bit red
    g = ((rgb565 & 0x07E0) >> 5) << 2;  // 6-bit green
    b = (rgb565 & 0x001F) << 3;         // 5-bit blue

    // Expand to full 8-bit range by copying MSBs to LSBs
    r |= (r >> 5);
    g |= (g >> 6);
    b |= (b >> 5);
}

lv_draw_buf_t* captureScreenshotSnapshot() {
    LOG_INFO("Starting screenshot snapshot capture");

    // Check available memory (snapshot is ~150KB for 240x320 RGB565)
    size_t snapshot_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2; // RGB565 = 2 bytes/pixel
    size_t free_heap = ESP.getFreeHeap();
    LOG_DEBUGF("Free heap: %u bytes, snapshot needs: %u bytes", free_heap, snapshot_size);

    if (free_heap < snapshot_size + 5000) { // Minimal 5KB margin (reduced from 20KB)
        LOG_ERRORF("Insufficient memory for snapshot: %u bytes free, need %u bytes", free_heap, snapshot_size + 5000);
        return nullptr;
    }

    // Capture LVGL snapshot
    lv_obj_t* screen = lv_scr_act();
    if (!screen) {
        LOG_ERROR("Failed to get active screen");
        return nullptr;
    }

    LOG_DEBUG("Taking LVGL snapshot");
    lv_draw_buf_t* snapshot = lv_snapshot_take(screen, LV_COLOR_FORMAT_RGB565);

    if (!snapshot) {
        LOG_ERROR("LVGL snapshot failed");
        return nullptr;
    }

    LOG_INFOF("Snapshot captured successfully: %dx%d pixels", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return snapshot;
}

uint8_t* captureScreenshotBMP(size_t& out_size) {
    LOG_INFO("Starting screenshot capture");

    // Check available memory
    size_t free_heap = ESP.getFreeHeap();
    LOG_DEBUGF("Free heap: %u bytes, required: %u bytes", free_heap, BMP_TOTAL_SIZE);

    if (free_heap < BMP_TOTAL_SIZE + 10000) { // Keep 10KB margin
        LOG_ERRORF("Insufficient memory: %u bytes free, need %u bytes", free_heap, BMP_TOTAL_SIZE);
        out_size = 0;
        return nullptr;
    }

    // Capture LVGL snapshot
    lv_draw_buf_t* snapshot = captureScreenshotSnapshot();
    if (!snapshot) {
        out_size = 0;
        return nullptr;
    }

    // Allocate BMP buffer
    uint8_t* bmp_data = (uint8_t*)malloc(BMP_TOTAL_SIZE);
    if (!bmp_data) {
        LOG_ERROR("Failed to allocate BMP buffer");
        lv_draw_buf_destroy(snapshot);
        out_size = 0;
        return nullptr;
    }

    LOG_DEBUG("Converting to BMP format");

    // Write BMP headers
    writeBMPFileHeader(bmp_data, BMP_TOTAL_SIZE);
    writeBMPInfoHeader(bmp_data + BMP_FILE_HEADER_SIZE);

    // Convert pixel data (RGB565 -> RGB888)
    // BMP stores pixels bottom-to-top, left-to-right
    uint16_t* pixels = (uint16_t*)snapshot->data;
    uint8_t* bmp_pixels = bmp_data + BMP_HEADER_SIZE;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        // BMP rows are stored bottom-to-top
        int bmp_row = DISPLAY_HEIGHT - 1 - y;
        uint8_t* row_ptr = bmp_pixels + (bmp_row * BMP_ROW_SIZE);

        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            uint16_t pixel_565 = pixels[y * DISPLAY_WIDTH + x];
            uint8_t r, g, b;
            rgb565ToRgb888(pixel_565, r, g, b);

            // BMP stores pixels as BGR (not RGB)
            row_ptr[x * 3 + 0] = b;
            row_ptr[x * 3 + 1] = g;
            row_ptr[x * 3 + 2] = r;
        }

        // Pad row to 4-byte boundary (already zeroed by padding calculation)
        int padding = BMP_ROW_SIZE - (DISPLAY_WIDTH * BMP_BYTES_PER_PIXEL);
        for (int p = 0; p < padding; p++) {
            row_ptr[DISPLAY_WIDTH * 3 + p] = 0;
        }
    }

    // Clean up snapshot
    lv_draw_buf_destroy(snapshot);

    out_size = BMP_TOTAL_SIZE;
    LOG_INFOF("Screenshot captured successfully: %u bytes", out_size);

    return bmp_data;
}

/**
 * @brief Output screenshot as hex to serial console
 *
 * IMPORTANT: This requires ~150KB free RAM for lv_snapshot_take().
 * To free sufficient memory, this function disconnects WebSocket and reboots after.
 *
 * Process:
 * 1. Disconnect WebSocket completely to free ~30-40KB
 * 2. Take LVGL snapshot
 * 3. Output as hex to serial
 * 4. Reboot device
 */
void outputScreenshotToSerial(WebSocketManager* ws_manager) {
    LOG_INFO("========== SCREENSHOT START ==========");

    // Pause WebSocket completely to free maximum memory (prevents auto-reconnect)
    if (ws_manager) {
        LOG_INFO("Pausing WebSocket to free memory for screenshot");
        ws_manager->pauseForMemoryCleanup();
        delay(1000); // Give time for complete cleanup
        LOG_INFOF("Free heap after pause: %u bytes", ESP.getFreeHeap());
    }

    size_t free_heap = ESP.getFreeHeap();
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const int rows_per_chunk = 32;
    const size_t chunk_size = DISPLAY_WIDTH * rows_per_chunk * 2; // 15,360 bytes per chunk

    LOG_INFOF("Free heap: %u bytes", free_heap);
    LOG_INFOF("Largest contiguous block: %u bytes", largest_block);
    LOG_INFOF("Chunk size needed: %u bytes (%d rows)", chunk_size, rows_per_chunk);

    if (largest_block < chunk_size) {
        LOG_ERROR("Insufficient memory even for chunked screenshot");
        LOG_ERRORF("Need %u bytes for chunk, largest block is %u bytes", chunk_size, largest_block);
        LOG_INFO("========== SCREENSHOT FAILED ==========");
        LOG_INFO("Rebooting in 2 seconds...");
        delay(2000);
        ESP.restart();
    }

    // Get the active screen
    lv_obj_t* screen = lv_scr_act();
    if (!screen) {
        LOG_ERROR("Failed to get active screen");
        LOG_INFO("========== SCREENSHOT FAILED ==========");
        LOG_INFO("Rebooting in 2 seconds...");
        delay(2000);
        ESP.restart();
    }

    // Output metadata
    Serial.println("FORMAT:RGB565");
    Serial.println("WIDTH:240");
    Serial.println("HEIGHT:320");
    Serial.println("DATA:");

    LOG_INFO("Attempting memory dump approach to find framebuffer");

    // Get LVGL display
    lv_display_t* disp = lv_display_get_default();
    if (!disp) {
        LOG_ERROR("Failed to get LVGL display");
        LOG_INFO("========== SCREENSHOT FAILED ==========");
        LOG_INFO("Rebooting in 2 seconds...");
        delay(2000);
        ESP.restart();
    }

    // Try to get the draw buffer directly from LVGL display
    lv_draw_buf_t* draw_buf = lv_display_get_buf_active(disp);

    if (!draw_buf || !draw_buf->data) {
        LOG_ERROR("Could not access LVGL draw buffer");
        LOG_INFO("========== SCREENSHOT FAILED ==========");
        LOG_INFO("Rebooting in 2 seconds...");
        delay(2000);
        ESP.restart();
    }

    LOG_INFO("Found active draw buffer!");
    LOG_INFOF("Buffer address: %p", draw_buf->data);
    LOG_INFOF("Buffer size: %u bytes", draw_buf->data_size);

    size_t full_screen_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2;

    if (draw_buf->data_size >= full_screen_size) {
        // Full framebuffer - dump it directly
        LOG_INFO("Full framebuffer found! Dumping...");
        lv_obj_invalidate(screen);
        lv_refr_now(disp);

        uint8_t* pixels = (uint8_t*)draw_buf->data;

        for (size_t i = 0; i < full_screen_size; i += 32) {
            for (size_t j = 0; j < 32 && (i + j) < full_screen_size; j++) {
                if (pixels[i + j] < 16) Serial.print('0');
                Serial.print(pixels[i + j], HEX);
            }
            Serial.println();
            if ((i % 1024) == 0) yield();
        }

        LOG_INFO("Framebuffer dump completed successfully!");
    } else {
        // Partial buffer - calculate how many rows per buffer
        int buffer_height = draw_buf->data_size / (DISPLAY_WIDTH * 2);
        int num_chunks = (DISPLAY_HEIGHT + buffer_height - 1) / buffer_height;

        LOG_INFOF("Partial buffer: %d rows per chunk, %d total chunks", buffer_height, num_chunks);
        LOG_INFO("Capturing screen in multiple refresh cycles...");

        // Allocate temporary storage for captured chunks
        uint8_t* full_screen = (uint8_t*)malloc(full_screen_size);
        if (!full_screen) {
            LOG_ERROR("Failed to allocate full screen buffer");
            LOG_INFO("Trying direct streaming approach instead...");

            // Dump screen in chunks by forcing specific areas to redraw
            for (int chunk = 0; chunk < num_chunks; chunk++) {
                int chunk_start_y = chunk * buffer_height;
                int chunk_end_y = min((chunk + 1) * buffer_height, DISPLAY_HEIGHT);
                int actual_chunk_height = chunk_end_y - chunk_start_y;

                LOG_DEBUGF("Chunk %d: rows %d-%d (%d rows)", chunk, chunk_start_y, chunk_end_y - 1, actual_chunk_height);

                // Invalidate specific area to force it into the draw buffer
                lv_area_t area;
                area.x1 = 0;
                area.y1 = chunk_start_y;
                area.x2 = DISPLAY_WIDTH - 1;
                area.y2 = chunk_end_y - 1;
                lv_obj_invalidate_area(screen, &area);

                // Force immediate refresh of this specific area
                lv_refr_now(disp);
                delay(100); // Give time for render to complete

                // Now the draw buffer should contain this region
                // Dump only the actual rows needed (not the entire buffer)
                uint8_t* pixels = (uint8_t*)draw_buf->data;
                size_t chunk_bytes = DISPLAY_WIDTH * actual_chunk_height * 2;

                for (size_t i = 0; i < chunk_bytes; i += 32) {
                    for (size_t j = 0; j < 32 && (i + j) < chunk_bytes; j++) {
                        if (pixels[i + j] < 16) Serial.print('0');
                        Serial.print(pixels[i + j], HEX);
                    }
                    Serial.println();
                }
                yield();
            }

            LOG_INFO("Direct streaming completed");
        } else {
            LOG_INFO("Allocated full screen buffer, capturing chunks...");
            free(full_screen);
            LOG_ERROR("Chunked capture with buffer not implemented yet");
            LOG_INFO("========== SCREENSHOT FAILED ==========");
            LOG_INFO("Rebooting in 2 seconds...");
            delay(2000);
            ESP.restart();
        }
    }

    LOG_INFO("========== SCREENSHOT END ==========");
    LOG_INFOF("Free heap after: %u bytes", ESP.getFreeHeap());
    LOG_INFO("To decode: python extract_screenshot.py screenshot.log");
    LOG_INFO("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
}
