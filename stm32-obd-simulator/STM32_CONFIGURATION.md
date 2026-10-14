# STM32F407G-DISC1: complete CubeMX configuration

This is the manual configuration recipe for the existing four-ECU OBD simulator. Values are matched to Core/Src/main.c, Core/Src/vehicle_app.c and Core/Inc/stm32f4xx_hal_conf.h. This file is documentation, not an importable .ioc file. The existing standalone build already applies these settings.

## 1. Create the project

Select STM32F407VGT6 (LQFP100) in the MCU selector, or STM32F407G-DISC1 in the board selector. If using the board selector, decline initialization of all board peripherals: this application needs only CAN1, GPIO, RCC and the system tick. Select STM32CubeIDE as the toolchain if generating an IDE project. UI labels can vary by CubeMX version.

## 2. Pinout & Configuration: System Core

- SYS > Debug: Serial Wire. Reserve PA13 as SYS_JTMS-SWDIO and PA14 as SYS_JTCK-SWCLK. Do not reassign these pins.
- SYS > Timebase Source: SysTick, 1 ms tick. No RTOS.
- RCC > High Speed Clock (HSE): Bypass Clock Source for the existing default build, using 8 MHz from ST-LINK MCO into PH0/OSC_IN.
- If your physical board instead uses its fitted X2 8 MHz crystal, select Crystal/Ceramic Resonator and reserve PH0/OSC_IN and PH1/OSC_OUT. Build the supplied firmware with -Crystal for this arrangement.
- RCC > Low Speed Clock (LSE): Disable. RTC, LSI, PLLI2S and MCO output are not required. The ST-LINK MCO input is not the target MCU's MCO output setting.
- Keep HSI at its default 16 MHz; it is not the PLL source in this application.

Board name alone does not prove the active clock source. Check the board revision and populated clock-routing components against ST UM1472 section 7.11 and the matching schematic. Do not change solder bridges just to match a software selection. Keep ST-LINK powered when using its MCO. A missing/wrong HSE source makes clock initialization stop in fail().

## 3. Pin assignments and GPIO

- PB8: CAN1_RX, AF9, alternate-function push-pull, no pull-up/down, very-high speed. Connect to TJA1050 RXD.
- PB9: CAN1_TX, AF9, alternate-function push-pull, no pull-up/down, very-high speed. Connect to TJA1050 TXD.
- PD12: GPIO_Output, push-pull, no pull-up/down, low speed, initial output Low. Optional user label: LED_GREEN. This drives the onboard green LED; no external LED is needed.
- PA13/PA14: SWD debug, managed by SYS.
- PH0: external HSE input; PH1 is also used when selecting a crystal. Do not assign either oscillator connection to another function.
- NRST: retain the onboard reset/ST-LINK connection.
- BOOT0: Low for normal boot from Flash; no application GPIO configuration.

No application assignment is needed for other pins. Do not initialize audio, accelerometer, USB OTG, Ethernet, UART, SPI, I2C, ADC, SDIO, CAN2 or timers for this simulator. ST-LINK USB works independently of the target USB OTG peripheral. Keep GPIOB, GPIOD and CAN1 peripheral clocks enabled; the HAL initialization supplies the system clocks it needs.

## 4. Clock Configuration tab

Enter these exact values:

- HSE input frequency: 8 MHz.
- PLL source mux: HSE.
- PLLM: 8 -> PLL input 1 MHz.
- PLLN: 336 -> VCO output 336 MHz.
- PLLP: 2 -> PLLCLK 168 MHz.
- PLLQ: 7 -> 48 MHz peripheral clock domain (USB remains disabled).
- System clock mux: PLLCLK -> SYSCLK 168 MHz.
- AHB prescaler: /1 -> HCLK 168 MHz.
- Cortex system timer clock: HCLK, with 1 kHz SysTick interrupts.
- APB1 prescaler: /4 -> PCLK1 42 MHz; APB1 timer clocks 84 MHz.
- APB2 prescaler: /2 -> PCLK2 84 MHz; APB2 timer clocks 168 MHz.
- Voltage regulator scale: Scale 1; Flash latency: 5 wait states for this 3.3 V/168 MHz setup.
- HSE_VALUE in HAL configuration: 8000000U.

CAN1 uses PCLK1 = 42 MHz, not the doubled 84 MHz APB1 timer clock. Do not calculate CAN timing from the timer clock. Flash prefetch, instruction cache and data cache are enabled in the supplied HAL configuration.

## 5. Connectivity > CAN1

Activate CAN1 and explicitly choose PB8/PB9 if CubeMX initially selects another pin pair. Match these parameter settings:

