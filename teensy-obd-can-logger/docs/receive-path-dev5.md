# Receive-path changes: 1.2.0-dev5-rx-stage

Applies to the separate OBD/CAN project only. The old OBD-only project is unchanged.
This build targets the receive-overflow stops and suspicious timing measurements
documented in the 26 November CAN and OBD capture comparisons. Hardware resolution
is not claimed until the simultaneous Kvaser test passes.

## Changes

- The CAN receive callback now copies into 2,048 internal DTCM frame slots.
  It no longer writes the external PSRAM frame queue from the interrupt.
  The staging array uses 81,920 bytes (40-byte capture records).
- The main loop moves bounded batches into the existing 65,536-slot PSRAM
  backlog before decoding/formatting. It also transfers around SD writes.
  A full backlog leaves staged records intact. Shutdown and diagnostic timeout/
  scheduling decisions wait for both queues, preserving order and drain coverage.
- The IRQ wrapper invokes the bundled one-frame FIFO reader up to eight times
  per entry while receive data remains pending. It checks hardware overflow on
  each pass. CAN priority is 64, above default-priority interrupts (128), below
  SysTick (32). Neither this change nor finite buffers guarantees zero loss at
  arbitrary load or during arbitrarily long blocking operations.
- A reserved free-running GPT2 peripheral counts at 1 MHz from 24 MHz PERCLK,
  divided by 24. CAN elapsed timestamps, SD-write durations, loop-gap measurements
  and independent ISR microseconds use it instead of the DWT-derived `micros()`.
  The shared 64-bit extension is serialized briefly and sampled during silence;
  unsigned rollover handling retains multi-hour elapsed CAN timestamps.
  No timer interrupt or additional wire is required. Startup rejects an occupied
  GPT2 or unexpected PERCLK configuration rather than silently changing its clock.
  Do not add another GPT2-based library without resolving ownership.
- DWT cycle measurements remain available for comparison, with a counter of ISR
  intervals where the nominal DWT duration differs from GPT2 by more than 100 us.
  These intervals include preemption, not just useful handler work. OBD scheduler
  and millisecond traces still use Arduino's uptime clock; CAN ASC uses GPT2.
- OBD raw RX traces now include all accepted response-ID frames delivered to the
  software queue, including idle/late/unmatched replies, remote frames, and records
  drained after a stop. Decoding still excludes remote frames and stops normally.
  Traffic outside the configured OBD response filters is not exported as OBD RX.

## Diagnostics

`RX_STAGING` reports staging occupancy/peak/capacity and PSRAM occupancy.
`RXdrops` includes failed staging insertions. `CAPTURE_STATUS buffered` includes
both queues; its `peak` and `capacity` continue to describe the PSRAM backlog.
`CAPTURE_TIMING` adds `source=GPT2_1MHz`, `max_isr_us`, `clock_disagreements`,
`max_drain_passes` and `irq_priority`. Existing hardware-overrun and write-failure
checks still stop a faulty recording; they have not been disabled.

## Validation

Host capture tests exercise staging saturation, blocked external transfers,
uneven producer/consumer progress, queue wrap, payload order and final draining
with staged records remaining. Existing configuration, ISO-TP, scheduling, trace,
storage, display-independent controls and file-rotation checks are retained.
Build target: Teensy 4.1, Teensy 1.62.0, USB Serial + MTP Disk.

27 November build results: all 14 host test executables and the layout coverage
reporter completed successfully. The final sketch compiled successfully. Reported
RAM1 variables: 222,336 bytes; RAM1 free for local variables: 138,112 bytes.
RAM2 variables: 28,800 bytes; free for heap allocation: 495,488 bytes before
runtime allocations. External capture/log allocations remain 3.75 MiB plus
allocator overhead. The compiler reports an existing bundled FlexCAN_T4
`strncpy` truncation warning; no project compilation error was reported.
No upload or bench validation is included in these results.

Required next bench tests, without claiming they have already passed:

1. Confirm timer initialization succeeds and CAN ASC elapsed seconds track an
   independent reference over at least ten minutes, then over a timer rollover.
2. Replay the same multi-ECU/background traffic through Kvaser. Compare both CAN
   and OBD log overlap windows by ordered ID/DLC/data; include late and unmatched
   responses and a stop with a software backlog.
3. Require zero hardware overruns, staging/log drops and SD-write failures.
   Investigate any clock disagreements or unexplained receive-time discontinuity.
4. Repeat during long runs, SD flushes/file rotation and USB presence changes.
5. Confirm no-traffic SD access still stops after the existing three-second idle
   threshold, with queues drained and the final file closed before MTP access.

The malformed simulator PID-11 replies and Mode-0A negative responses are separate
from the overrun fix. They remain logged and validated; no invalid reply is made
valid by this build. The proposed JSON filter format is also a separate change.
