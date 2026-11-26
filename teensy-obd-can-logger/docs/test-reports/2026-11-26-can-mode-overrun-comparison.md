# CAN mode LOG ERROR investigation — 26 November 2026

## Result

All three supplied logger captures stopped after a hardware receive-overrun
indication. Firmware deliberately converts that indication into a logging fault
and closes the recording. These runs do not pass continuous-recording validation.
The logs do not identify the underlying cause of the receive-service delay.

No SD write failure or software capture/log queue drop was reported. Increasing
PSRAM capacity alone does not address the controller-side receive overflow.
Do not remove the overrun check to hide the fault.

## Inputs and method

Reviewed the exact four `C:/Users/LENOVO/Desktop/CAN (1).asc` through
`CAN (4).asc` files and the exact `E:/LOGS/01_CAN_1757338.ASC` through
`03_CAN_1757338.ASC` files. Similarly named Desktop logger copies are different
runs and were excluded. Original files were not modified.

Analysis used snapshots with SHA-256 verification. Parsed ASC frame records,
checked timestamp order and DLC, and aligned ordered frame signatures containing
CAN identifier, standard/extended marker, data/remote type, DLC and payload bytes.
Absolute timestamps were excluded from signature matching because the recording
start times differ. Matching long sequences was followed by direct equality
assertions for every frame in the reported overlaps. Short matches in repeated
traffic were not accepted as evidence of another paired run.

All seven captures contain 34 distinct standard CAN IDs, received data frames
with DLC 8, and no backwards frame timestamps. Kvaser frame counts are 8,328;
37,195; 9,197; and 17,622 respectively. Trigger-event text is not a CAN frame.

## Frame comparison

- Logger `01`: all **10,097 frames** match Kvaser `CAN (2).asc` frames
  **3,686–13,782**, in order with identical IDs, DLC and data.
  Logger elapsed interval: 0.000131–7.374801 seconds.
  Kvaser interval: 2.720130–10.103130 seconds.
  The Kvaser file contains another 23,413 frames after this section.
- Logger `02`: its final **3,133 frames**, frames **15,820–18,952**, match
  Kvaser `CAN (3).asc` frames **1–3,133**, exactly and in order.
  Logger interval: 11.560504–15.632259 seconds.
  Kvaser interval: 0.024860–2.313880 seconds.
  The Kvaser file contains another 6,064 frames after this section.
  The first 15,819 logger frames have no established full matching reference
  window in these supplied files; they are not independently validated here.
- Logger `03`: all **8,327 frames** match Kvaser `CAN (4).asc` frames
  **125–8,451**, exactly and in order.
  Logger interval: 0.002122–6.079819 seconds.
  Kvaser interval: 0.150110–6.233440 seconds.
  The Kvaser file contains another 9,171 frames after this section.
- `CAN (1).asc` has no established long matching section with these three logger
  runs. Its 8,328-frame total being close to logger `03` is not a valid pairing.

Frame numbers above are one-based CAN-frame positions, not text-file line numbers.
There are no inserted, missing or altered frames inside the established matching
sections. Frames recorded by Kvaser after the logger stopped are outside its
recording window, not an assertion of thousands of silent in-window losses.
An overflow means the full boundary capture cannot be certified lossless; an
overrun count is not a count of lost frames.

## Stop evidence

Each logger file reports `STOP Logging or CAN failure; reset required`, followed
by `CAPTURE_DRAINED` and `End Triggerblock`.

- `01`: seen/drained/ASC rows **10,097**; hardware overruns **2**;
  capture peak **21 / 65,536**; log queue peak **31 / 4,096**;
  maximum SD write **2,651 us**; stop uptime **10,858 ms**.
- `02`: seen/drained/ASC rows **18,952**; hardware overruns **1**;
  capture peak **57 / 65,536**; log queue peak **65 / 4,096**;
  maximum SD write **15,883 us**; stop uptime **19,117 ms**.
- `03`: seen/drained/ASC rows **8,327**; hardware overruns **1**;
  capture peak **15 / 65,536**; log queue peak **23 / 4,096**;
  maximum SD write **2,651 us**; stop uptime **9,566 ms**.

