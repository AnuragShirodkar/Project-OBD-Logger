#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
typedef struct { uint32_t ESR, RF0R; } CAN_TypeDef;
typedef struct { CAN_TypeDef *Instance; } CAN_HandleTypeDef;
typedef struct {
    uint32_t FilterBank,FilterMode,FilterScale,FilterIdHigh,FilterMaskIdHigh;
    uint32_t FilterMaskIdLow,FilterFIFOAssignment,FilterActivation,SlaveStartFilterBank;
} CAN_FilterTypeDef;
typedef struct { uint32_t StdId,IDE,RTR,DLC; } CAN_RxHeaderTypeDef;
typedef struct { uint32_t StdId,IDE,RTR,DLC; } CAN_TxHeaderTypeDef;
#define CAN_FILTERMODE_IDMASK 0
#define CAN_FILTERSCALE_32BIT 1
#define CAN_FILTER_FIFO0 0
#define ENABLE 1
#define CAN_ESR_BOFF 4
#define CAN_RF0R_FOVR0 16
#define CAN_FLAG_FOV0 16
#define CAN_RX_FIFO0 0
#define CAN_ID_STD 0
#define CAN_RTR_DATA 0
#define CAN_TX_MAILBOX0 1
#define CAN_TX_MAILBOX1 2
#define CAN_TX_MAILBOX2 4
#define __HAL_CAN_CLEAR_FLAG(h,f) ((h)->Instance->RF0R &= ~(f))
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef*,CAN_FilterTypeDef*);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef*);
uint32_t HAL_GetTick(void);
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef*,uint32_t,CAN_RxHeaderTypeDef*,uint8_t*);
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef*);
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef*,CAN_TxHeaderTypeDef*,uint8_t*,uint32_t*);
#endif
