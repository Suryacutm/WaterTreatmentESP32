#ifndef WATER_TREATMENT_API_H
#define WATER_TREATMENT_API_H

#include "hal_water_system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *step_name;
    uint32_t duration_sec;
    uint32_t post_delay_sec;
    void (*action_start)(void);
    void (*action_stop)(void);
} treatment_step_t;

void api_init(void);
void api_emergency_stop_all(void);

#ifdef __cplusplus
}
#endif

#endif 