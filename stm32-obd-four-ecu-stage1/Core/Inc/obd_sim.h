#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct { uint16_t id; uint8_t dlc; uint8_t data[8]; uint32_t due; } ObdFrame;
size_t obd_reply(const ObdFrame *request,uint32_t now,ObdFrame replies[4]);
