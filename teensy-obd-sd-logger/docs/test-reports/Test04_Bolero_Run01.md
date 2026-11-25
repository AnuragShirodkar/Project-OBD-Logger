# Test04_Bolero_Run01

## Test

Bolero: first supplied vehicle run

- Objective: Verify standard OBD discovery, filtering, responses and recording on the real vehicle.
- Outcome: **PASS for basic communication and capture health; five request types were rejected.**
- Test date/time: not established. The source header has an empty Date field; all times below are uptime milliseconds, not calendar time.
- Firmware provenance: the log identifies OBD-SkillLogger Version 1.0.0 but contains no commit hash. Exact flashed revision cannot be proven from the file.

## Setup and scope

Bolero B6 BS6, described by the operator as a 2020 model; exact build/year not independently verified. The operator identified these as real-vehicle logs. Laptop/power-bank USB power was discussed, but the actual source used in each run was not recorded.

Project hardware: Teensy 4.1 with external PSRAM, built-in SD and TLE9371VSJ; CAN1 TX22/RX23, 500 kbps configuration, 11-bit functional request ID 7DF. These are project/operator configuration details; bitrate is not independently measured by the text log. The observed response ID was 7E8.

## Observed sequence and findings

The filter enabled 26 of 30 requests and skipped 01 2F, 09 08, 09 0A and 09 0B. RPM ranged from 0 to 1385.5; speed remained 0 km/h. Coolant temperature ranged from 70 to 74 C. These readings are consistent with a stationary vehicle and an engine start, but the operator sequence was not independently recorded.

Four freeze-frame requests (02 04 00, 02 05 00, 02 0C 00, 02 0D 00) each returned 7F 02 12 on 419 occasions. Mode 0A returned 7F 0A 11 on 419 occasions. No-response timeouts were zero, but negative replies are not successful measurements. Mode 04 was not sent.

## Measurements

- Source size: 2,638,746 bytes.
- First / last diagnostic request submission: 3649 / 196834 ms.
- Request-observation span: 193.185 seconds.
- Diagnostic requests, including discovery and any manual clear: 10,016.
- Complete positive reply records: 7,921.
- Complete negative reply records: 2,095.
- No-response timeout records: 0.
- Overall diagnostic request rate: 51.85/second.
- Median time to first logged reply frame: 9 ms.
- Median consecutive RPM request spacing: 440 ms.

Request counts exclude ISO-TP flow-control frames. Rate is count divided by first-to-last request span; it is not bus utilization or per-PID rate. First-reply latency uses the first logged RX-CAN frame after each request; TX timestamps represent mailbox submission, not a hardware bus acknowledgement. Positive service replies are not, by themselves, validation of every returned measurement.

## Final health evidence

```text
196842  STATUS  RXdrops=0 logDrops=0 consoleDrops=0
196842  CAPTURE_STATUS  seen=285742 drained=285742 buffered=0 peak=44 capacity=65536 hw_overruns=0
196842  SD_STATUS  log_peak=34 log_capacity=4096 write_calls=720 max_write_us=22396 write_failures=0 unwritten_bytes=0
```

Zero reported loss counters are evidence for this run, not proof of lossless operation under every load. Capture totals include background traffic that is not all exported to the diagnostic text log.

## Limits and follow-up

The same support maps appeared in both vehicle runs. One diagnostic responder (7E8) was observed; this is not an inventory of all vehicle modules. Mode 01 PID 11 returned an unexpected payload length and needs validation before decoding throttle position. VIN and calibration data were received, but vehicle identifiers and raw logs are not included in this report.

The file is below 250 MB, so it does not validate rollover. It does not prove power-loss resilience, SD-full handling, EMC compliance or long-duration reliability. Actual LED behavior and USB file-copy integrity are not established by these text records alone.

## Source evidence

- Original supplied filename: `OBD0000 (4).TXT`.
- SHA-256 of original bytes: `1c8bf61fdbae4871a2e1dae464ffd96a61f89605ce70d926496a2ddc98980984`.
- Source retained in the operator's supplied local logs; raw log not uploaded with this report.
- This document records an already performed test; no new hardware test was executed to create it.
