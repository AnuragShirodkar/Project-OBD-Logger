#pragma once
#include <Arduino.h>
// Reserved GPT2: 24 MHz PERCLK / 24 = 1 MHz, independent of DWT/SysTick.
// No timer interrupt is needed. The core configures PERCLK to the 24 MHz crystal.
// Reject an occupied timer or an unexpected clock instead of silently retiming it.
namespace automatic {
class CaptureTimer {
  uint32_t offset=0;
public:
  bool begin(){
    CCM_CCGR0 |= CCM_CCGR0_GPT2_BUS(3)|CCM_CCGR0_GPT2_SERIAL(3);
    if((GPT2_CR&GPT_CR_EN)||!(CCM_CSCMR1&CCM_CSCMR1_PERCLK_CLK_SEL)||
       (CCM_CSCMR1&CCM_CSCMR1_PERCLK_PODF(0x3f)))return false;
    GPT2_CR=GPT_CR_SWR;
    unsigned spins=100000;
    while((GPT2_CR&GPT_CR_SWR)&&--spins){}
    if(!spins)return false;
    GPT2_PR=23;GPT2_IR=0;GPT2_SR=0x3f;
    // Align initial epoch with the existing power-on millisecond trace.
    offset=micros();
    GPT2_CR=GPT_CR_CLKSRC(1)|GPT_CR_FRR|GPT_CR_ENMOD|GPT_CR_EN;
    asm volatile("dsb" ::: "memory");
    return true;
  }
  uint32_t now() const{return offset+GPT2_CNT;}
};
}
