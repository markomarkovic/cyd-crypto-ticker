#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h>

class HardwareController {
public:
    HardwareController(int red_pin, int green_pin, int blue_pin, int light_sensor_pin);
    ~HardwareController() = default;
    
    void initialize();
    void setLED(bool red, bool green, bool blue);
    void updateLEDStatus(int coins_up, int coins_down, bool has_error, bool data_stale);
    
    void updateAdaptiveBrightness();
    int readLightSensor() const;
    
    // Button functionality
    void updateButtonStatus();
    bool isReconfigurationRequested() const;
    void clearReconfigurationRequest();
    unsigned long getButtonPressTime() const;
    
private:
    int led_red_pin_;
    int led_green_pin_;
    int led_blue_pin_;
    int light_sensor_pin_;
    
    unsigned long led_last_blink_;
    bool led_blink_state_;
    
    unsigned long brightness_last_update_;
    float current_brightness_float_;
    
    // Button state variables
    static const int BOOT_BUTTON_PIN = 0;
    bool button_pressed_;
    bool button_was_pressed_;
    unsigned long button_press_start_;
    bool reconfiguration_requested_;
    static const unsigned long RECONFIGURATION_HOLD_TIME = 5000; // 5 seconds
    
    void blinkLED();
};

#endif // HARDWARE_CONTROLLER_H