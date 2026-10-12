#ifndef HAL_WATER_SYSTEM_H
#define HAL_WATER_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PUMP_WASTE_TO_5000L = 0,
    PUMP_NORMAL_TO_COAGULATION,
    PUMP_NORMAL_TO_FLOCCULANT,
    PUMP_WASTE_TO_MIXING,
    PUMP_DC_DOSING,
    PUMP_MAX_COUNT
} hal_pump_t;

typedef enum {
    ACTUATOR_POWDER_DOSER = 0,
    ACTUATOR_MIXER_MOTOR,
    ACTUATOR_MAX_COUNT
} hal_actuator_t;

typedef enum {
    SENSOR_TANK_5000L_HIGH = 0,
    SENSOR_TANK_5000L_LOW,
    SENSOR_MIXING_TANK_HIGH,
    SENSOR_MAX_COUNT
} hal_sensor_t;

esp_err_t hal_gpio_init(void);
esp_err_t hal_pump_set(hal_pump_t pump, bool turn_on);
esp_err_t hal_actuator_set(hal_actuator_t actuator, bool turn_on);

esp_err_t hal_stepper_init(void);
esp_err_t hal_stepper_start(uint32_t rpm);
esp_err_t hal_stepper_stop(void);

bool hal_sensor_read(hal_sensor_t sensor);
uint32_t hal_get_time_sec(void);

#ifdef __cplusplus
}
#endif

#endif 