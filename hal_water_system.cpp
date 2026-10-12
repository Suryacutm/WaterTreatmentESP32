#include "hal_water_system.h"
#include <Arduino.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"

/*  Pin Mapping for ESP32-S2 */
#define PIN_PUMP_WASTE_5000L    GPIO_NUM_4
#define PIN_PUMP_NORM_COAG      GPIO_NUM_5
#define PIN_PUMP_NORM_FLOC      GPIO_NUM_6
#define PIN_PUMP_WASTE_MIX      GPIO_NUM_7
#define PIN_PUMP_DC             GPIO_NUM_8

#define PIN_ACT_POWDER_DOSER    GPIO_NUM_9
#define PIN_ACT_MIXER           GPIO_NUM_10

#define PIN_STEPPER_STEP        GPIO_NUM_11
#define PIN_STEPPER_DIR         GPIO_NUM_12
#define PIN_STEPPER_EN          GPIO_NUM_13

#define PIN_SENS_5000L_HIGH     GPIO_NUM_14
#define PIN_SENS_5000L_LOW      GPIO_NUM_15
#define PIN_SENS_MIXING_HIGH    GPIO_NUM_16

static const gpio_num_t pump_pins[PUMP_MAX_COUNT] = {
    PIN_PUMP_WASTE_5000L, PIN_PUMP_NORM_COAG, PIN_PUMP_NORM_FLOC,
    PIN_PUMP_WASTE_MIX,   PIN_PUMP_DC
};

static const gpio_num_t actuator_pins[ACTUATOR_MAX_COUNT] = {
    PIN_ACT_POWDER_DOSER, PIN_ACT_MIXER
};

static const gpio_num_t sensor_pins[SENSOR_MAX_COUNT] = {
    PIN_SENS_5000L_HIGH, PIN_SENS_5000L_LOW, PIN_SENS_MIXING_HIGH
};

esp_err_t hal_gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    for (int i = 0; i < PUMP_MAX_COUNT; i++) {
        io_conf.pin_bit_mask |= (1ULL << pump_pins[i]);
    }
    for (int i = 0; i < ACTUATOR_MAX_COUNT; i++) {
        io_conf.pin_bit_mask |= (1ULL << actuator_pins[i]);
    }
    io_conf.pin_bit_mask |= (1ULL << PIN_STEPPER_DIR) | (1ULL << PIN_STEPPER_EN);
    gpio_config(&io_conf);

    
    for (int i = 0; i < PUMP_MAX_COUNT; i++) gpio_set_level(pump_pins[i], 0);
    for (int i = 0; i < ACTUATOR_MAX_COUNT; i++) gpio_set_level(actuator_pins[i], 0);
    gpio_set_level(PIN_STEPPER_EN, 1); 

    
    gpio_config_t in_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    for (int i = 0; i < SENSOR_MAX_COUNT; i++) {
        in_conf.pin_bit_mask |= (1ULL << sensor_pins[i]);
    }
    gpio_config(&in_conf);

    return ESP_OK;
}

esp_err_t hal_pump_set(hal_pump_t pump, bool turn_on) {
    if (pump >= PUMP_MAX_COUNT) return ESP_ERR_INVALID_ARG;
    gpio_set_level(pump_pins[pump], turn_on ? 1 : 0);
    return ESP_OK;
}

esp_err_t hal_actuator_set(hal_actuator_t actuator, bool turn_on) {
    if (actuator >= ACTUATOR_MAX_COUNT) return ESP_ERR_INVALID_ARG;
    gpio_set_level(actuator_pins[actuator], turn_on ? 1 : 0);
    return ESP_OK;
}

esp_err_t hal_stepper_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 4267, 
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PIN_STEPPER_STEP,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    return ESP_OK;
}

esp_err_t hal_stepper_start(uint32_t rpm) {
    // 80 RPM with 200 step motor and 1/16 microsteps = (80 * 200 * 16) / 60 (formaula)
    uint32_t freq_hz = (rpm * 200 * 16) / 60;
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    gpio_set_level(PIN_STEPPER_EN, 0); 
    gpio_set_level(PIN_STEPPER_DIR, 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    return ESP_OK;
}

esp_err_t hal_stepper_stop(void) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    gpio_set_level(PIN_STEPPER_EN, 1); 
    return ESP_OK;
}

bool hal_sensor_read(hal_sensor_t sensor) {
    if (sensor >= SENSOR_MAX_COUNT) return false;
    return (gpio_get_level(sensor_pins[sensor]) == 0); 
}

uint32_t hal_get_time_sec(void) {
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}