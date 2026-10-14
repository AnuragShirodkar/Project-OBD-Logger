# STM32 CAN bench wiring

## Confirmed project hardware

User-confirmed hardware: STM32F407G-DISC1 with TJA1050; Teensy 4.1 with two 8 MB PSRAM chips and TLE9371VSJ (14 October 2026).
Confirm the actual board revision, transceiver markings and module schematic before powering it.
Module header order varies: use signal labels, not connector position. IC pin numbers below refer to each transceiver's 8-pin package, not a module header.

## STM32 to TJA1050

- **PB9 (CAN1_TX) -> TXD** (IC pin 1).
- **PB8 (CAN1_RX) <- RXD** (IC pin 4).
- **GND -> GND** (IC pin 2).
- **Regulated 5 V -> VCC** (IC pin 3); the TJA1050 requires 4.75–5.25 V, not 3.3 V.
- **S -> GND** (IC pin 8) for normal operation, if exposed and not already strapped on the module.
- **Vref** (IC pin 5) is an output; leave unused for this bench wiring.
- **CANH** (IC pin 7) and **CANL** (IC pin 6) go to the bus.

These assignments match `Core/Src/main.c`: GPIO alternate function 9, no internal pulls.
PB8 is a five-volt-tolerant digital pin on STM32F407; the TJA1050 RXD output is referenced to its 5 V supply. Keep the MCU powered whenever that output is driven, and respect ST's input-voltage limits. Teensy 4.1 GPIOs are not five-volt tolerant; use the TLE9371VSJ with 3.3 V VIO as below.

Power the Discovery through its ST-LINK USB connection. A verified board 5 V output may supply the transceiver within the board's current budget. Do not tie an independent supply output to USB-derived 5 V. For a bare IC, add local supply decoupling per the datasheet; check whether the module already includes it.

## Teensy 4.1 to TLE9371VSJ

Use CAN1 for this single-channel bench. These are Teensy digital pin numbers:

- **Pin 22 (CTX1) -> TxD** (IC pin 1).
- **Pin 23 (CRX1) <- RxD** (IC pin 4).
- **Teensy GND -> GND** (IC pin 2).
- **Regulated 5 V -> VCC** (IC pin 3).
- **Teensy 3.3 V -> VIO** (IC pin 5). Do not strap VIO to 5 V when connected to the Teensy.
- **STB -> GND** (IC pin 8) for always-on bench operation. Its internal pull-up selects standby if left disconnected. If a carrier already drives STB, configure that control low instead of shorting its output to ground.
- **CANH** (IC pin 7) and **CANL** (IC pin 6) go to the bus.

Fit **1 uF from VCC to GND** and **100 nF from VIO to GND**, close to the IC, unless already fitted on the carrier. Supplying only VIO does not enable transmission. See the Infineon pin definitions and application circuit.

Power the Teensy from USB for initial testing. Use a verified regulated 5 V source for transceiver VCC with a common ground; do not join separate USB/external 5 V supply outputs. Do not apply 5 V to Teensy GPIO or its 3.3 V rail. Inspect carrier-board VIO straps before connecting RxD.

The two 8 MB PSRAM chips provide 16 MB nominal external RAM on the underside QSPI pads. They do not use CAN1 pins 22/23. PSRAM detection and memory testing remain logger-side tasks; no STM32 code change is needed for them.

## Between the two CAN nodes

Connect simulator CANH to logger-transceiver CANH, CANL to CANL, and a common ground between the two nodes. Use a short twisted pair for CANH/CANL.

Place one 120-ohm resistor across CANH and CANL at each physical end. Count any resistors already fitted to the modules: there should be two total on this two-node bench. With both nodes unpowered, the bus should measure approximately 60 ohms between CANH and CANL.

Each MCU needs its own CAN transceiver. Do not connect CANH/CANL to MCU GPIOs. The logger's MCU TX connects to its transceiver TXD, and its MCU RX to its transceiver RXD. Connect the two nodes through CANH, CANL and ground only; do not cross-connect their TXD/RXD signals.

## Firmware and first exchange

1. Build using `build.ps1`. Its default assumes an external 8 MHz MCO clock in HSE bypass mode. Verify the actual Discovery clock arrangement; use `build.ps1 -Crystal` only for a fitted 8 MHz crystal.
2. With power off, complete the wiring and termination check above.
3. Use STM32CubeProgrammer with ST-LINK/SWD to program `build/vehicle.hex` and verify it. The BIN alternative loads at `0x08000000`. Keep BOOT0 low for normal Flash boot.
4. Reset/run. PD12 green LED should toggle every 500 ms. A blinking LED confirms the loop runs, not that CAN works.
5. Configure the logger for **CAN1 on pins 22/23**, classical CAN at **500 kbit/s**, standard 11-bit identifiers, **normal mode**. Listen-only mode will not acknowledge the simulator's replies. Although the TLE9371VSJ supports CAN FD, this STM32 bench uses classical CAN only. Logger firmware is not yet included in this repository.
6. Send `7DF [8] 02 01 00 00 00 00 00 00`. Expect replies from `7E8`, `7E9`, `7EA`, and `7EB`; see the README for the exact bytes.
7. Send requests at 10 Hz initially and collect all responses before the next request. The simulator does not broadcast readings without requests.

If there is no reply, check the clock configuration, common ground, TXD/RXD directions, termination, bitrate, and normal mode. Inspect `vehicle_stats` in the debugger for received frames, submitted transmissions, drops and bus-off events. Hardware operation has not yet been verified.

## Manufacturer references

- [NXP TJA1050 datasheet: supply, pinout, mode selection and logic levels](https://www.nxp.com/docs/en/data-sheet/TJA1050.pdf)
- [ST STM32F407 datasheet: PB8/PB9 functions and FT input requirements](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [ST Discovery user manual: board headers, power and clock revisions](https://www.st.com/resource/en/user_manual/um1472-discovery-kit-with-stm32f407vg-mcu-stmicroelectronics.pdf)

- [Infineon TLE9371VSJ datasheet: VCC, VIO, STB and decoupling](https://www.infineon.com/assets/row/public/documents/10/49/infineon-tle9371vsj-datasheet-en.pdf)
- [PJRC Teensy 4.1 pinout card](https://www.pjrc.com/teensy/card11a_rev4_web.pdf)
- [PJRC Teensy 4.1: GPIO voltage and QSPI memory expansion](https://www.pjrc.com/store/teensy41.html)
