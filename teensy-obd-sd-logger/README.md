# Automatic SD-configured OBD logger

This is a separate Arduino sketch. The original `teensy-obd-tester` files are unchanged. Open `TeensyOBDSDLogger/TeensyOBDSDLogger.ino`; Arduino requires the folder and sketch basename to match. Copy the WHOLE sketch folder, including headers, usb_identity.c and the src subfolder. The original manual sketch is unchanged.

## Upload and run continuously

1. Use Teensy board package **1.62.0**, ArduinoJson 7, and board **Teensy 4.1**. Select **Tools > USB Type > Serial + MTP Disk (Experimental)**. The build script selects this option automatically. No separate MTP library is required; the core responder is customized locally (see src/mtp/PROVENANCE.md).
2. Copy the updated `config.json` to the SD-card root. **`run_duration_ms: 0` means continuous logging**; omitting it also defaults to continuous. An old JSON with 60000 still stops after 60 seconds. Keep all other settings unchanged for comparison.
3. Upload this sketch with its complete folder. On the first setup, insert the configured card as usual. Subsequent file copies and configuration updates can use USB without removing it.
4. Power the ECU first, then Teensy. Normal CAN reception/acknowledgement begins after valid configuration and PSRAM checks. Startup support discovery begins after the demo configuration's 500 ms startup delay; configured polling begins after discovery. Logging continues until requested to stop, an optional duration expires, or a fault occurs.
5. Stop using one of the controls below. The logger stops scheduling new requests, lets the active exchange finish, stops capture, drains buffers, and closes the file. SD-card access becomes available only after this shutdown.

## OLED and Windows file access

Wire the SSD1306 128x32 I2C display to 3.3 V, GND, SDA 18 and SCL 19. Install Adafruit SSD1306 and its GFX and BusIO dependencies. Firmware probes 0x3C and 0x3D and can record without a display. Pin 40 is the only user button.

The OLED dashboard shows the device ID, a LIVE/WAIT/LOST badge, a large operating status, and a moving pulse along the bottom. The pulse moves only while diagnostic replies have arrived within the last five seconds and recording/discovery is active. It freezes on reply silence or shutdown. With slow configured polling it can pause between replies; this is a response-activity indicator. After confirmed connection loss, LINK LOST remains visible while the USB badge indicates file access. SAVE, USB and ERR badges identify shutdown, file access and faults. CLEAR WAIT, CLEAR OK and CLEAR FAIL show Mode 04 feedback.

Display frames refresh at most ten times per second and transfer in 16-byte chunks between logger loop iterations. An I2C transfer error disables the display until restart and records OLED_IO_FAILED when a log remains active. Bench-test readability, unplugged OLED and held-low I2C lines under CAN traffic before accepting display-fault resilience.

Windows identifies the portable device as OBD_<factory number>. During recording the device advertises no SD storage. Windows clients can enumerate devices independently and write config.json through MTP once the selected device exposes its SD storage. Restart after completing file transfers to load the new configuration.

Send stop or close followed by a newline over USB serial for a deliberate stop. After a polling timeout and at least five seconds without a diagnostic reply, firmware checks the connection using Mode 01 PID 00. Two unanswered probes end the run. A reply resets the check. CAN bus-off or a transmission fault can end the run sooner. A stopped logger drains its buffers and closes the log before making a mounted card available over USB. An unmounted card cannot be exposed. Removing all power interrupts the processor immediately and can lose unflushed data.

Bench check with separate USB power: record with the ECU connected, confirm Explorer cannot browse storage, disconnect CAN only, and wait for USB file access. Open the final file, check the stop reason, then replace config.json and reset the board. Repeat with the ECU absent at startup and with a single unsupported PID among responding requests.

## Manual Mode 04 button

Connect a normally-open button between **pin 40 and GND** (INPUT_PULLUP). While logging is active, hold it for **two seconds** to queue one Mode 04 request. Release before another activation. A button held at boot must first be released. The request waits for the current transaction and uses the configured CAN addressing, then normal polling resumes. Button activation, transmission, response/negative response or timeout are logged. Stop/file-access takes priority and cancels any queued manual request. Mode 04 can be queued during startup/discovery and is unavailable during file access and remains prohibited in automatic JSON jobs.

Mode 04 clears emissions-related diagnostic information, including stored codes and freeze-frame information, and resets readiness information. With the default functional address 7DF, the request is not limited to one ECU. Use the button deliberately.