- Operating mode: Normal (not loopback or silent).
- Prescaler: 6.
- Synchronization Jump Width: 1 time quantum.
- Time Segment 1: 11 time quanta.
- Time Segment 2: 2 time quanta.
- Time Triggered Communication Mode: Disable.
- Automatic Bus-Off Management: Enable.
- Automatic Wake-Up Mode: Disable.
- Automatic Retransmission: Enable.
- Receive FIFO Locked Mode: Disable.
- Transmit FIFO Priority: Enable (transmit request order).

Total bit length = 1 + 11 + 2 = 14 time quanta.
Bitrate = 42,000,000 / (6 * 14) = 500,000 bit/s.
Sample point = (1 + 11) / 14 = 85.714%.
Time quantum = 142.857 ns; bit time = 2 us.

Classical CAN, standard 11-bit identifiers, data frames, reply DLC 8. The logger must also use classical CAN at 500 kbit/s in normal mode. CAN FD is not supported by the STM32F407 CAN peripheral.

## 6. NVIC and DMA

- Leave CAN1 TX, RX0, RX1 and SCE interrupts disabled: Vehicle_Poll() handles reception, transmission and error checks.
- Do not configure DMA for CAN.
- SysTick interrupt: enabled, preemption priority 15 (subpriority 0 where shown).
- HAL priority grouping: NVIC_PRIORITYGROUP_4.
- Keep the normal system exception handlers. SysTick_Handler() must call HAL_IncTick() once per tick.

## 7. CAN filter and start-up code

The filter is runtime application code in Vehicle_Init(), not merely a CubeMX pin setting. Keep these settings:

- FilterBank = 0; SlaveStartFilterBank = 14.
- FilterMode = CAN_FILTERMODE_IDMASK.
- FilterScale = CAN_FILTERSCALE_32BIT.
- FilterIdHigh = 0xF800; FilterIdLow = 0x0000.
- FilterMaskIdHigh = 0xF800; FilterMaskIdLow = 0x0006.
- FilterFIFOAssignment = CAN_FILTER_FIFO0.
- FilterActivation = ENABLE.

This accepts standard data frames in 0x7C0..0x7FF. Software narrows requests to 0x7DF and 0x7E0..0x7E3. Reply identifiers are 0x7E8..0x7EB. Vehicle_Init() calls HAL_CAN_ConfigFilter() then HAL_CAN_Start(). No HAL_CAN_ActivateNotification() call is required.

## 8. Integrating generated code

The repository currently uses a standalone main.c and build.ps1, not a generated .ioc project. Generating a fresh CubeMX project does not automatically integrate the simulator.

For a separate generated project, keep its generated main.c, HAL_CAN_MspInit() and SystemClock_Config(), and add obd_sim.c/.h and vehicle_app.c/.h. After HAL_Init(), SystemClock_Config(), MX_GPIO_Init() and MX_CAN1_Init(), call Vehicle_Init(&hcan1) and handle failure. Call Vehicle_Poll() continuously in the main loop and toggle PD12 every 500 ms using HAL_GetTick(), without a blocking delay. Do not copy the standalone main.c or duplicate exception handlers into the same generated project. Keep additions in USER CODE regions so regeneration preserves them.

Alternatively, keep using the supplied build.ps1 unchanged; it already compiles the working standalone configuration. Existing tests passed for the standalone firmware, not a newly generated CubeMX project.

## 9. Build, programming and bench check

- Target: STM32F407VG, Cortex-M4, hard-float FPv4-SP-D16.
- Defines: STM32F407xx, USE_HAL_DRIVER; add VEHICLE_HSE_CRYSTAL only for the crystal configuration.
- Flash base: 0x08000000; use the supplied STM32F407VG.ld for the standalone build.
- Program build/vehicle.hex via onboard ST-LINK/SWD and verify. BIN alternative: build/vehicle.bin at 0x08000000.
- Keep the onboard ST-LINK-to-target SWD jumpers fitted and target power enabled; BOOT0 Low.
- PD12 toggles every 500 ms once the main loop runs.
- Send 7DF [8] 02 01 00 00 00 00 00 00 from the logger and collect four responses.
- See WIRING.md for both transceivers, 5 V/3.3 V supply handling and termination.

Host tests and the standalone ARM build passed. Actual clock routing, flashing, CAN electrical operation and a generated CubeMX project remain unverified.

## Sources

- Core/Src/main.c: clock tree, GPIO and CAN initialization.
- Core/Src/vehicle_app.c: CAN filter, start and polling.
- Core/Inc/stm32f4xx_hal_conf.h: HSE value, tick priority and HAL modules.
- Core/Src/stm32f4xx_it.c: SysTick and fault handlers.
- [ST UM1472: Discovery clock sources and board connections](https://www.st.com/resource/en/user_manual/um1472-discovery-kit-with-stm32f407vg-mcu-stmicroelectronics.pdf)
- [STM32F407 datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
