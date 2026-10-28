#include "obd_sim.h"
#include <string.h>
// Four logical ECUs on one physical CAN controller. Only Stage 1 services.
static const uint32_t masks[4]={0x08100000u,0x00180000u,0x08000000u,0x00080000u};
size_t obd_reply(const ObdFrame *q,uint32_t now,ObdFrame out[4]) {
    if(!q||!out||q->dlc>8||q->dlc<3||q->data[0]>7||q->data[0]+1u>q->dlc)return 0;
    unsigned first=0,last=4;
    if(q->id!=0x7df){if(q->id<0x7e0||q->id>0x7e3)return 0;first=q->id-0x7e0;last=first+1;}
    unsigned mode=q->data[1],pid=q->data[2];
    if(!((mode==1&&q->data[0]==2)||(mode==2&&q->data[0]==3&&q->data[3]==0)||
         ((mode==6||mode==9)&&q->data[0]==2)))return 0;
    size_t count=0;
    for(unsigned e=first;e<last;++e){
        ObdFrame r={0};r.id=0x7e8+e;r.dlc=8;r.due=now+2u+2u*e;
        r.data[1]=(uint8_t)(mode+0x40);r.data[2]=(uint8_t)pid;
        if(pid==0){
            uint32_t bits=mode==1?masks[e]:0;unsigned offset=mode==2?4:3;
            r.data[0]=(uint8_t)(offset+3); // SF payload excludes PCI.
            r.data[offset]=(uint8_t)(bits>>24);r.data[offset+1]=(uint8_t)(bits>>16);
            r.data[offset+2]=(uint8_t)(bits>>8);r.data[offset+3]=(uint8_t)bits;
        }else if(mode==1&&pid<=32&&(masks[e]&(1u<<(32-pid)))){
            if(pid==5){r.data[0]=3;r.data[3]=(uint8_t)(e==0?120:130);} // 80 / 90 C
            else if(pid==12){unsigned rpm=e==0?1000:2000;r.data[0]=4;r.data[3]=(uint8_t)((rpm*4)>>8);r.data[4]=(uint8_t)(rpm*4);}
            else if(pid==13){r.data[0]=3;r.data[3]=(uint8_t)(e==1?25:50);}
            else continue;
        }else {
            // Unsupported functional requests remain silent, physical queries get NRC 12.
            if(q->id==0x7df)continue;
            memset(r.data,0,8);r.data[0]=3;r.data[1]=0x7f;r.data[2]=(uint8_t)mode;r.data[3]=0x12;
        }
        out[count++]=r;
    }
    return count;
}
