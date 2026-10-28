#include "vehicle_app.h"

static CAN_HandleTypeDef hcan1;
static void fail(void) { __disable_irq(); for(;;) {} }
static void clock_init(void) {
    /* Discovery's external 8 MHz source: crystal on older revisions,
       ST-LINK MCO on newer ones. Default to MCO bypass for DISC1. */
    RCC_OscInitTypeDef o={0}; RCC_ClkInitTypeDef c={0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    o.OscillatorType=RCC_OSCILLATORTYPE_HSE;
#ifdef VEHICLE_HSE_CRYSTAL
    o.HSEState=RCC_HSE_ON;
#else
    o.HSEState=RCC_HSE_BYPASS;
#endif
    o.PLL.PLLState=RCC_PLL_ON; o.PLL.PLLSource=RCC_PLLSOURCE_HSE;
    o.PLL.PLLM=8; o.PLL.PLLN=336; o.PLL.PLLP=RCC_PLLP_DIV2; o.PLL.PLLQ=7;
    if(HAL_RCC_OscConfig(&o)!=HAL_OK) fail();
    c.ClockType=RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    c.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK; c.AHBCLKDivider=RCC_SYSCLK_DIV1;
    c.APB1CLKDivider=RCC_HCLK_DIV4; c.APB2CLKDivider=RCC_HCLK_DIV2;
    if(HAL_RCC_ClockConfig(&c,FLASH_LATENCY_5)!=HAL_OK) fail();
}
int main(void) {
    HAL_Init(); clock_init();
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_CAN1_CLK_ENABLE();
    GPIO_InitTypeDef g={0};
    g.Pin=GPIO_PIN_8|GPIO_PIN_9; g.Mode=GPIO_MODE_AF_PP; g.Pull=GPIO_NOPULL;
    g.Speed=GPIO_SPEED_FREQ_VERY_HIGH; g.Alternate=GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB,&g);
    g.Pin=GPIO_PIN_12; g.Mode=GPIO_MODE_OUTPUT_PP; g.Speed=GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD,&g);
    hcan1.Instance=CAN1; hcan1.Init.Prescaler=6; hcan1.Init.Mode=CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth=CAN_SJW_1TQ; hcan1.Init.TimeSeg1=CAN_BS1_11TQ;
    hcan1.Init.TimeSeg2=CAN_BS2_2TQ; /* 42 MHz / (6 * 14) = 500 kbit/s */
    hcan1.Init.AutoBusOff=ENABLE; hcan1.Init.AutoRetransmission=ENABLE;
    hcan1.Init.TransmitFifoPriority=ENABLE;
    if(HAL_CAN_Init(&hcan1)!=HAL_OK || Vehicle_Init(&hcan1)!=HAL_OK) fail();
    uint32_t led=0;
    for(;;) {
        Vehicle_Poll();
        if((uint32_t)(HAL_GetTick()-led)>=500u) {
            led=HAL_GetTick(); HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_12);
        }
    }
}
