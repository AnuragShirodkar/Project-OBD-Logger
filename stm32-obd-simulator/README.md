# STM32 four-ECU OBD bench simulator

Target: STM32F407G-DISC1 / STM32F407VG, TJA1050, STM32CubeIDE toolchain.

Start with [pin connections and bench bring-up](WIRING.md) before powering the hardware.
For every CubeMX pin, clock, CAN and interrupt setting, use [the complete STM32 configuration guide](STM32_CONFIGURATION.md).

This firmware makes ONE physical CAN node answer as FOUR logical diagnostic ECUs.
It implements a deliberately small OBD Service 01 subset. It is not a production
ECU emulator or a complete OBD conformance implementation. XCP and CCP are absent.

## Four profiles

All addresses and signal assignments below are **simulator conventions**, not
universal vehicle module addresses. Standard OBD does not provide universal gear,
transmission oil temperature, ABS, or body-control signals. This version therefore
uses only the standard parameter meanings listed below. The transmission profile
exposes shared powertrain measurements, not transmission-specific measurements.

- **Engine:** request `7E0`, response `7E8`. PIDs `04` calculated load,
  `05` coolant temperature, `0C` engine RPM, `0D` vehicle speed, `0F` intake air
  temperature, `11` throttle position, `42` module voltage.
- **Transmission:** request `7E1`, response `7E9`. PIDs `0C` engine RPM,
  `0D` vehicle speed, `42` module voltage.
- **Fuel system:** request `7E2`, response `7EA`. PIDs `0A` fuel pressure,
  `2F` fuel level, `42` module voltage, `5E` engine fuel rate.
- **Exhaust/aftertreatment:** request `7E3`, response `7EB`. PIDs `0F` intake
  air temperature, `3C` catalyst temperature bank 1 sensor 1, `3D` catalyst
  temperature bank 2 sensor 1, `42` module voltage.

The exhaust profile represents a synthetic two-bank engine configuration.
Functional requests to `7DF` reach every profile. Each supporting ECU replies.
PIDs `00`, `20`, and `40` advertise each profile's supported ranges correctly.
All numbers in CAN examples are hexadecimal.

## Behavior

This is request/response OBD, not unsolicited periodic CAN broadcasting. Values
change with uptime, but the STM32 sends readings only when the tester asks.
One request contains exactly one PID; ISO-TP single frames only. No flow-control
exchange is needed for these responses. Multi-PID requests, multi-frame requests,
other services, extended IDs, remote frames and unsupported parameters do not
receive a response. VIN, DTCs, freeze frames, ECU-name queries and negative-response
coverage are not implemented. Silence for unsupported requests is a deliberate
subset behavior, not a claim of complete standard compliance.

Speed ramps 0 to 90 to 0 km/h over a 60-second cycle. Engine RPM follows
`800 + 35 * speed`. Other values vary deterministically or remain fixed.
These are test waveforms, not a physically accurate drivetrain model. The cycle
starts at firmware boot, so a request after boot need not see the initial value.

The application polls CAN without interrupts, keeps a 64-frame software queue,
and submits one hardware transmission at a time. A stalled transmission is
aborted after 100 ms and queued replies are discarded. Automatic bus-off recovery
is enabled. It does not guarantee every response under excessive request load.
Start testing at 10 requests per second. For a functional request, collect all
responses before sending the next request.

In the debugger, watch `vehicle_stats`: received requests, submitted transmissions,
ignored requests, dropped response frames, transmit failures, bus-off transitions,
and receive FIFO overruns. `tx` counts mailbox submissions, not confirmed delivery.
The green PD12 LED blinks every 500 ms while the main loop runs; it does not prove
CAN connectivity. Initialization failures stop in `fail()` for inspection.

## Firmware configuration

- CAN1: PB8 RX, PB9 TX, alternate function 9; no internal pulls.
- Classical CAN, standard 11-bit IDs, 500 kbit/s, normal mode.
- SYSCLK 168 MHz; APB1 / CAN clock 42 MHz.
- CAN prescaler 6, BS1 11 TQ, BS2 2 TQ, SJW 1 TQ; sample point 85.7%.
- Default clock input: external 8 MHz ST-LINK MCO, HSE bypass.
- Older Discovery revisions with an 8 MHz crystal: build with `-Crystal`.

Verify the Discovery board revision's clock source before flashing. The default
requires the ST-LINK MCO clock to be present; keep the ST-LINK section powered.
There is no fallback to an inaccurate oscillator that silently changes CAN timing.
This does not change any board solder bridges or your wiring.

