#include "obd_sim.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ObdFrame req(unsigned id,unsigned pid) {
    ObdFrame f={(uint16_t)id,8,{2,1,(uint8_t)pid,0,0,0,0,0}}; return f;
}
int main(void) {
    ObdFrame q=req(0x7df,0), out[4];
    assert(obd_reply(&q,0,out)==4);
    const uint8_t masks[4][4]={{0x18,0x1a,0x80,0x01},{0,0x18,0,1},{0,0x40,0,1},{0,2,0,1}};
    for(unsigned ecu=0;ecu<4;++ecu) {
        assert(out[ecu].id==0x7e8+ecu && out[ecu].dlc==8);
        assert(out[ecu].data[0]==6 && out[ecu].data[1]==0x41);
        assert(!memcmp(out[ecu].data+3,masks[ecu],4));
    }
    q=req(0x7e0,0x0c); assert(obd_reply(&q,0,out)==1);
    const uint8_t idle[]={4,0x41,0x0c,0x0c,0x80,0,0,0};
    assert(!memcmp(out[0].data,idle,8)); /* 800 RPM */
    assert(obd_reply(&q,30000,out)==1);
    assert(((out[0].data[3]<<8)|out[0].data[4])==3950*4);
    q=req(0x7df,0x0c); assert(obd_reply(&q,0,out)==2);
    q=req(0x7e2,0x0c); assert(obd_reply(&q,0,out)==0);
    q=req(0x7e3,0x3c); assert(obd_reply(&q,0,out)==1);
    assert(((out[0].data[3]<<8)|out[0].data[4])==3900); /* 350 C */
    q=req(0x7e2,0x5e); assert(obd_reply(&q,0,out)==1);
    assert(out[0].data[3]==0 && out[0].data[4]==16); /* 0.8 L/h */
    /* Every advertised PID responds; every unadvertised PID stays silent. */
    for(unsigned ecu=0;ecu<4;++ecu) for(unsigned base=0;base<=0x40;base+=32) {
        q=req(0x7e0+ecu,base); assert(obd_reply(&q,0,out)==1);
        uint32_t mask=((uint32_t)out[0].data[3]<<24)|((uint32_t)out[0].data[4]<<16)|
                      ((uint32_t)out[0].data[5]<<8)|out[0].data[6];
        for(unsigned i=1;i<=32;++i) {
            q=req(0x7e0+ecu,base+i);
            assert((obd_reply(&q,0,out)!=0)==((mask>>(32-i))&1u));
        }
    }
    q=req(0x7df,0); q.dlc=2; assert(!obd_reply(&q,0,out));
    q=req(0x7df,0); q.dlc=9; assert(!obd_reply(&q,0,out));
    q=req(0x7df,0); q.data[0]=0x10; assert(!obd_reply(&q,0,out));
    q=req(0x7df,0); q.data[0]=3; assert(!obd_reply(&q,0,out));
    q=req(0x7df,0); q.data[1]=9; assert(!obd_reply(&q,0,out));
    q=req(0x7e8,0); assert(!obd_reply(&q,0,out));
    assert(!obd_reply(NULL,0,out));
    q=req(0x7df,0x42); assert(obd_reply(&q,UINT32_MAX,out)==4);
    puts("PASS: discovery, addressing, known values, PID masks, malformed requests, tick wrap");
}
