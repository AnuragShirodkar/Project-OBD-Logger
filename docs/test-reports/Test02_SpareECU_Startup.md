# Test02_SpareECU_Startup

## Test

Spare Audi ECU: shorter startup and sustained logging

- Objective: Verify the reduced startup wait and operation through a user-requested stop.
- Outcome: **PASS for observed startup and capture health; unanswered diagnostic requests remain.**
- Test date/time: not established. The source header has an empty Date field; all times below are uptime milliseconds, not calendar time.
- Firmware provenance: the log identifies OBD-SkillLogger Version 1.0.0 but contains no commit hash. Exact flashed revision cannot be proven from the file.

## Setup and scope

Spare Audi ECU on a bench, as identified by the operator; ECU part number unavailable. This is not a real-vehicle test.

Project hardware: Teensy 4.1 with external PSRAM, built-in SD and TLE9371VSJ; CAN1 TX22/RX23, 500 kbps configuration, 11-bit functional request ID 7DF. These are project/operator configuration details; bitrate is not independently measured by the text log. The observed response ID was 7E8.

## Observed sequence and findings

The first request was submitted at 3.436 s, with the first reply at 3.444 s (8 ms). This is consistent with the 500 ms configured startup wait, rather than the former 5000 ms wait. Hardware initialization remains separate. The run ended at 511.409 s on User requested stop. No Mode 04 clear request was sent.

## Measurements

- Source size: 4,101,219 bytes.
- First / last diagnostic request submission: 3436 / 511339 ms.
- Request-observation span: 507.903 seconds.
- Diagnostic requests, including discovery and any manual clear: 14,954.
- Complete positive reply records: 10,474.
- Complete negative reply records: 0.
- No-response timeout records: 4,480.
- Overall diagnostic request rate: 29.44/second.
- Median time to first logged reply frame: 4 ms.
- Median consecutive RPM request spacing: 1020 ms.

Request counts exclude ISO-TP flow-control frames. Rate is count divided by first-to-last request span; it is not bus utilization or per-PID rate. First-reply latency uses the first logged RX-CAN frame after each request; TX timestamps represent mailbox submission, not a hardware bus acknowledgement. Positive service replies are not, by themselves, validation of every returned measurement.

## Final health evidence

```text
511409  STATUS  RXdrops=0 logDrops=0 consoleDrops=0
511409  CAPTURE_STATUS  seen=359437 drained=359437 buffered=0 peak=89 capacity=65536 hw_overruns=0
511409  SD_STATUS  log_peak=11 log_capacity=4096 write_calls=1415 max_write_us=81686 write_failures=0 unwritten_bytes=0
```

Zero reported loss counters are evidence for this run, not proof of lossless operation under every load. Capture totals include background traffic that is not all exported to the diagnostic text log.

## Limits and follow-up

Bench results apply to this spare ECU. They do not establish vehicle gateway behavior or multi-ECU operation.

The file is below 250 MB, so it does not validate rollover. It does not prove power-loss resilience, SD-full handling, EMC compliance or long-duration reliability. Actual LED behavior and USB file-copy integrity are not established by these text records alone.

## Source evidence

- Original supplied filename: `OBD0000 (2).TXT`.
- SHA-256 of original bytes: `3c1fddd83f5e40e88bd7380f7e25995ef0095a8d5303ff8d0d057af5726f0dae`.
- Source retained in the operator's supplied local logs; raw log not uploaded with this report.
- This document records an already performed test; no new hardware test was executed to create it.