Both transceivers and controllers must be powered, terminated and connected before
the bench exchange. The Teensy must use normal mode, not listen-only, to acknowledge
frames. One physical STM32 represents all four logical ECUs; it does not reproduce
arbitration and independent faults of four separate controllers.

## Build and load

The compiled `build/vehicle.elf`, `build/vehicle.hex`, and `build/vehicle.bin` are
available after a successful build. The BIN load address is `0x08000000`; HEX and
ELF carry addresses. Use ST-LINK with STM32CubeProgrammer or a CubeIDE debug setup
to load the firmware. Flashing replaces the current application on the target.
No hardware has been flashed as part of preparing this package.

From PowerShell in this directory:

```powershell
python fetch_dependencies.py
.\build.ps1
.\test.ps1
```

Dependencies are official ST HAL v1.8.3, ST device CMSIS v2.6.10 and Arm CMSIS
5.9.0, downloaded into `vendor/` with upstream licenses. Downloads require network
access. The build automatically finds the GCC bundled under `C:\ST`; otherwise
pass `-ToolchainBin 'path/to/tools/bin'`. The host tests require desktop GCC.

For an older board clock source:

```powershell
.\build.ps1 -Crystal
```

The build script is the reproducible build entry point. This folder is **not** a
CubeMX-generated project and contains no `.ioc` or preconfigured CubeIDE debug
launch. To work in CubeIDE, create an empty STM32F407VG C project, then:

1. Replace its application sources with the files in `Core/Src`, avoiding duplicate
   `main`, SysTick and exception handlers. Use the supplied `Core/Inc` files.
2. Add the downloaded HAL sources, device `system_stm32f4xx.c`, and CMSIS headers.
   Keep only one copy of each HAL driver and the startup assembly.
3. Add include directories `Core/Inc`, `vendor/hal/Inc`,
   `vendor/device/Include`, and `vendor/cmsis`; define `USE_HAL_DRIVER` and
   `STM32F407xx`. Use Cortex-M4, FPv4-SP-D16, hard-float settings.
4. Use the supplied linker script `STM32F407VG.ld`, or the IDE's equivalent
   STM32F407VG script with matching startup symbols. Use the normal ST-LINK SWD
   debug connection. The standalone script already supplies a working binary.

## First bench checks

1. Send `7DF [8] 02 01 00 00 00 00 00 00`. Expect four responses:
   - `7E8 [8] 06 41 00 18 1A 80 01 00`
   - `7E9 [8] 06 41 00 00 18 00 01 00`
   - `7EA [8] 06 41 00 00 40 00 01 00`
   - `7EB [8] 06 41 00 00 02 00 01 00`
2. Send `7E0 [8] 02 01 0C 00 00 00 00 00`. Only `7E8` replies.
   RPM = `(256*A+B)/4` using response data bytes 3 and 4 (zero-based).
   At simulated time zero the response is `04 41 0C 0C 80 00 00 00` (800 RPM).
3. Send the same RPM query to `7DF`. Both `7E8` and `7E9` reply.
4. Query `7E2` PID `5E` for fuel rate: `(256*A+B)/20` L/h.
5. Query `7E3` PID `3C` for catalyst temperature: `(256*A+B)/10-40` C.
6. Stop the tester/disconnect its CAN link, then reconnect and resume requests.
   Check transmit timeout counters and fresh replies. Verify this on the bench;
   host tests cannot establish electrical operation or physical recovery timing.

## Validation performed

- Host protocol tests: exact discovery masks, all advertised PIDs, per-ECU routing,
  known RPM/fuel/catalyst encodings, malformed inputs, uptime wrap input.
- Mocked HAL tests: four-ECU queuing, busy mailbox retention, transmit timeout,
  queue overflow, bus-off and FIFO overrun counters.
- ARM firmware compilation and linking using the installed STM32CubeIDE GCC.
- Not yet verified: physical CAN timing, clock source on your actual board,
  ST-LINK flashing, or operation with Teensy.

The default bare-metal linker reports unused newlib syscall-stub warnings and an
RWX load-segment warning; these do not prevent the ELF/HEX build. No filesystem
or console syscalls are used by the application. This remains a bench prototype.

## References

- [OBD request/response examples](https://www.csselectronics.com/pages/obd-ii-pid-examples)
- [PID definitions and scaling](https://www.csselectronics.com/pages/obd2-pid-table-on-board-diagnostics-j1979)
- [ST CAN HAL](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/tree/v1.8.3)
- [STM32F407 datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [Discovery user manual](https://www.st.com/resource/en/user_manual/um1472-discovery-kit-with-stm32f407vg-mcu-stmicroelectronics.pdf)
