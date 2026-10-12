#include <Arduino.h>
#include "hal_water_system.h"
#include "water_treatment_api.h"

/* Step Wrapper Functions */
static void step1_start(void) { hal_pump_set(PUMP_WASTE_TO_5000L, true); }
static void step1_stop(void)  { hal_pump_set(PUMP_WASTE_TO_5000L, false); }

static void step2_start(void) { hal_pump_set(PUMP_NORMAL_TO_COAGULATION, true); }
static void step2_stop(void)  { hal_pump_set(PUMP_NORMAL_TO_COAGULATION, false); }

static void step3_start(void) { hal_pump_set(PUMP_NORMAL_TO_FLOCCULANT, true); }
static void step3_stop(void)  { hal_pump_set(PUMP_NORMAL_TO_FLOCCULANT, false); }

static void step4_start(void) { hal_pump_set(PUMP_WASTE_TO_MIXING, true); }
static void step4_stop(void)  { hal_pump_set(PUMP_WASTE_TO_MIXING, false); }

static void step5_start(void) { hal_actuator_set(ACTUATOR_POWDER_DOSER, true); }
static void step5_stop(void)  { hal_actuator_set(ACTUATOR_POWDER_DOSER, false); }

static void step6_start(void) { hal_stepper_start(80); }
static void step6_stop(void)  { hal_stepper_stop(); }

static void step7_start(void) { hal_pump_set(PUMP_DC_DOSING, true); }
static void step7_stop(void)  { hal_pump_set(PUMP_DC_DOSING, false); }

/*
 * Process Recipe Table matching your chart:
 * 1. Waste Water to 5000L: 12 min = 720 sec, delay = 1 sec
 * 2. Normal Water to Coagulation: 4.4 min = 264 sec, delay = 1 sec
 * 3. Normal Water to Flocculant: 58 sec, delay = 1 sec
 * 4. Waste Water to Mixing: 4.4 min = 264 sec, delay = 1 sec
 * 5. Powder Dosing: 1 min = 60 sec, delay = 1 sec
 * 6. Coagulant Mixing (Stepper 80 RPM): 10 min = 600 sec, delay = 3 sec
 * 7. DC Pump Start: 1 min = 60 sec, delay = 1 sec
 */
static const treatment_step_t process_recipe[] = {
    { "1. Waste Water Pumping to 5000 Tank",        720, 1, step1_start, step1_stop },
    { "2. Normal Water to Coagulation Tank",        264, 1, step2_start, step2_stop },
    { "3. Normal Water to Flocculant Tank",          58, 1, step3_start, step3_stop },
    { "4. Waste Water to Mixing Water Tank",        264, 1, step4_start, step4_stop },
    { "5. Powder Dosing Start",                      60, 1, step5_start, step5_stop },
    { "6. Coagulant Mixing 1 - Stepper Motor (80RPM)", 600, 3, step6_start, step6_stop },
    { "7. DC Pump Start",                            60, 1, step7_start, step7_stop }
};

#define TOTAL_STEPS (sizeof(process_recipe) / sizeof(process_recipe[0]))

/* FreeRTOS Sequencer Task */
void waterTreatmentSequencerTask(void *pvParameters) {
    Serial.println(">>> Water Treatment Automation Task Started");

    while (1) {
        for (size_t i = 0; i < TOTAL_STEPS; i++) {
            const treatment_step_t *step = &process_recipe[i];
            Serial.printf("\n[STEP %u/%u] STARTING: %s (Duration: %lu s)\n",
                          i + 1, TOTAL_STEPS, step->step_name, step->duration_sec);

            step->action_start();

                        uint32_t elapsed = 0;
            while (elapsed < step->duration_sec) {
                                if (hal_sensor_read(SENSOR_TANK_5000L_HIGH) && (i == 0)) {
                    Serial.println("[WARNING] High level reached in 5000L tank! Stopping pump early.");
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(1000));
                elapsed++;
            }

            step->action_stop();
            Serial.printf("[STEP %u/%u] COMPLETED. Waiting post delay: %lu s\n",
                          i + 1, TOTAL_STEPS, step->post_delay_sec);

            vTaskDelay(pdMS_TO_TICKS(step->post_delay_sec * 1000));
        }

        Serial.println("\n--- Cycle Complete. Waiting 10 seconds before next batch ---");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println("Initializing Water Treatment HAL & APIs...");
    api_init();

        xTaskCreatePinnedToCore(
        waterTreatmentSequencerTask,
        "wt_sequencer",
        4096,
        NULL,
        1,
        NULL,
        0
    );
}

void loop() {
      vTaskDelay(pdMS_TO_TICKS(500));
}