All three report `RXdrops=0 logDrops=0 consoleDrops=0`, `buffered=0`,
`write_failures=0` and `unwritten_bytes=0`. Each first-overrun flags value is
`000000E0`, including FIFO overflow bit 7 (`0x80`). These counters support that
all frames delivered to the software capture queue were written; they cannot
measure frames lost before delivery to that queue. The recorded trigger is not
the three-second idle stop, SD-full handling, or a reported SD write failure.

Firmware headers identify `1.2.0-dev4-idle-sd`, logger `OBD_1757338`, CAN mode,
500,000 bit/s, listen-only, and configuration fingerprint `BDFFF497`.

## Timing problem requiring investigation

Logger `02` has a **1.789508-second** timestamp gap between frame 18,946
(`13.842735`) and frame 18,947 (`15.632243`). The corresponding Kvaser frames
are at `2.307050` and `2.308380`, only **1.330 ms** apart. The following frames
remain in the same exact matching sequence. The logger timestamps near the stop
therefore do not faithfully represent the reference bus timing.

All three files report maximum CAN interrupt duration near 1.073 billion CPU
cycles, nominally about 1.79 seconds at 600 MHz. Those unusually similar values
require validation: a raw cycle-counter difference can include preemption, and
the existing counters alone do not establish whether a true handler stall,
counter/clock behaviour, or another interrupt interaction caused this result.
Do not present the nominal duration as a proven ISR execution time or the
timestamp gap as a proven period without bus traffic.

Kvaser records approximately **1,368–1,369 frames/second** over each supplied
capture. This is a frame rate, not a measured percentage of CAN bus utilization.
No OBD requests or decoded PID values are demonstrated by this passive CAN test.

## Firmware path and next corrective work

Reviewed `TeensyOBDSDLogger/TeensyOBDSDLogger.ino`, `CaptureRing.h`,
`CanRecording.h`, and the installed Teensy 1.62.0 FlexCAN_T4/core sources.
`captureCanInterrupt()` records FIFO overflow before the library clears it;
`onCan()` also counts a frame overrun flag. The main loop sets `loggingFault`
when `hardwareOverruns` is nonzero, then calls `stopRun()`.

The callback runs in the CAN interrupt context and currently writes captured
records directly to the external-PSRAM ring. Investigate receive service and
clock behaviour before assigning a physical-bus or SD-card root cause.

1. Validate cycle and receive timestamps against an independent time source;
   measure interrupt entry/exit with a scope/logic analyzer if available.
2. Instrument FIFO occupancy/flags and interrupt masking/preemption. Audit
   SD, USB/MTP, display and external-memory interactions with CAN servicing.
3. Evaluate a small internal-RAM interrupt staging queue and fast bounded FIFO
   draining, with external-memory transfers outside the receive interrupt.
   This is a proposed change, not a verified fix.
4. Repeat the same simultaneous Kvaser test after the change, including a long
   run, SD flushes/file rotation and USB connected/disconnected conditions.
   Pass criteria: no hardware/software overruns, no write failures, exact frame
   equality in aligned overlap windows, and no unexplained timestamp jumps.

No firmware was changed or flashed, no config was changed, and the connected
logger was not stopped as part of this investigation.

## Input SHA-256 digests

- CAN (1).asc: `986B30D7AA445936F7BB5ACE6FC94B063E3F4AE6A72502F7FB91B03D6FB32535`
- CAN (2).asc: `75C3026CAC2BE18E734D5956B69E00887746ED47F6B72ACE0606E991B9146AA7`
- CAN (3).asc: `86DEE8C74AFB0C3374B2E36A4D3B6B0ECCB2937D6723CE2692FA90D3641045D4`
- CAN (4).asc: `D9160B99AB5EA2FF32873079E0F4DCCBA7FFEB58B86551A95587F8EC02EE6091`
- 01_CAN_1757338.ASC: `CD987F0938D26FFB6B30715F218533621E3EC3844682AF550BDCC2194DFD1076`
- 02_CAN_1757338.ASC: `FB5903DB2EEFCEB4FFCE55933E93D5C40BF68CC38CFC8D64E9115B00E8BF02CB`
- 03_CAN_1757338.ASC: `BEA8BA1100C19CCEBD58770A819248FE9315DBCEC3A9F3FE69C1F6176A97009A`
