#include "water_treatment_api.h"
#include <Arduino.h>

void api_init(void) {
    hal_gpio_init();
    hal_stepper_init();
}

void api_emergency_stop_all(void) {
    Serial.println("[EMERGENCY] Stopping all pumps, actuators, and motors!");
    for (int i = 0; i < PUMP_MAX_COUNT; i++) {
        hal_pump_set((hal_pump_t)i, false);
    }
    for (int i = 0; i < ACTUATOR_MAX_COUNT; i++) {
        hal_actuator_set((hal_actuator_t)i, false);
    }
    hal_stepper_stop();
}