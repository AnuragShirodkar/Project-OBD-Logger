# Automatic SD-configured OBD logger

This is a separate Arduino sketch. The original `teensy-obd-tester` files are unchanged. Open `TeensyOBDSDLogger/TeensyOBDSDLogger.ino`; Arduino requires the folder and sketch basename to match. Copy the WHOLE sketch folder, including headers, usb_identity.c and the src subfolder. The original manual sketch is unchanged.

## Upload and run continuously

1. Use Teensy board package **1.62.0**, ArduinoJson 7, and board **Teensy 4.1**. Select **Tools > USB Type > Serial + MTP Disk (Experimental)**. The build script selects this option automatically. No separate MTP library is required; the core responder is customized locally (see src/mtp/PROVENANCE.md).
2. Copy the updated `config.json` to the SD-card root. **`run_duration_ms: 0` means continuous logging**; omitting it also defaults to continuous. An old JSON with 60000 still stops after 60 seconds. Keep all other settings unchanged for comparison.
3. Upload this sketch with its complete folder. On the first setup, insert the configured card as usual. Subsequent file copies and configuration updates can use USB without removing it.
4. Power the ECU first, then Teensy. Normal CAN reception/acknowledgement begins after valid configuration and PSRAM checks. Diagnostic requests begin after the five-second startup delay. Logging continues until requested to stop, an optional duration expires, or a fault occurs.
5. Stop using one of the controls below. The logger stops scheduling new requests, lets the active exchange finish, stops capture, drains buffers, and closes the file. SD-card access becomes available only after this shutdown.

## Stop controls and Windows file access

- **Button:** connect a normally-open momentary pushbutton between **Teensy pin 35 and GND**. An internal pull-up is enabled; no external pull-up resistor is needed. Hold for at least three seconds to request a stop. This is NOT the onboard Program button (which enters the bootloader).
- **USB serial alternative:** send `stop` or `close` with a newline. These are control commands only; diagnostic requests still come from JSON. No Serial Monitor is needed if the external button is fitted.
- **File-access-only boot:** hold pin 35's button while powering/resetting the board. After SD initialization, the firmware exposes the card without starting CAN, validating JSON, or creating a log. This also allows installing/fixing config.json over USB.
- Connect a USB data cable to Windows. In normal firmware mode the USB product and MTP model/friendly name are **OBD logger**. After stop/file-access boot, open **This PC > OBD logger** and its SD storage to copy logs or replace config.json. MTP is a portable-device interface, not a drive letter; tools that require a normal disk path cannot use it directly. The storage volume may retain its existing SD volume label.
- During logging the USB device can enumerate, but it advertises **no SD storage**. Live file browsing/editing is intentionally unavailable. USB connection/disconnection alone neither stops nor restarts logging. Once stopped, it stays stopped until reset/power-cycle.
- Configuration edits take effect only on restart. Finish file transfers before disconnecting power or restarting. Use ordinary copy/replace in Explorer; do not restart while a JSON upload is incomplete.
- MTP publishes a storage-added/reset event after stop. If Explorer cached an empty device, refresh it or reconnect USB after the stop. Windows may cache an earlier friendly name: reconnect the newly flashed device; if necessary remove its old portable-device entry in Device Manager and reconnect. The compiled descriptors and responder name are checked by the build; actual Explorer presentation still needs a Windows hardware test.

The LED gives a short pulse for a matching diagnostic reply (including negative/pending replies), and is off after clean completion/file-access-only boot. A failed run blinks rapidly. The serial output says LOG_CLOSED on a clean stop. Wait for file access or completion before removing power.

## Manual Mode 04 button

Connect a second normally-open button between **pin 40 and GND** (INPUT_PULLUP). While logging is active, hold it for **two seconds** to queue one Mode 04 request. Release before another activation. A button held at boot must first be released. The request waits for the current transaction and uses the configured CAN addressing, then normal polling resumes. Button activation, transmission, response/negative response or timeout are logged. Stop/file-access takes priority and cancels any queued manual request. Mode 04 is unavailable during startup or file access and remains prohibited in automatic JSON jobs.

Mode 04 clears emissions-related diagnostic information, including stored codes and freeze-frame information, and resets readiness information. With the default functional address 7DF, the request is not limited to one ECU. Use the button deliberately. Pin 35's file-access-only boot shortcut still works immediately when held at power-up; the three-second hold applies during normal operation.

## Long runs

Each text log rotates near **128 MiB** into the next unused AUTOxxxx.TXT file. Rotation occurs between diagnostic transactions; capture interrupts continue and the PSRAM queue absorbs the file-write pause. Sequence numbers and counters continue across segments. Each new segment has the standard header plus a SEGMENT_START record; combine all segments when checking receive continuity. Files are never overwritten. Exhausting the card or available names stops the logger; it does not delete old logs. The run is continuous, not unlimited storage.

The old manual firmware and installed board libraries are not modified. The local MTP responder is tied to the 1.62.0 core API; revalidate when upgrading Teensy support.

