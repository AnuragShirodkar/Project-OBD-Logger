# Dual-mode implementation - software validation

Date: 2026-11-25. Firmware: `1.2.0-dev1-dual-mode`.
No device was flashed, and no connected device's SD config was changed: the user
explicitly deferred flashing and physical testing.

## Completed checks

- `test.ps1` passed all 14 host regression executables plus the layout-coverage
  reporter. Existing OBD addressing, ISO-TP, timing, discovery, readiness, Mode 04,
  rotation, filtering, MTP visibility and capture-ring checks passed.
- Configuration tests cover v1 compatibility, v2 OBD/CAN, real example files,
  selector/version/type errors and rejection of OBD-only fields in CAN mode.
- New passive-run tests cover startup delay, continuous silence (no idle stop),
  user stop, duration stop, stop priority and millisecond wrap.
- New ASC tests cover standard/extended/remote/zero-length frames, bounds rejection,
  six-digit elapsed timestamps, multiple micros wraps and a silent wrap sampled by
  the loop. These are software timestamps, not measured bus-arrival accuracy.
- Independent python-can 4.6.1 ASCReader imported four emitted fixture frames with
  correct IDs, frame flags, DLCs, bytes, channel and timestamps. The fixture has no
  date header, includes comment metadata and covers elapsed time beyond 71 minutes.
- Final Teensy 4.1 build passed with core 1.62.0, USB Serial + MTP, ArduinoJson 7.4.3,
  the core FlexCAN_T4/SD/SdFat libraries and installed Adafruit display libraries.
  The existing FlexCAN_T4 `strncpy` warning remains; no new sketch warnings remain.
- Final build memory report: flash code 161384, data 31788, headers 8552 bytes;
  RAM1 variables 139360, code 158408, padding 5432, remaining 221088 bytes;
  RAM2 variables 28800, remaining 495488 bytes. Runtime allocator/stack use is extra.
  Captured is 40 bytes: RX PSRAM allocation is 2.5 MiB, log queue is 1.25 MiB,
  total fixed external allocations 3.75 MiB (allocator metadata is additional).

## Reviewed protections and unchanged boundaries

CAN mode branches before OBD discovery/scheduling, stays in the controller's listen-only
mode, does not feed the diagnostic receiver and disables the pin-40 Mode 04 action.
Both diagnostic submission and the hardware send sink also reject CAN mode.
Installed FlexCAN_T4 begin() initializes listen-only and ACCEPT_ALL clears FIFO masks.
These source checks are not physical evidence that no ACK/error frames occur.

Missing/invalid selected config never initializes CAN and exposes available SD storage
with CONFIG ERROR. There is no automatic OBD fallback. Timestamp clock sampling uses
restore-state atomic access between ISR and main loop. CAN activity uses ISR reception,
not delayed SD queue draining. Closing drains queued software frames, persists the
capture-boundary/FIFO-pending note and final error/loss counters as ASC comments.

## Pending physical acceptance

Test both SD configurations and legacy v1 on the actual logger; compare full CAN
recordings with a simultaneous Kvaser capture at representative loads. Verify no
application TX, ACK or error signalling in listen-only mode, using a separate active
receiver. Check standard/extended/RTR, quiet/resume, stop and queue drain, actual
250 MB rotation, SD/OLED failures, and OBD/Mode 04 regressions. A pending hardware
FIFO at the capture cutoff is explicitly outside the drained software frame count.

Import the generated ASC into the intended Kvaser/Vector analysis software before
claiming compatibility with that software. Neither maximum supported bus load nor
lossless capture nor physical OLED appearance has been established here.
Battery/power-loss shutdown, CAN FD and ID filtering remain outside this version.
