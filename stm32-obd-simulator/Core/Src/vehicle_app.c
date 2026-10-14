#include "vehicle_app.h"
#include "obd_sim.h"

volatile VehicleStats vehicle_stats;
static CAN_HandleTypeDef *bus;
static ObdFrame queue[64];
static unsigned head, tail, used;
static uint32_t last_bus_off, last_tx;

HAL_StatusTypeDef Vehicle_Init(CAN_HandleTypeDef *can) {
    CAN_FilterTypeDef f={0};
    bus=can; head=tail=used=0;
    /* Accept standard data frames 0x7C0..0x7FF; software narrows addresses. */
    f.FilterBank=0; f.FilterMode=CAN_FILTERMODE_IDMASK;
    f.FilterScale=CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh=0x7c0u<<5; f.FilterMaskIdHigh=0x7c0u<<5;
    f.FilterMaskIdLow=6; /* Match IDE=0 and RTR=0. */
    f.FilterFIFOAssignment=CAN_FILTER_FIFO0; f.FilterActivation=ENABLE;
    f.SlaveStartFilterBank=14;
    if(HAL_CAN_ConfigFilter(bus,&f)!=HAL_OK) return HAL_ERROR;
    return HAL_CAN_Start(bus);
}
void Vehicle_Poll(void) {
    uint32_t now=HAL_GetTick();
    uint32_t off=bus->Instance->ESR & CAN_ESR_BOFF;
    if(off && !last_bus_off) ++vehicle_stats.bus_off;
    last_bus_off=off;
    if(bus->Instance->RF0R & CAN_RF0R_FOVR0) {
        ++vehicle_stats.fifo_overruns;
        __HAL_CAN_CLEAR_FLAG(bus,CAN_FLAG_FOV0);
    }
    /* Bounded receive work so transmit is not starved by unrelated traffic. */
    for(unsigned n=0;n<8 && HAL_CAN_GetRxFifoFillLevel(bus,CAN_RX_FIFO0);++n) {
        CAN_RxHeaderTypeDef h; ObdFrame q={0}, replies[4];
        if(HAL_CAN_GetRxMessage(bus,CAN_RX_FIFO0,&h,q.data)!=HAL_OK) break;
        ++vehicle_stats.rx;
        if(h.IDE!=CAN_ID_STD || h.RTR!=CAN_RTR_DATA) continue;
        q.id=(uint16_t)h.StdId; q.dlc=(uint8_t)h.DLC;
        size_t count=obd_reply(&q,now,replies);
        if(!count) ++vehicle_stats.ignored;
        /* Keep all responses to a functional request, or reject the whole set. */
        if(count>64u-used) { vehicle_stats.queue_drops+=(uint32_t)count; continue; }
        for(size_t i=0;i<count;++i) {
            queue[head]=replies[i]; head=(head+1u)%64u; ++used;
        }
    }
    /* One mailbox at a time preserves response order. Timeout prevents an
       unplugged receiver retaining a stale hardware transmission forever. */
    if(HAL_CAN_GetTxMailboxesFreeLevel(bus)!=3) {
        if((uint32_t)(now-last_tx)>=100u) {
            HAL_CAN_AbortTxRequest(bus,CAN_TX_MAILBOX0|CAN_TX_MAILBOX1|CAN_TX_MAILBOX2);
            vehicle_stats.queue_drops+=used; used=0; head=tail=0;
            ++vehicle_stats.tx_errors; last_tx=now;
        }
        return;
    }
    if(!used || off) return;
    CAN_TxHeaderTypeDef h={0}; uint32_t mailbox;
    h.StdId=queue[tail].id; h.IDE=CAN_ID_STD; h.RTR=CAN_RTR_DATA; h.DLC=8;
    if(HAL_CAN_AddTxMessage(bus,&h,queue[tail].data,&mailbox)==HAL_OK) {
        tail=(tail+1u)%64u; --used; ++vehicle_stats.tx; last_tx=now;
    } else ++vehicle_stats.tx_errors;
}
