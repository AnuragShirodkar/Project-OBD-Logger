# Automatic SD-configured OBD logger

This is a separate Arduino sketch. The original `teensy-obd-tester` files are unchanged. Open `TeensyOBDSDLogger/TeensyOBDSDLogger.ino`; Arduino requires the folder and sketch basename to match. Its protocol/report headers are independent copies, so copying this whole sketch folder is sufficient.

## First run

1. Install Teensy board support plus ArduinoJson 7 (tested with Teensy 1.62.0 and ArduinoJson 7.4.3). Select Teensy 4.1 in Arduino IDE. FlexCAN_T4 and SD are supplied by Teensy board support.
2. Copy the provided `config.json` to the ROOT of the Teensy microSD card. Do not rename it or place it inside a folder. Use the built-in SD slot and the same FAT32 card used in the manual tests.
3. Upload this NEW sketch. No Serial Monitor commands are accepted or needed. USB serial, if connected, only mirrors output.
4. With the ECU connected and ready, power the logger. It validates the entire configuration before transmitting. It listens during the configured five-second startup delay, then runs automatically.
5. The demo stops scheduling after 60 seconds, lets any active transaction finish (up to the 10-second transaction deadline), drains queued output, flushes and closes the file. The onboard LED gives a 150 ms pulse when a matching positive, negative, or response-pending diagnostic reply is received (at most twice per second). Background CAN frames and transmitted requests alone do not trigger it. It stays off before the first reply and after successful completion; a failed run blinks continuously at 250 ms on / 250 ms off. A response pulse confirms communication, not necessarily request success. Wait for completion before removing the SD card or disconnecting power. The serial output also says `LOG_CLOSED` on success.
6. Read the new `AUTO0000.TXT`, `AUTO0001.TXT`, etc. Existing files are not overwritten. Reset/power-cycle to reload JSON and start another run. Do not edit/remove the card while running.

CAN1 uses Teensy TX22/RX23 with the TLE9371 transceiver, STB grounded as before. The logger does not control ECU ignition or wake inputs. Correct bitrate, ECU power/wake state and wiring remain prerequisites. For a complete vehicle, disable added logger termination; the bench network has separate termination requirements. Run the initial automatic test on the spare ECU.

## Demo requests

- `01 0C`: current RPM, target interval 10 ms.
- `02 0C 00`: freeze-frame RPM, frame 00, target interval 10 ms. A stored freeze frame may not exist; timeout/negative responses are retained.
- `03`: stored emission-related codes, target interval 10 ms.
- `07`: pending emission-related codes, target interval 10 ms.
- `0A`: permanent emission-related codes, target interval 10 ms. The spare ECU did not answer this in previous testing; each timeout will slow other requests.
- `09 02`: VIN, target interval 1000 ms. The spare ECU previously supplied hyphens, which are logged unchanged.

Modes 04, 05, 06 and 08 are rejected by this demo's configuration validator. Each array entry is one mode-plus-parameter request, not an automatic scan of all PIDs in that mode. Edit the request hex string to change the PID; add entries to poll additional PIDs (maximum 16 unique requests).

## Timing: target periods, not guaranteed transmission rates

Only ONE diagnostic transaction is active at a time. A fair round-robin scheduler selects due requests. It never interrupts a multi-frame response and never queues a burst of missed requests. On selecting an overdue job it records the delay and skips old periods. Thus 10 ms is a requested schedule, NOT a claim of one completed exchange every 10 ms.

The demo preserves the tested functional addressing: `tx_id: "7DF"`, `rx_id: "0"`, 11-bit CAN. With this addressing, the core keeps the configured 1500 ms response window open to allow multiple ECUs to answer, even if the first answer arrives promptly. Actual request spacing in the default demo is therefore usually at least 1500 ms; all six requests get turns, and the one-second VIN interval will also be missed. This is deliberate response preservation, not a high-load stress generator.

For the identified spare ECU only, you can change `tx_id` to `"7E0"` and `rx_id` to `"7E8"` to target that ECU alone. A physical-target transaction ends as soon as its terminal positive/negative response is fully processed. Pending NRC 78 and multi-frame replies still keep it open. This addressing change has not yet been hardware-tested by the new sketch. A timeout still waits `response_timeout_ms`; do not shorten it solely to force an advertised rate. Late replies after a timeout can be ambiguous if an identical request has since been sent; raw frames are retained, but this protocol has no request sequence number on the wire.

ISO-TP flow control uses block size 8, minimum separation 10 ms, a one-second consecutive-frame timeout, and a ten-second overall transaction limit. Run duration stops NEW requests; it does not truncate an active response. Timing is best-effort cooperative scheduling: SD writes and CAN handling can add latency.

## JSON fields

- `version`: required, 1.
- `bitrate`: default 500000 if omitted; allowed 125000, 250000, 500000 or 1000000. No auto-baud detection.
- `address_bits`: 11 (default) or 29.
- `tx_id` / `rx_id`: hexadecimal strings without `0x`. RX `"0"` selects the standard functional response range. Functional TX must be `"7DF"` for 11-bit or `"18DB33F1"` for 29-bit. A nonzero RX requires the matching physical TX, which is also used for flow control.
- `startup_delay_ms`: 0..600000, default 5000.
- `run_duration_ms`: 1000..86400000, default 60000. Finite duration ensures an automatic log-close point.
- `response_timeout_ms`: 10..10000, default 1500; also the collection window for functional addressing.
- `requests`: 1..16 unique objects, each containing `request` (space-separated hex bytes) and `interval_ms` (10..3600000).

Unknown keys, unsupported modes, bad parameter counts, malformed IDs and invalid timing values prevent transmission. Configuration must be at most 8192 bytes. Missing or invalid configuration is reported in the new log if the SD card is writable. Configuration loads only at boot. The demo has no clock source: `Date:` stays blank; timestamps are uptime milliseconds, not wall-clock time.

## Logs and counters

The header remains `OBD-SkillLogger Version 1.0.0` and `Date:`. Logs retain background RX, submitted TX, flow control, positive/negative/pending responses, decoded values and timeouts.

`SCHEDULE_SEND` identifies the selected JSON job. Periodic and final `SCHEDULE` records include target interval, `submitted`, `skipped_periods`, `late_starts`, and maximum lateness. `submitted` means accepted into a CAN hardware mailbox, not independently verified ACK. `skipped_periods` counts elapsed opportunities discarded rather than replayed; `late_starts` counts submissions after their scheduled time. Skipped totals for a waiting job are updated when selected and finalized at stop. `STATUS` includes RX/log/console drops. USB console drops do not imply SD drops.

Capture overflow, SD/log failure, or bus-off stops the automatic run; there is no automatic restart. The raw error register is logged only when error bits/counters/fault state indicate a problem. Normal RX/TX state changes previously labeled CAN_ERROR by the manual sketch are filtered in this new sketch. Background capture uses a bounded buffer; zero-loss operation at high bus load is not guaranteed. Controller resets after timeouts may introduce a capture gap, and software drop counters do not measure those gaps. Power loss can lose buffered/unflushed output.

## Validation

Run `./test.ps1` in PowerShell (requires g++, Arduino CLI and ArduinoJson 7). Tests cover configuration rejection/defaults, fair scheduling, skipped periods, timer wrap, existing ISO-TP behavior, physical-target completion and pending/timeout handling. The script compiles the standalone sketch for Teensy 4.1. Bench validation of the new firmware remains required; no firmware is uploaded by this script.
