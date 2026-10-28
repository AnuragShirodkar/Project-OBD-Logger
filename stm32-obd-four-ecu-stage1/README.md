# Four-ECU OBD simulator: Stage 1

Separate STM32F407G-DISC1 project. Original simulator, user CubeIDE workspace, Teensy firmware and Bolero config are unchanged. This is a controlled bench simulator, not a model of actual engine/ABS/transmission module behavior. Do not connect this transmitting simulator to a real vehicle.

## What it does

One CAN1 controller emits four logical response IDs at 500 kbit/s, 11-bit IDs. Functional requests go to 7DF. Physical requests 7E0..7E3 target 7E8..7EB respectively. Single-frame OBD only; no VIN/ISO-TP multi-frame, DTC or Mode 04 clearing implementation yet.

- 7E8: Mode 01 RPM = 1000, coolant = 80 C; support bitmap 08 10 00 00.
- 7E9: Mode 01 RPM = 2000, speed = 25 km/h; support bitmap 00 18 00 00.
- 7EA: Mode 01 coolant = 90 C; support bitmap 08 00 00 00.
- 7EB: Mode 01 speed = 50 km/h; support bitmap 00 08 00 00.

All four answer 01 00. Modes 02 (frame 00), 06 and 09 answer base support queries with a zero bitmap so the existing Teensy discovery can finish with known unsupported status. This does not claim those modes implement measurements. Unsupported functional PID queries are silent; unsupported physical PID queries for implemented request formats return 7F <mode> 12. Mode 04 is ignored. Reply target delays are 2, 4, 6 and 8 ms for the four identities, subject to mailbox availability and arbitration; there are no blocking delays.

## Build and import

Local dependencies have been prepared in ignored vendor/. A fresh checkout needs Python 3 and `python fetch_dependencies.py` from this folder, which fetches official ST/Arm tagged sources and license files. Versions: STM32F4 HAL v1.8.3, CMSIS device F4 v2.6.10, CMSIS 5.9.0. Build/test output and dependencies are excluded from Git.

In STM32CubeIDE: File > Import > General > Existing Projects into Workspace; select this directory. Keep the project at its existing location (do not copy it into another project). Build Debug. This is a managed source project, not a CubeMX-generated .ioc project; do not regenerate the old project to use it. Program/debug the generated ELF through ST-LINK/SWD.

Alternative: run `./build.ps1` in PowerShell. It finds the installed ST ARM GCC toolchain under C:/ST and creates build/vehicle.elf, build/vehicle.hex and build/vehicle.bin. Flash the HEX using STM32CubeProgrammer and ST-LINK; BIN starts at 0x08000000. Flashing has not been performed by the agent.

The clock defaults to an external 8 MHz ST-LINK MCO in HSE bypass mode, PLL to 168 MHz, APB1 42 MHz. CAN uses prescaler 6, BS1=11, BS2=2, SJW=1: 42 MHz / (6 x 14) = 500 kbit/s. Board revision/clock routing must match. Keep ST-LINK powered. For a board actually using an 8 MHz crystal, use `./build.ps1 -Crystal`, or define VEHICLE_HSE_CRYSTAL in the CubeIDE C compiler symbols. Verify the clock arrangement rather than changing solder bridges blindly. Onboard green PD12 blinks every 500 ms when the main loop runs; it is not a CAN-success indicator. Debug variable vehicle_stats exposes RX/TX submissions, ignored requests, queue drops, TX errors, bus-off and FIFO overruns. TX submission is not confirmation of wire delivery. The inherited adapter aborts a stuck TX after 100 ms and discards queued replies; counters identify this fault. Controller automatic bus-off recovery is enabled.

## Bench wiring

- STM32 PB9 (CAN1_TX) to TJA1050 TX; PB8 (CAN1_RX) to TJA1050 RX.
- TJA1050 VCC to regulated 5 V, GND to common ground; normal-mode S low if exposed. PB8 is the chosen 5 V-tolerant digital input; keep STM32 powered while the transceiver drives it. Do not use this 5 V RX output on Teensy GPIO.
- Existing Teensy: TX22/RX23 through TLE9371 with 3.3 V VIO and STB low.
- Connect CANH to CANH, CANL to CANL and grounds. Do not join independent USB 5 V supplies.
- Use two 120-ohm terminations total, one at each end. Count the resistor already fitted on the TJA1050 module. Approximately 60 ohms across H/L with both nodes unpowered.
- Kvaser may monitor on a short tap; do not add a third termination.

See ../stm32-obd-simulator/WIRING.md for the existing hardware reference and clock checks.

## Teensy configuration and acceptance test

1. Save the current SD config.json as your Bolero backup. The repository's Bolero configuration remains untouched.
2. Enter Teensy USB file-access mode. Copy teensy-stage1-config.json to the SD as config.json, then restart.
3. Power/program the STM32 first, start it running, then start Teensy. No Teensy firmware change is needed.
4. Record 30-60 seconds, then hold Teensy pin 35 button for 3 seconds to close the log before reading it.
5. Expect four discovered IDs: 7E8, 7E9, 7EA and 7EB. PID_FILTER_SUMMARY must show configured=5 enabled=4 skipped=1. Request 01 2F must have submitted=0. Support 01 00 gets four replies; RPM, coolant and speed get two each, with the exact values above.
6. Verify no CAN errors/timeouts/drops and compare the diagnostic frames against Kvaser if attached. Restore the Bolero SD config before a vehicle session.

The five JSON requests use 100 ms targets for readable first tests. Discovery still queries its normal support modes. Successful filtering relies on all four ECU support replies being received. An incomplete scan must retain unknown jobs rather than incorrectly filtering them.

## Software verification

Run ./test.ps1 for exact support masks, distinct values, malformed request rejection, functional/physical addressing and tick wrap. It also runs the actual Teensy DiagnosticCore, Discovery and Config parser against the simulator: all four ECUs must be discovered and exactly four jobs retained. Run ./build.ps1 for the embedded firmware. These checks do not replace a hardware bench run or prove electrical timing, ACK behavior or loss-free capture.