## Long runs

### Elapsed time, without an RTC

The header contains firmware version, hardware ID, filename, configuration fingerprint, bitrate, addressing and configured request count when the configuration is valid. Elapsed milliseconds remain on trace lines. The header omits calendar timestamps. Windows filesystem timestamps are separate.

Each text log is capped at **250 MB (250,000,000 bytes, including the header)**. Before a buffered write would exceed the limit, the logger flushes/closes the current file and opens the next unused <sequence>_OBD_<factory number>.TXT file. A segment may therefore finish a few kilobytes below the limit. Capture interrupts continue and PSRAM buffers incoming frames during rotation. Counters continue across segments, and each new file gets the standard header. Rotation can occur during an exchange (or a long text payload), so read consecutive files in order for complete transactions. Files are never overwritten. A flush/open/header/write failure stops logging; existing files are retained. Exhausting storage or the sequence through 999999 also stops logging.

The old manual firmware and installed board libraries are not modified. The local MTP responder is tied to the 1.62.0 core API; revalidate when upgrading Teensy support.

CAN1 uses Teensy TX22/RX23 with the TLE9371 transceiver, STB grounded as before. The logger does not control ECU ignition or wake inputs. Correct bitrate, ECU power/wake state and wiring remain prerequisites. For a complete vehicle, disable added logger termination; the bench network has separate termination requirements. Run the initial automatic test on the spare ECU.

## Demo requests

Expanded from identical support bitmaps in both supplied Bolero logs (ECU 7E8). Original entries remain; discovery filters unsupported entries at each boot.

- Mode 01: 00, 01, 04, 05, 0B, 0C, 0D, 0F, 10, 11, 1C, 1F, 21, 23, 2F, 33, 42, 49, 88.
- Mode 02: 00, 02, 04, 05, 0B, 0C, 0D, 0F, 10, 11, 1F, 21, 23, 33, 42, 49, each with freeze-frame number 00.
- Modes 03, 07 and 0A: no PID parameters.
- Mode 06: 00, 21, 31, 85, 98, B2 (base support query plus five advertised monitor IDs).
- Mode 09: 00, 02, 04, 06, 08, 0A, 0B.

There are 51 entries: 19 Mode 01, 16 Mode 02, six Mode 06, seven Mode 09, and three DTC services. Higher support-range queries are already sent by discovery and are not duplicated as periodic jobs.

Mode 09 entries target 1000 ms; all other entries retain the 10 ms target. Availability depends on the ECU; timeouts and negative responses are retained.

Modes 04, 05 and 08 are rejected by this demo's configuration validator. Each array entry is one mode-plus-parameter request, not an automatic scan of all PIDs in that mode. Edit the request hex string to change the PID; add entries to poll additional PIDs (maximum 64 unique requests).

## Timing: target periods, not guaranteed transmission rates

Only ONE diagnostic transaction is active at a time. A fair round-robin scheduler selects due requests. It never interrupts a multi-frame response and never queues a burst of missed requests. On selecting an overdue job it records the delay and skips old periods. Thus 10 ms is a requested schedule, NOT a claim of one completed exchange every 10 ms.

The demo uses functional addressing (`tx_id: "7DF"`, `rx_id: "0"`, 11-bit CAN). Requests are serialized with a minimum 10 ms start-to-start gap. After discovery, PID requests can end early when every discovered ECU's support status is known and all supporting ECUs have completed a terminal response. A valid negative response counts as terminal; NRC 78 does not. Unknown support, no bitmap (03/04/07/0A), no expected responders, or discovery overflow retains the collection window.

The updated JSON and firmware default use `response_timeout_ms: 70`. With no response the timeout is 70 ms from submission. During unknown-responder collection, each matching single frame or first frame restarts the 70 ms window. Multi-frame reception and response-pending states keep the exchange open, subject to the existing 1-second consecutive-frame timeout and 10-second overall limit. Discovery always uses collection timing, not early completion. Timer start is mailbox submission, not a hardware TX acknowledgement; these are tested implementation settings, not a claim of standards compliance. Existing JSON containing 1500 continues to use 1500 for collection/timeouts, so copy the updated JSON too.

