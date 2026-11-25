# Test03_SpareECU_PIDFiltering

## Test

Spare Audi ECU: supported-PID filtering

- Objective: Verify only confirmed unsupported jobs are disabled and retained requests continue to run.
- Outcome: **PASS for observed filtering and capture health; repeated timeouts remain.**
- Test date/time: not established. The source header has an empty Date field; all times below are uptime milliseconds, not calendar time.
- Firmware provenance: the log identifies OBD-SkillLogger Version 1.0.0 but contains no commit hash. Exact flashed revision cannot be proven from the file.

## Setup and scope

Spare Audi ECU on a bench, as identified by the operator; ECU part number unavailable. This is not a real-vehicle test.

Project hardware: Teensy 4.1 with external PSRAM, built-in SD and TLE9371VSJ; CAN1 TX22/RX23, 500 kbps configuration, 11-bit functional request ID 7DF. These are project/operator configuration details; bitrate is not independently measured by the text log. The observed response ID was 7E8.

## Observed sequence and findings

At 4.830 s the filter reported configured=30 enabled=26 skipped=4. Requests 01 0B, 01 2F, 09 08 and 09 0B were skipped and their final submitted counters were zero. Unknown support is retained by design.

Remaining timeouts: 02 04 00 (634), 02 05 00 (633), 02 0C 00 (633), 02 0D 00 (633), and 0A (633). Silence alone is not proof of unsupported data. Compared with Test02, overall request rate rose from 29.4 to 34.6/s and median RPM request spacing fell from 1020 to 726 ms. The runs have different durations; compare rates rather than raw totals.

## Measurements

- Source size: 4,433,434 bytes.
- First / last diagnostic request submission: 3631 / 454181 ms.
- Request-observation span: 450.550 seconds.
- Diagnostic requests, including discovery and any manual clear: 15,574.
- Complete positive reply records: 12,408.
- Complete negative reply records: 0.
- No-response timeout records: 3,166.
- Overall diagnostic request rate: 34.57/second.
- Median time to first logged reply frame: 4 ms.
- Median consecutive RPM request spacing: 726 ms.

Request counts exclude ISO-TP flow-control frames. Rate is count divided by first-to-last request span; it is not bus utilization or per-PID rate. First-reply latency uses the first logged RX-CAN frame after each request; TX timestamps represent mailbox submission, not a hardware bus acknowledgement. Positive service replies are not, by themselves, validation of every returned measurement.

## Final health evidence

```text
454251  STATUS  RXdrops=0 logDrops=0 consoleDrops=0
454251  CAPTURE_STATUS  seen=323216 drained=323216 buffered=0 peak=89 capacity=65536 hw_overruns=0
454251  SD_STATUS  log_peak=34 log_capacity=4096 write_calls=1256 max_write_us=132604 write_failures=0 unwritten_bytes=0
```

Zero reported loss counters are evidence for this run, not proof of lossless operation under every load. Capture totals include background traffic that is not all exported to the diagnostic text log.

## Limits and follow-up

Bench results apply to this spare ECU. They do not establish vehicle gateway behavior or multi-ECU operation.

The file is below 250 MB, so it does not validate rollover. It does not prove power-loss resilience, SD-full handling, EMC compliance or long-duration reliability. Actual LED behavior and USB file-copy integrity are not established by these text records alone.

## Source evidence

- Original supplied filename: `OBD0000 (3).TXT`.
- SHA-256 of original bytes: `97d5f43dba33e9c3334b951ebc99a0d25fea08909fc7b8f2f7c258faa647f4de`.
- Source retained in the operator's supplied local logs; raw log not uploaded with this report.
- This document records an already performed test; no new hardware test was executed to create it.
