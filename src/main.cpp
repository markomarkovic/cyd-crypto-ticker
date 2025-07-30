#include <Arduino.h>
#include <lvgl.h>
#include "ApplicationController.h"

// Global application controller
ApplicationController* app_controller;

void setup() {
    app_controller = new ApplicationController();
    app_controller->initialize();
}

void loop() {
    app_controller->update();
    lv_timer_handler();
}