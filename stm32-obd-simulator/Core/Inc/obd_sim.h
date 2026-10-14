#ifndef OBD_SIM_H
#define OBD_SIM_H
#include <stdint.h>
#include <stddef.h>
typedef struct { uint16_t id; uint8_t dlc; uint8_t data[8]; } ObdFrame;
/* Pure protocol engine: standard data frames only, passed by the adapter. */
size_t obd_reply(const ObdFrame *request, uint32_t milliseconds, ObdFrame replies[4]);
#endif
