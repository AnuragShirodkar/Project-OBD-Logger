#include "obd_sim.h"
#include <string.h>

/* Synthetic ECU personalities, NOT production vehicle address assignments. */
static const uint8_t engine[] = {0x04,0x05,0x0c,0x0d,0x0f,0x11,0x42};
static const uint8_t transmission[] = {0x0c,0x0d,0x42};
static const uint8_t fuel[] = {0x0a,0x2f,0x42,0x5e};
static const uint8_t exhaust[] = {0x0f,0x3c,0x3d,0x42};
typedef struct { const uint8_t *pids; size_t count; } Profile;
static const Profile profiles[] = {
    {engine,sizeof engine}, {transmission,sizeof transmission},
    {fuel,sizeof fuel}, {exhaust,sizeof exhaust}
};
static int supports(unsigned ecu, unsigned pid) {
    const Profile *p = &profiles[ecu];
    for (size_t i=0; i<p->count; ++i) if (p->pids[i]==pid) return 1;
    return 0;
}
static unsigned highest(unsigned ecu) {
    const Profile *p = &profiles[ecu]; unsigned max=0;
    for (size_t i=0; i<p->count; ++i) if (p->pids[i]>max) max=p->pids[i];
    return max;
}
static void be16(uint8_t *d, unsigned value) { d[0]=(uint8_t)(value>>8); d[1]=(uint8_t)value; }
static size_t value(unsigned ecu, unsigned pid, uint32_t ms, uint8_t *d) {
    unsigned phase=(ms/1000u)%60u;
    unsigned speed=(phase<=30u ? phase : 60u-phase)*3u;
    unsigned rpm=800u+speed*35u;
    switch(pid) {
    case 0x04: d[0]=(uint8_t)(30u+speed*2u); return 1; /* 100*A/255 % */
    case 0x05: d[0]=(uint8_t)(120u+speed/9u); return 1; /* A-40 C */
    case 0x0a: d[0]=100; return 1; /* 3*A kPa */
    case 0x0c: be16(d,rpm*4u); return 2;
    case 0x0d: d[0]=(uint8_t)speed; return 1;
    case 0x0f: d[0]=(uint8_t)(65u+speed/15u); return 1;
    case 0x11: d[0]=(uint8_t)(20u+speed*2u); return 1;
    case 0x2f: d[0]=(uint8_t)(200u-(ms/60000u)%100u); return 1;
    case 0x3c: be16(d,(350u+speed*3u+40u)*10u); return 2;
    case 0x3d: be16(d,(330u+speed*3u+40u)*10u); return 2;
    case 0x42: be16(d,13800u+ecu*20u); return 2;
    case 0x5e: be16(d,16u+speed*3u); return 2; /* raw/20 L/h */
    default: return 0;
    }
}
size_t obd_reply(const ObdFrame *q, uint32_t ms, ObdFrame out[4]) {
    if (!q || !out || q->dlc<3 || q->dlc>8 || q->data[0]!=2 || q->data[1]!=1)
        return 0; /* Only one-PID Mode 01 ISO-TP single-frame requests. */
    if (q->id!=0x7df && (q->id<0x7e0 || q->id>0x7e3)) return 0;
    unsigned pid=q->data[2]; size_t count=0;
    for(unsigned ecu=0; ecu<4; ++ecu) {
        if(q->id!=0x7df && q->id!=0x7e0+ecu) continue;
        ObdFrame r={0}; r.id=(uint16_t)(0x7e8+ecu); r.dlc=8;
        r.data[1]=0x41; r.data[2]=(uint8_t)pid;
        if(pid==0 || pid==0x20 || pid==0x40) {
            if(pid && highest(ecu)<=pid) continue;
            uint32_t bits=0;
            for(unsigned offset=1; offset<=32; ++offset) {
                unsigned candidate=pid+offset;
                if(supports(ecu,candidate) || (offset==32 && highest(ecu)>candidate))
                    bits |= (uint32_t)1u<<(32u-offset);
            }
            r.data[0]=6;
            for(unsigned b=0;b<4;++b) r.data[3+b]=(uint8_t)(bits>>(24u-8u*b));
        } else {
            if(!supports(ecu,pid)) continue;
            r.data[0]=(uint8_t)(2+value(ecu,pid,ms,&r.data[3]));
        }
        out[count++]=r;
    }
    return count;
}
