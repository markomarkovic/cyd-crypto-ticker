#include "HardwareController.h"
#include <esp32_smartdisplay.h>

HardwareController::HardwareController(int red_pin, int green_pin, int blue_pin, int light_sensor_pin)
    : led_red_pin_(red_pin), led_green_pin_(green_pin), led_blue_pin_(blue_pin), 
      light_sensor_pin_(light_sensor_pin), led_last_blink_(0), led_blink_state_(false),
      brightness_last_update_(0), current_brightness_float_(0.5f),
      button_pressed_(false), button_was_pressed_(false), button_press_start_(0), 
      reconfiguration_requested_(false) {
}

void HardwareController::initialize() {
    // Initialize RGB LED pins
    pinMode(led_red_pin_, OUTPUT);
    pinMode(led_green_pin_, OUTPUT);
    pinMode(led_blue_pin_, OUTPUT);
    setLED(false, false, false); // Turn off initially
    
    // Initialize adaptive brightness control
    pinMode(light_sensor_pin_, INPUT);
    analogSetAttenuation(ADC_0db);
    
    // Initialize boot button (active LOW with internal pullup)
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    
    Serial.println("Hardware controller initialized");
}

void HardwareController::setLED(bool red, bool green, bool blue) {
    digitalWrite(led_red_pin_, red ? LOW : HIGH);    // LEDs are active LOW
    digitalWrite(led_green_pin_, green ? LOW : HIGH);
    digitalWrite(led_blue_pin_, blue ? LOW : HIGH);
}

void HardwareController::updateLEDStatus(int coins_up, int coins_down, bool has_error, bool data_stale) {
    unsigned long now = millis();
    
    if (has_error || data_stale) {
        // Blink yellow for errors or stale data (500ms interval)
        if (now - led_last_blink_ > 500) {
            led_blink_state_ = !led_blink_state_;
            led_last_blink_ = now;
        }
        setLED(led_blink_state_, led_blink_state_, false); // Yellow = red + green
        return;
    }
    
    // Set LED color based on majority
    if (coins_up > coins_down) {
        setLED(false, true, false);  // Green
    } else if (coins_down > coins_up) {
        setLED(true, false, false);  // Red
    } else {
        setLED(false, false, false); // Off (equal or no valid data)
    }
}

void HardwareController::updateAdaptiveBrightness() {
    unsigned long now = millis();
    
    // Update every 1 second
    if (now - brightness_last_update_ > 1000) {
        int lightLevel = readLightSensor();
        
        // Map light sensor reading to brightness (0.0 to 1.0)
        // Actual observed range: ~75mV (bright) to ~214mV (dark/covered)
        lightLevel = constrain(lightLevel, 75, 220);
        
        // Invert mapping: lower voltage = brighter environment = higher backlight
        float normalized = (float)(220 - lightLevel) / (220 - 75);  // 0.0 (dark) to 1.0 (bright)
        
        // Apply gamma correction for better perceived brightness
        float gamma = 2.2;
        float corrected = pow(normalized, 1.0/gamma);
        
        // Map to brightness range: 0.05 (5% minimum) to 1.0 (100% maximum)
        float newBrightness = 0.05f + corrected * 0.95f;  // 5% to 100%
        
        // Only update if brightness changed significantly (>1%)
        if (abs(newBrightness - current_brightness_float_) > 0.01f) {
            current_brightness_float_ = newBrightness;
            smartdisplay_lcd_set_backlight(current_brightness_float_);
        }
        
        brightness_last_update_ = now;
    }
}

int HardwareController::readLightSensor() const {
    return analogReadMilliVolts(light_sensor_pin_);
}

void HardwareController::blinkLED() {
    unsigned long now = millis();
    if (now - led_last_blink_ > 500) {
        led_blink_state_ = !led_blink_state_;
        led_last_blink_ = now;
    }
}

void HardwareController::updateButtonStatus() {
    unsigned long now = millis();
    bool current_button_state = digitalRead(BOOT_BUTTON_PIN) == LOW; // Active LOW
    
    // Button press detection
    if (current_button_state && !button_was_pressed_) {
        // Button just pressed
        button_pressed_ = true;
        button_press_start_ = now;
        button_was_pressed_ = true;
        
        Serial.println("Boot button pressed - hold for 5 seconds to request reconfiguration");
    }
    else if (!current_button_state && button_was_pressed_) {
        // Button just released
        button_pressed_ = false;
        button_was_pressed_ = false;
        
        unsigned long press_duration = now - button_press_start_;
        Serial.print("Boot button released after ");
        Serial.print(press_duration);
        Serial.println("ms");
        
        if (press_duration < RECONFIGURATION_HOLD_TIME) {
            Serial.println("Button not held long enough to request reconfiguration");
        }
    }
    else if (current_button_state && button_pressed_) {
        // Button is being held
        unsigned long press_duration = now - button_press_start_;
        
        if (press_duration >= RECONFIGURATION_HOLD_TIME && !reconfiguration_requested_) {
            // Button held for required time - set flag but don't repeat
            reconfiguration_requested_ = true;
            Serial.println("Reconfiguration requested!");
            
            // Visual feedback - blink blue LED rapidly
            setLED(false, false, true);
            delay(100);
            setLED(false, false, false);
            delay(100);
            setLED(false, false, true);
            delay(100);
            setLED(false, false, false);
        }
    }
}

bool HardwareController::isReconfigurationRequested() const {
    return reconfiguration_requested_;
}

void HardwareController::clearReconfigurationRequest() {
    reconfiguration_requested_ = false;
    button_pressed_ = false;
    button_was_pressed_ = false;
}

unsigned long HardwareController::getButtonPressTime() const {
    if (button_pressed_) {
        return millis() - button_press_start_;
    }
    return 0;
}