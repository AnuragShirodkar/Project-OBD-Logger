# Dev5 installation and USB check — 27 November 2026

## Scope

User confirmed USB-only connection with ECU/vehicle CAN wiring disconnected and
then confirmed pressing the physical Program button. No live receive-load test
was performed. Existing SD configuration and log files were preserved.

## Installation

Before recovery, USB identified `OBD_1757338` on COM8 running
`1.2.0-dev4-idle-sd`, OBD mode. Normal preparation returned `state=failed`,
`stop_reason=TX pending at transaction end; stopped without resetting RX`,
with no SD failures or software drops. No ready response was assumed.
The physical Program button entered HalfKay (`VID 16C0`, `PID 0478`).

Flashed the already compiled Teensy 4.1 Serial + MTP image using
`teensy_loader_cli.exe --mcu=TEENSY41 -v`, launched hidden with captured output.
Loader reported finding HalfKay, programming, and booting, and exited with code 0.
No graphical loader was launched.

HEX SHA-256:
`357CC4BDE48E823FB0BF8C37D308CF194B081DBACB16CED738328BCBC2A31233`

After restart, `device_info` verified:

- Product `SkiDrive`, device ID **OBD_1757338**, unchanged.
- Firmware **1.2.0-dev5-rx-stage**.
- COM8, normal USB `VID 16C0`, `PID 04D5`.
- OBD mode, SD mounted and OLED responding.
- Configuration retained: fingerprint `BBCB677B`, 64 request jobs, 500 kbit/s,
  11-bit functional addressing. A local backup was made before programming.

## Startup-log readback

MTP exposed the SD card. Copied `/LOGS/04_OBD_1757338.TXT` locally for review;
its header identifies the new firmware and unchanged device/configuration.
The file reports:

- `RX_STAGING`: capacity 2,048, buffered/peak 0.
- `CAPTURE_TIMING`: source `GPT2_1MHz`, maximum ISR 2 us,
  clock disagreements 0, maximum drain passes 1, IRQ priority 64.
- Capture seen/drained/buffered 0, hardware overruns 0.
- Reported SD write failures/unwritten bytes 0; USB also reports no drops.
- `CAPTURE_DRAINED`, followed by a completed readable file.

The firmware reached OBD startup without a capture-timer or PSRAM initialization
failure. This confirms startup and readback, not timer-frequency accuracy under
load or successful CAN frame acquisition. OLED communication is reported by
firmware; visual display correctness was not inspected.

## Expected USB-only stop and remaining validation

The retained OBD configuration attempted its initial request with no CAN peer
connected. It stopped at 3,891 ms after power-on with
`TX pending at transaction end; stopped without resetting RX`. USB consequently
reports `state=failed`, `logging_fault=true`. This is an unacknowledged transmit
stop, not a newly observed receive overflow or SD-write error. The fault state
was not hidden or cleared, and capture did not automatically restart.

To test reception, connect the powered ECU/simulator and reference Kvaser bus
setup, then restart the logger. Compare the new OBD recording and a separate CAN
mode recording under the previously failing traffic. Required checks remain zero
overruns/drops/write failures, ordered frame equality, correct elapsed timing,
long-run/file-rotation behaviour and idle SD access. The receive-overrun problem
is **not yet declared resolved** by this USB-only check.
