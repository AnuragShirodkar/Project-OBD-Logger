# Project OBD Logger

The current implementation is an STM32F407 four-ECU OBD bench simulator for testing a CAN logger.

Confirmed hardware: STM32F407G-DISC1 + TJA1050 simulator; Teensy 4.1 + two 8 MB PSRAM chips + TLE9371VSJ logger. The wiring guide selects Teensy CAN1 on pins 22/23 and 3.3 V VIO for the logger transceiver.

- [Firmware, build instructions and test requests](stm32-obd-simulator/README.md)
- [STM32 pin connections and bench bring-up](stm32-obd-simulator/WIRING.md)

The logger-side firmware is not included yet. The simulator supports a limited OBD Service 01 subset at 500 kbit/s; it is intended for an isolated test bench.

Host protocol and mocked CAN tests and an ARM build passed on 14 October 2026. Hardware flashing and a physical CAN exchange remain to be performed. Generated firmware and host test binaries are kept locally in `stm32-obd-simulator/build/` and are excluded from Git.