CAN1 uses Teensy TX22/RX23 with the TLE9371 transceiver, STB grounded as before. The logger does not control ECU ignition or wake inputs. Correct bitrate, ECU power/wake state and wiring remain prerequisites. For a complete vehicle, disable added logger termination; the bench network has separate termination requirements. Run the initial automatic test on the spare ECU.

## Demo requests

- Mode 01: 00, 01, 04, 05, 0B, 0C, 0D, 0F, 10, 11, 1F, 2F, 42.
- Mode 02: 00, 02, 04, 05, 0C, 0D, each with freeze-frame number 00.
- Modes 03, 07 and 0A: no PID parameters.
- Mode 06: 00 (supported monitor IDs only).
- Mode 09: 00, 02, 04, 06, 08, 0A, 0B.

Mode 09 entries target 1000 ms; all other entries retain the 10 ms target. Availability depends on the ECU; timeouts and negative responses are retained.

Modes 04, 05 and 08 are rejected by this demo's configuration validator. Each array entry is one mode-plus-parameter request, not an automatic scan of all PIDs in that mode. Edit the request hex string to change the PID; add entries to poll additional PIDs (maximum 64 unique requests).

## Timing: target periods, not guaranteed transmission rates

Only ONE diagnostic transaction is active at a time. A fair round-robin scheduler selects due requests. It never interrupts a multi-frame response and never queues a burst of missed requests. On selecting an overdue job it records the delay and skips old periods. Thus 10 ms is a requested schedule, NOT a claim of one completed exchange every 10 ms.

The demo preserves the tested functional addressing: `tx_id: "7DF"`, `rx_id: "0"`, 11-bit CAN. With this addressing, the core keeps the configured 1500 ms response window open to allow multiple ECUs to answer, even if the first answer arrives promptly. Actual request spacing in the default demo is therefore usually at least 1500 ms; all 30 requests get turns, and the one-second VIN interval will also be missed. This is deliberate response preservation, not a high-load stress generator.

For the identified spare ECU only, you can change `tx_id` to `"7E0"` and `rx_id` to `"7E8"` to target that ECU alone. A physical-target transaction ends as soon as its terminal positive/negative response is fully processed. Pending NRC 78 and multi-frame replies still keep it open. This addressing change has not yet been hardware-tested by the new sketch. A timeout still waits `response_timeout_ms`; do not shorten it solely to force an advertised rate. Late replies after a timeout can be ambiguous if an identical request has since been sent; raw frames are retained, but this protocol has no request sequence number on the wire.

ISO-TP flow control uses block size 8, minimum separation 10 ms, a one-second consecutive-frame timeout, and a ten-second overall transaction limit. A nonzero run duration or user stop stops NEW requests; it does not normally truncate an active response. Timing is best-effort cooperative scheduling: SD writes and CAN handling can add latency.

## JSON fields

- `version`: required, 1.
- `bitrate`: default 500000 if omitted; allowed 125000, 250000, 500000 or 1000000. No auto-baud detection.
- `address_bits`: 11 (default) or 29.
- `tx_id` / `rx_id`: hexadecimal strings without `0x`. RX `"0"` selects the standard functional response range. Functional TX must be `"7DF"` for 11-bit or `"18DB33F1"` for 29-bit. A nonzero RX requires the matching physical TX, which is also used for flow control.
- `startup_delay_ms`: 0..600000, default 5000.
- `run_duration_ms`: 0 (continuous, default), or 1000..86400000 for a timed run. Use the stop button/USB stop command to close a continuous run safely.
- `response_timeout_ms`: 10..10000, default 1500; also the collection window for functional addressing.
- `requests`: 1..64 unique objects, each containing `request` (space-separated hex bytes) and `interval_ms` (10..3600000).

Unknown keys, unsupported modes, bad parameter counts, malformed IDs and invalid timing values prevent transmission. Configuration must be at most 8192 bytes. Missing or invalid configuration is reported in the new log if the SD card is writable. Configuration loads only at boot. Serial accepts only stop/close, not diagnostic requests. The demo has no clock source: `Date:` stays blank; timestamps are uptime milliseconds, not wall-clock time.

## Logs and counters

The header remains `OBD-SkillLogger Version 1.0.0` and `Date:`. Logs retain background RX, submitted TX, flow control, positive/negative/pending responses, decoded values and timeouts.

`SCHEDULE_SEND` identifies the selected JSON job. Periodic and final `SCHEDULE` records include target interval, `submitted`, `skipped_periods`, `late_starts`, and maximum lateness. `submitted` means accepted into a CAN hardware mailbox, not independently verified ACK. `skipped_periods` counts elapsed opportunities discarded rather than replayed; `late_starts` counts submissions after their scheduled time. Skipped totals for a waiting job are updated when selected and finalized at stop. `STATUS` includes RX/log/console drops. USB console drops do not imply SD drops.

Capture overflow, hardware FIFO overrun, SD/log failure, or bus-off stops the automatic run; there is no automatic restart. A still-pending transmit mailbox at transaction end also stops the run rather than resetting reception and starting another request. The raw error register is logged only when error bits/counters/fault state indicate a problem. Normal RX/TX state changes previously labeled CAN_ERROR by the manual sketch are filtered in this new sketch. Capture now uses 65,536 PSRAM slots and a separate 4,096-line PSRAM text queue. External addresses are checked because Teensy allocation may otherwise fall back to internal RAM. Missing/insufficient PSRAM prevents transmission and produces PSRAM_ERROR. The receive callback copies frames only; it never formats text or writes SD. The atomic single-producer/single-consumer queue lets reception continue during main-loop SD writes without disabling interrupts around every copy.