For the identified spare ECU only, you can change `tx_id` to `"7E0"` and `rx_id` to `"7E8"` to target that ECU alone. A physical-target transaction ends as soon as its terminal positive/negative response is fully processed. Pending NRC 78 and multi-frame replies still keep it open. This addressing change has not yet been hardware-tested by the new sketch. A timeout still waits `response_timeout_ms`; do not shorten it solely to force an advertised rate. Late replies after a timeout can be ambiguous if an identical request has since been sent; raw frames are retained, but this protocol has no request sequence number on the wire.

ISO-TP flow control uses block size 8, minimum separation 10 ms, a one-second consecutive-frame timeout, and a ten-second overall transaction limit. A nonzero run duration or user stop stops NEW requests; it does not normally truncate an active response. Timing is best-effort cooperative scheduling: SD writes and CAN handling can add latency.

## JSON fields

- `version`: required, 1.
- `bitrate`: default 500000 if omitted; allowed 125000, 250000, 500000 or 1000000. No auto-baud detection.
- `address_bits`: 11 (default) or 29.
- `tx_id` / `rx_id`: hexadecimal strings without `0x`. RX `"0"` selects the standard functional response range. Functional TX must be `"7DF"` for 11-bit or `"18DB33F1"` for 29-bit. A nonzero RX requires the matching physical TX, which is also used for flow control.
- `startup_delay_ms`: 0..600000, firmware fallback 5000 if omitted; the supplied demo config explicitly sets 500 for an already-powered ECU. Copy the updated config.json to the SD card and restart to apply it; no firmware upload is needed. This shortens the deliberate wait by 4.5 seconds but does not remove hardware initialization or support discovery time. Increase this value if the ECU needs more time to boot.
- `run_duration_ms`: 0 (continuous, default), or 1000..86400000 for a timed run. Use the stop button/USB stop command to close a continuous run safely.
- `response_timeout_ms`: 10..10000, default 70; also the response-reloaded collection window when responders are uncertain.
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

Use the updated config.json: 500 kbit/s, 30 jobs, five-second diagnostic delay, continuous duration, functional addressing and the 70 ms response window. Upload with Serial + MTP selected. Power the spare ECU, then Teensy. Run beyond 60 seconds, request a safe stop, and copy the new OBD log through Windows Explorer. Check CONFIG_OK, BUFFERS, CAPTURE_START, normal STOP, CAPTURE_DRAINED, final zero buffered/drop/overrun counters, write failures, and receive sequence continuity. Also compare CAN errors with earlier runs. This test improves and measures capture reliability; it does not change polling into a 10 ms stress test. A separate sequence-numbered STM32 traffic test is still needed to measure end-to-end losses.

## Validation still requiring hardware

Host tests cover zero/nonzero duration, held-button timing and timer wrap in addition to protocol, JSON and capture tests. The sketch builds in Serial + MTP mode. Hardware checks remain: Explorer naming/enumeration, copy a log and compare its hash, upload config and restart, unplug/replug USB, stop while a multi-frame reply is active, file-access-only boot, sustained logging beyond 60 seconds, 250 MB rotation, and SD-full handling. Capture and file transfer intentionally run in separate modes, so simultaneous transfer performance is not claimed.

## Expanded request configuration

The supplied config.json contains 51 explicit requests, including all data PIDs and monitor IDs advertised in the tested Bolero ECU's support maps, plus the original candidate entries. Mode 04 remains button-only; Modes 05/08 remain excluded. Existing firmware supports up to 64 requests; this expansion needs only an SD config update and restart, not a firmware upload.

This is a candidate polling list. Startup discovery follows advertised support ranges and excludes requests confirmed unsupported by every discovered ECU. It does not add measurement requests automatically. Unknown-support requests remain enabled and can time out. Mode 06 now requests five advertised monitor results as well as the first support bitmap. Their returned data has not yet been validated. Mode 09 uses 1000 ms targets; other entries retain the existing 10 ms target. Actual cycle time depends on response completion, collection windows, timeouts and the configured per-job intervals. There is no guaranteed 10 ms update rate for each PID.

## Step 1: startup support discovery

On each normal boot, after the configured startup delay, the logger discovers support maps for Modes 01, 02 (freeze frame 00), 06 and 09 using the configured addressing. Each mode starts at identifier 00; it follows 20, 40, ... E0 only when a responding ECU advertises continuation. Up to eight ECU response IDs are tracked independently, including ECUs returning a negative response (their absent maps remain unknown).

