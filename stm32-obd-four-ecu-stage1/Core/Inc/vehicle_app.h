#ifndef VEHICLE_APP_H
#define VEHICLE_APP_H
#include "stm32f4xx_hal.h"
typedef struct {
    uint32_t rx, tx, ignored, queue_drops, tx_errors, bus_off, fifo_overruns;
} VehicleStats;
extern volatile VehicleStats vehicle_stats;
HAL_StatusTypeDef Vehicle_Init(CAN_HandleTypeDef *can);
void Vehicle_Poll(void);
#endif
