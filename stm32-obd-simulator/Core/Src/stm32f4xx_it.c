#include "stm32f4xx_hal.h"
void SysTick_Handler(void) { HAL_IncTick(); }
void HardFault_Handler(void) { for(;;) {} }
void MemManage_Handler(void) { for(;;) {} }
void BusFault_Handler(void) { for(;;) {} }
void UsageFault_Handler(void) { for(;;) {} }