Base queries are sent twice to allow a missed response to be recovered. Additional ranges are retried once if an ECU that advertised that range did not provide its map. Malformed replies, negative replies and timeouts never create an unsupported bitmap. Each query uses the existing diagnostic response window and ISO-TP timeout handling. Repeated valid bitmaps preserve positive support evidence.

Logs contain DISCOVERY_START, DISCOVERY_SEND (mode/range/attempt), DISCOVERY_MAP and DISCOVERY_DONE, followed by DISCOVERY_SUPPORT for each JSON job and ECU. The support status is SUPPORTED, UNSUPPORTED, UNKNOWN, or NO_PID_BITMAP (Modes 03/07/0A). Nonzero freeze-frame numbers remain UNKNOWN in this first implementation. Full received payloads remain in the existing raw and diagnostic records.

Discovery supplies expected-responder sets for faster polling and filters the configured schedule before polling starts. Discovery runs on every restart and adds startup time (at least 560 ms for eight base transactions at the default 70 ms window, plus response collection extensions and advertised ranges). Timed polling duration starts after discovery. A no-response discovery completes with unknown results unless a CAN/SD fault stops the run.

USB serial stop works during discovery and waits for an active transaction before closing the log. Mode 04 waits until discovery completes before transmission. Holding pin 35 at boot still bypasses CAN and enters USB file access directly. For the bench check, power the ECU first, then Teensy, wait for DISCOVERY_DONE and RUN_START, stop normally, and inspect the per-ECU support summary. Hardware validation is still pending.

## Fast-polling bench acceptance

Upload the complete sketch and copy the updated config.json (response_timeout_ms=70) before restarting. Check REQUEST_TIMING for expected_ecus and early_completion. Compare TX_SUBMIT timestamps: request starts must be at least 10 ms apart, complete known responders should end the transaction promptly, and a missing response must time out. Flow-control frames are exempt from the 10 ms request gap so they can be sent promptly. Inspect EXPECTED_RESPONSE_MISSING, ISO-TP errors, drop and SD counters. Test VIN multi-frame responses, negative/pending responses, and stopping while a reply is active. Actual ECU timing and throughput still require hardware validation; the software tests cover timing and responder handling. Very late replies after timeout remain ambiguous on OBD because it has no transaction identifier.

## CAN and diagnostic trace layout

New OBD logs include the OBD-SkillLogger Version 1.0.0 title and explicitly identify elapsed milliseconds since power-on. Calendar time is unavailable; no blank Date field is emitted. Existing log files are not rewritten.

Raw frames use TX-CAN / RX-CAN with Std/Ext ID, Time, Len and hexadecimal bytes. ISO-TP flow-control frames are included as TX-CAN. TX-CAN means accepted into a transmit mailbox, not confirmed wire transmission. Requests have a Mode/PID (or MID)/Frame description and an assembled TX line; assembled positive, negative and pending responses use RX lines. Transport bytes and CAN padding appear only in the raw view. Multi-frame payloads remain on a single logical RX line even up to 4095 bytes; bounded queue fragments prevent large temporary allocations. Remote frames are marked RTR.

Status/error records use the same compact timestamp-first layout. Per-frame CAPTURE_META lines and repeated request/bitrate prefixes are removed; raw frames and aggregate capture/drop/overrun statistics remain. Mode and assembled TX/RX lines are grouped after the raw response; negative responses retain their NRC explanation. Per-frame sequence/microsecond metadata is no longer exported. This is a readable layout matching the reference's main fields, not a claim of compatibility with the reference tool's proprietary import/parser. Vendor license identifiers and Kvaser setup calls are not copied. Serial output uses the same format. Polling, pin 35 file access and pin 40 Mode 04 controls are unchanged.

The log contains no reference-tool branding. New output uses seven-column event timestamps and six-column raw CAN timestamps, matching the supplied sample. Existing files are not rewritten.

## Single readable OBD file

Each recording now creates only **<sequence>_OBD_<factory number>.TXT**, with the requested diagnostic exchange layout. AUTO files are no longer created; existing files on the SD card are left untouched. Full unrelated bus traffic is intentionally no longer saved. The logger still receives/processes it for controller operation and capture health, so captured-frame counters exceed the diagnostic RX lines exported to the file.

The sole log includes diagnostic requests, flow-control frames, response-address frames during active transactions, assembled responses, timeouts, negative-response explanations, button actions, faults and final health counters. Periodic scheduler/debug records go to Serial Monitor only. Stop flushes and closes the OBD file before USB access. Rotation creates a new unused OBD filename before a write would exceed 250 MB. No JSON change is required.

