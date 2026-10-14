#include "vehicle_app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick, pending_rx, free_slots=3, sent, ids[100], aborts;
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef*h,CAN_FilterTypeDef*f) {
    (void)h; assert(f->FilterMaskIdLow==6); return HAL_OK;
}
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef*h) { (void)h; return HAL_OK; }
uint32_t HAL_GetTick(void) { return tick; }
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef*h,uint32_t f) {(void)h;(void)f;return pending_rx;}
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef*h,uint32_t f,CAN_RxHeaderTypeDef*r,uint8_t*d) {
    (void)h;(void)f; --pending_rx;
    r->StdId=0x7df;r->IDE=0;r->RTR=0;r->DLC=8;
    memset(d,0,8);d[0]=2;d[1]=1; return HAL_OK;
}
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef*h) {(void)h;return free_slots;}
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef*h,uint32_t mask) {
    (void)h;assert(mask==7);++aborts;free_slots=3;return HAL_OK;
}
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef*h,CAN_TxHeaderTypeDef*r,uint8_t*d,uint32_t*m) {
    (void)h;assert(free_slots==3);assert(d[1]==0x41); *m=1;
    ids[sent++]=r->StdId;free_slots=2;return HAL_OK;
}
int main(void) {
    CAN_TypeDef regs={0}; CAN_HandleTypeDef h={&regs};
    assert(Vehicle_Init(&h)==HAL_OK); pending_rx=1;
    for(unsigned i=0;i<4;++i) {
        free_slots=3;Vehicle_Poll();assert(sent==i+1);assert(ids[i]==0x7e8+i);
        Vehicle_Poll();assert(sent==i+1); /* Busy mailbox must not lose next ECU. */
    }
    free_slots=3;Vehicle_Poll();assert(sent==4);
    pending_rx=1;Vehicle_Poll();assert(sent==5);
    tick=101;Vehicle_Poll();assert(aborts==1 && vehicle_stats.tx_errors==1);
    assert(vehicle_stats.queue_drops==3);Vehicle_Poll();assert(sent==5);
    /* Bounded queue, whole response-group drop, and counted overflow. */
    pending_rx=100;
    for(unsigned i=0;i<5;++i) Vehicle_Poll();
    assert(vehicle_stats.queue_drops>3);
    regs.ESR=CAN_ESR_BOFF;Vehicle_Poll();Vehicle_Poll();assert(vehicle_stats.bus_off==1);
    regs.RF0R=CAN_RF0R_FOVR0;Vehicle_Poll();assert(vehicle_stats.fifo_overruns==1);
    puts("PASS: four-ECU transmit queue, busy mailboxes, disconnect timeout, overflow, bus-off counters");
}