SD writes collect complete lines into blocks of at least 4 KiB (except periodic flushes and the final partial block). Logs retain the same readable format. USB mirroring is best effort and cannot block SD capture. Normal diagnostic timeouts and the start of scheduled polling no longer reset CAN or clear its receive queue. Diagnostic timeout decisions and new requests wait until the captured backlog is drained, which may further delay polling under load.

Each received frame includes `seq` (callback sequence) and `us` (callback-time microseconds, wraps about every 71 minutes) as well as the existing uptime milliseconds, ID, frame type, DLC, and bytes. Remote-request frames have no payload bytes. These are software capture timestamps, not exact wire timestamps. Sequence numbers detect losses after the receive callback, not frames lost before it; transmitter-generated sequence numbers remain necessary for end-to-end verification. The `Request` column is processing context, not proof that a background frame is a diagnostic reply.

`CAPTURE_STATUS` reports `seen`, `drained`, `buffered`, peak occupancy, capacity, and hardware-overrun events. The CAN1 ISR wrapper observes the FIFO-overflow flag before FlexCAN_T4 clears it; this is tested by compilation against bundled FlexCAN_T4 in Teensy 1.62.0 and requires hardware validation. Overrun events are not an exact count of lost frames. `SD_STATUS` reports log-queue peak, write calls, maximum observed write duration in microseconds, write failures, and known unwritten bytes. Zero counters cannot prove that all traffic on the wire was captured.

`CAPTURE_STOP` explicitly bounds the capture interval. Traffic after this point is excluded. The software queue is preserved and drained; `CAPTURE_DRAINED` follows final counters. At a normal drained stop, `seen = drained + RXdrops`, and buffered should be zero. Any pending hardware FIFO data at the stop boundary is reported separately and is outside the drained software queue. Switching to listen-only happens only at this final boundary, not between requests. No automatic controller recovery/reset is attempted during capture.

A finite buffer cannot guarantee zero loss at arbitrary bus load or during an arbitrarily long SD stall. Power loss can lose queued/unflushed data. If the card fails, failure details may only reach USB/the error LED because the card itself cannot reliably save them.

## Validation

Run `./test.ps1` in PowerShell (requires g++, Arduino CLI and ArduinoJson 7). Tests cover configuration rejection/defaults, fair scheduling, skipped periods, timer wrap, existing ISO-TP behavior, physical-target completion and pending/timeout handling. The script also tests capture ordering, full-buffer rejection, buffer-slot wrap and complete queue draining, then compiles the standalone sketch for Teensy 4.1. Bench validation of the new firmware remains required; no firmware is uploaded by this script.

## Next bench comparison

Use the updated config.json: 500 kbit/s, 30 jobs, five-second diagnostic delay, continuous duration, functional addressing and the 1500 ms response window. Upload with Serial + MTP selected. Power the spare ECU, then Teensy. Run beyond 60 seconds, request a safe stop, and copy the new AUTO log through Windows Explorer. Check CONFIG_OK, BUFFERS, CAPTURE_START, normal STOP, CAPTURE_DRAINED, final zero buffered/drop/overrun counters, write failures, and receive sequence continuity. Also compare CAN errors with earlier runs. This test improves and measures capture reliability; it does not change polling into a 10 ms stress test. A separate sequence-numbered STM32 traffic test is still needed to measure end-to-end losses.

## Validation still requiring hardware

Host tests cover zero/nonzero duration, held-button timing and timer wrap in addition to protocol, JSON and capture tests. The sketch builds in Serial + MTP mode. Hardware checks remain: Explorer naming/enumeration, copy a log and compare its hash, upload config and restart, unplug/replug USB, stop while a multi-frame reply is active, file-access-only boot, sustained logging beyond 60 seconds, 128 MiB rotation, and SD-full handling. Capture and file transfer intentionally run in separate modes, so simultaneous transfer performance is not claimed.

## Expanded request configuration

The supplied config.json now contains 30 explicit requests: 13 Mode 01 entries, six Mode 02 entries (frame 00), Modes 03/07/0A, Mode 06 supported-MID request 06 00, and seven Mode 09 entries. Mode 04 remains button-only; Modes 05/08 remain excluded. Upload the matching firmware before using this JSON: the request capacity is now 64 and Mode 06 accepts a one-byte MID.

This is a candidate polling list, not automatic discovery. Support-map responses are logged, but do not yet filter unsupported entries or add further ranges/MIDs automatically. Unsupported requests can time out. Mode 06 currently requests only the first support bitmap, not individual monitor results. Mode 09 uses 1000 ms targets; other entries retain the existing 10 ms target. With serialized functional requests and the 1500 ms response window, a 30-request cycle is roughly 45 seconds and may take longer. These targets are not achieved polling rates.