This change does not establish that the previously reported malformed transferred records are resolved. Verify the next OBD file after a clean stop.

## Mode 04 feedback on the OLED

A two-second hold on pin 40 queues the clear. The OLED shows pending, confirmed or unconfirmed. Release the button before another activation.

Success requires an exact assembled `44` reply from every ECU discovered at startup (or the configured physical response ID), with no negative reply or transport error. Discovery overflow or an empty inventory cannot produce a success indication. This conservative inventory can include ECUs that do not support clearing: red means **not confirmed for all expected ECUs**, not that none cleared. Pending `7F 04 78` is not a rejection; the existing diagnostic deadlines still apply. Existing response timing is unchanged.

The log and Serial Monitor record `MODE04_ECU` with each responder ID and response type, followed by `MODE04_CONFIRMED` or `MODE04_NOT_CONFIRMED`. The normal RX lines retain the reply bytes/NRC. Confirmation means the ECU accepted the clearing request, not that the underlying fault was repaired. Validate the OLED states and an accepted/rejected clear on the spare ECU before deployment.


### Supported-PID filtering (spare-ECU bench validation)

After each boot's discovery, every configured request receives one `PID_FILTER` decision in the OBD log and Serial Monitor. `PID_FILTER_SUMMARY` reports configured, enabled and skipped counts. JSON entries and job numbers remain unchanged; this is an in-memory schedule filter, not an edit of the SD configuration.

- Keep a request when any discovered ECU advertises support.
- Skip only when every discovered ECU has explicit unsupported evidence from a valid support bitmap (including a known terminal range).
- Keep requests with missing/incomplete maps, no discovered ECU, inventory overflow, or an unscanned Mode 02 freeze frame. A timeout or negative response alone is never unsupported evidence.
- Modes 03, 07 and 0A have no PID bitmap and remain enabled. Manual Mode 04 is unaffected.

Skipped jobs are never scheduled and do not accrue missed-deadline counters. Final SCHEDULE lines include `enabled=0` for these jobs. Unknown requests still use the normal response timeout. No request spacing or response deadlines change.

The inventory covers responders observed at startup, not ECUs that were silent or appear later. Restart after changing the bench ECU; a full vehicle and late-joining ECUs need separate validation. Current hardware evidence is from a spare ECU only. For the next bench run, compare the filter summary, skipped job submission counts (must be zero), timeout count and per-PID update interval with the previous log. No JSON change is required; upload the updated firmware.


### Expanded Bolero request list: validation limits

This update adds 21 requests: six live-data PIDs, ten freeze-frame PIDs, and five monitor IDs. The new entries are advertised by the tested ECU, not yet proven to return usable measurements. Earlier freeze-frame rejections and Mode 0A rejection remain possible; response-aware backoff is not implemented. PID 01 11 still needs validation due to the observed payload-length anomaly. Original candidates 01 2F, 09 08, 09 0A and 09 0B remain for other ECUs and should be filtered out with the same Bolero maps, leaving 47 enabled requests, not 47 guaranteed successful measurements.

Copy config.json through USB file-access mode and restart. More enabled jobs generally lengthen individual PID polling cycles; the 10 ms target is not a guaranteed per-PID update rate. Review the next parked-vehicle test log to validate the new replies. No other vehicle's support is assumed.

### USB file visibility
The USB MTP listing hides System Volume Information and mtpindex.dat (case-insensitive), including contents of hidden folders. config.json and OBD logs remain visible. These support items stay on the SD card; a direct card reader may still show them. After uploading this firmware, reconnect USB to refresh the listing. This is a display filter, not access control.

## October 2011 data-validation work

The logger now checks selected response layouts before counting a positive reply
as complete. `INVALID_LENGTH` / `INVALID_VALUE` entries retain the original bytes.
`UNVERIFIED_DEFINITION` means a decoder rule has not yet been installed; it is not
an ECU rejection. Mode 01 PID 01 also produces per-ECU readiness explanations after
the RX payload, with separate spark/compression monitor labels.

The current JSON has layout rules for 38 of 51 jobs (74.5%). This is **not** an
overall standards-compliance percentage. Run `test.ps1` to recompute it after
configuration changes. The review notes, remaining gaps and claim boundaries are in
[the OCT2011 review](../docs/J1979_DA_OCT2011_Review.md).
