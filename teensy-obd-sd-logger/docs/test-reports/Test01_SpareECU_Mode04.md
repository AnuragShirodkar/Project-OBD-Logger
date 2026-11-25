# Test01_SpareECU_Mode04

## Test

Spare Audi ECU: Mode 04 confirmation

- Objective: Verify a held clear button causes one request and confirmation follows the ECU reply.
- Outcome: **PARTIAL PASS: Mode 04 confirmed; the run subsequently stopped on a CAN communication fault.**
- Test date/time: not established. The source header has an empty Date field; all times below are uptime milliseconds, not calendar time.
- Firmware provenance: the log identifies OBD-SkillLogger Version 1.0.0 but contains no commit hash. Exact flashed revision cannot be proven from the file.

## Setup and scope

Spare Audi ECU on a bench, as identified by the operator; ECU part number unavailable. This is not a real-vehicle test.

Project hardware: Teensy 4.1 with external PSRAM, built-in SD and TLE9371VSJ; CAN1 TX22/RX23, 500 kbps configuration, 11-bit functional request ID 7DF. These are project/operator configuration details; bitrate is not independently measured by the text log. The observed response ID was 7E8.

## Observed sequence and findings

The button was detected at 57.793 s, queued at 59.793 s, and request 04 was submitted at 59.809 s. ECU 7E8 returned 44 at the same millisecond timestamp. MODE04_CONFIRMED was recorded at 59.879 s. Same-millisecond timestamps do not establish zero physical response latency. The firmware requested three yellow confirmation flashes; actual LED visibility is not proven by the log.

CAN errors began at 80.909 s (32 CAN_ERROR records). At 80.990 s the logger stopped because a transmit mailbox remained pending. The physical cause is not established; do not label this an intentional power-disconnection test.

## Measurements

- Source size: 589,864 bytes.
- First / last diagnostic request submission: 8132 / 80920 ms.
- Request-observation span: 72.788 seconds.
- Diagnostic requests, including discovery and any manual clear: 2,135.
- Complete positive reply records: 1,501.
- Complete negative reply records: 0.
- No-response timeout records: 634.
- Overall diagnostic request rate: 29.33/second.
- Median time to first logged reply frame: 4 ms.
- Median consecutive RPM request spacing: 1020 ms.

Request counts exclude ISO-TP flow-control frames. Rate is count divided by first-to-last request span; it is not bus utilization or per-PID rate. First-reply latency uses the first logged RX-CAN frame after each request; TX timestamps represent mailbox submission, not a hardware bus acknowledgement. Positive service replies are not, by themselves, validation of every returned measurement.

## Final health evidence

```text
80990  STATUS  RXdrops=0 logDrops=0 consoleDrops=0
80990  CAPTURE_STATUS  seen=54832 drained=54832 buffered=0 peak=35 capacity=65536 hw_overruns=0
80990  SD_STATUS  log_peak=11 log_capacity=4096 write_calls=201 max_write_us=22902 write_failures=0 unwritten_bytes=0
```

Zero reported loss counters are evidence for this run, not proof of lossless operation under every load. Capture totals include background traffic that is not all exported to the diagnostic text log.

## Limits and follow-up

Bench results apply to this spare ECU. They do not establish vehicle gateway behavior or multi-ECU operation.

The file is below 250 MB, so it does not validate rollover. It does not prove power-loss resilience, SD-full handling, EMC compliance or long-duration reliability. Actual LED behavior and USB file-copy integrity are not established by these text records alone.

## Source evidence

- Original supplied filename: `OBD0000 (1).TXT`.
- SHA-256 of original bytes: `19063ea0549593064f6dd6aa764b0cdd6e9811e5deb4726ed346a20fdfdc6114`.
- Source retained in the operator's supplied local logs; raw log not uploaded with this report.
- This document records an already performed test; no new hardware test was executed to create it.
