# OBD Logger System Design

Revision: 0.2
Date: 28 October 2026
Status: For review

## 1 Product overview

The current product is a Teensy 4.1 logger that reads a JSON configuration from SD, discovers OBD support, requests diagnostic data over classical CAN, and records readable diagnostic exchanges. Windows retrieves closed log files through USB MTP without removing the SD card. A physical button provides deliberate Mode 04 operation. Four external LEDs report operating status.

Included in this baseline are the automatic logger, the existing manual tester as a development aid, SD configuration, USB access, and the separate STM32 Stage 1 four-responder test fixture. The simulator is test equipment, not shipped logger functionality.

Future scope includes a Windows configuration application, RTC calendar-time integration, proactive low-space warnings, and further robustness work. Android is no longer planned. Wireless access, generic UDS diagnostics, OEM ABS/wheel data, XCP/CCP, CAN FD, J1979-2/-3 and ISO 27145 are outside this implemented baseline. The current protocol scope must not be advertised as support for every vehicle module or every OBD service.

## 2 Product V model

The left side defines the product from operator needs down to module design. Implementation connects the two sides. The right side checks the completed product from individual modules up to the operator workflow. Each paired stage must have an agreed expected result and recorded evidence.

The diagram defines the planned review structure; it does not indicate that every stage has passed.

V-model diagram is embedded in the Word document.

## 3 Product needs

- UN-01 Record supported diagnostic data without typing individual requests in Serial Monitor.
- UN-02 Preserve understandable evidence of requests, responses and failures.
- UN-03 Retrieve files and update configuration without removing the SD card.
- UN-04 Understand operating state and deliberately request diagnostic clearing.
- UN-05 Handle multiple diagnostic responders without mixing their data.
- UN-06 Establish repeatable evidence before product release.

## 4 System requirements

The following shall statements are proposed baseline requirements derived from current behavior and the agreed project direction. Implementation evidence is not equivalent to completed acceptance testing. Test and design identifiers connect the requirements to the corresponding sections below.

### R01 Configuration control

The logger shall load config.json at boot and block diagnostic transmission if configuration is missing, malformed, oversized or invalid. Accept version 1, at most 8192 bytes, and 1 to 64 unique jobs. Reject unknown keys, invalid addressing/timing and automatic Modes 04, 05 and 08. Edits shall take effect after restart. Trace: UN-01; D01; UT01 and ST01. Status: implemented; hardware rejection/restart evidence pending.

### R02 CAN and diagnostic interface

The logger shall use the configured bitrate and addressing and retain each response's CAN ID. The supplied profile shall use 500000 bit/s, 11-bit functional TX 7DF and standard OBD response selection. Configured 29-bit and physical addressing paths require separate hardware qualification. Trace: UN-01/05; D02; UT02, IT01 and ST02. Status: implemented; historical single-responder 11-bit evidence exists.

### R03 Discovery and support filtering

The logger shall discover support for Modes 01, 02 frame 00, 06 and 09, tracking up to eight responders. It shall keep a job if any responder supports it and skip only with explicit unsupported evidence for every inventoried responder. Missing maps, no responders and inventory overflow shall preserve unknown jobs. Timeouts/negative replies alone shall not prove lack of support. Trace: UN-01/05; D03; UT03 and IT01. Status: implemented; four-responder hardware test pending.

### R04 Scheduling and timing

The logger shall have at most one active diagnostic transaction, use fair selection among due jobs, and maintain at least 10 ms between diagnostic request submissions. Flow control is exempt. It shall skip expired polling opportunities rather than replay a backlog, and report missed periods. Ten milliseconds is a target interval, not a guaranteed per-PID refresh rate. Trace: UN-01; D04; UT01/02 and IT02. Status: implemented; operating envelope requires measurement.

### R05 Response handling

The logger shall distinguish complete positive replies, negative replies, pending replies, transport errors and no response. It shall assemble multi-frame data separately by responder. Defaults are a 70 ms response collection window, 1000 ms consecutive-frame timeout and 10000 ms overall transaction limit. Known complete responder sets may finish early; pending replies shall not count as completed success. Trace: UN-02/05; D02; UT02 and IT02. Status: implemented; multi-responder transport stress coverage pending.

### R06 Diagnostic records

The logger shall create OBDxxxx.TXT files containing raw diagnostic CAN frames, assembled requests/replies, event times, faults and health counters. New runs shall not create AUTO files. Unrelated background traffic is not required in the exported text. TX records shall be understood as mailbox submission, not proof of physical delivery. Trace: UN-02; D05; UT04 and ST02. Status: implemented; independent-capture acceptance remains open.

### R07 Time interpretation

Current logs shall explicitly identify elapsed milliseconds since power-on and unavailable calendar time. Rotation shall preserve the same elapsed-time origin. RTC-derived dates shall not be represented as implemented. The 32-bit millisecond counter wraps after approximately 49.7 days; extended-time presentation is an open requirement. Trace: UN-02; D05; UT04 and ST03. Status: implemented for elapsed time only.

### R08 Capture and faults

The automatic logger shall verify external-memory allocation before starting CAN operation, use bounded capture/log queues, and stop new requests on capture overflow, hardware overrun, logging failure or bus-off. It shall preserve existing files and report available fault evidence; storage failure may prevent final records from being saved. No automatic restart is required. Trace: UN-02/06; D05/D06; UT05 and ST04. Status: implemented paths; complete fault-injection evidence pending.

### R09 Continuous logging and rotation

With run_duration_ms=0, the logger shall run until stop or fault. Every file, including its header, shall remain at or below 250000000 bytes; rotation shall select the next unused filename without overwriting existing logs. Exhausted storage or filenames shall stop recording. Trace: UN-02; D05; UT06 and ST03/04. Status: implemented; full-size hardware rollover pending.

### R10 Safe stop and file access

A three-second hold on pin 35, or stop/close over serial, shall stop new requests, allow the active exchange to end within its deadlines, drain buffers and close the log before exposing SD over MTP. Holding pin 35 at boot shall enter file-access mode without starting CAN. USB shall identify as OBD logger; SD shall be hidden during capture. Restart is required to resume. Trace: UN-03; D06; UT07 and ST05. Status: implemented; complete Windows acceptance pending.

### R11 Deliberate Mode 04 control

A debounced two-second hold on pin 40 shall queue one Mode 04 request during normal logging. Release shall be required before another request; a held-at-boot button shall require release. Stop/fault shall take priority. Confirmation shall require an exact assembled 44 response from all expected responders, without transport/negative-response failure; unknown or overflowing inventory shall not give all-ECU success. Trace: UN-04; D07; UT08 and ST06. Status: implemented; spare-ECU acceptance evidence is partial.

### R12 Operator feedback

Pins 4/5/6/7 shall indicate communication established, recording, USB access and alert respectively. Mode 04 shall give three slow yellow flashes for confirmation or three quick red flashes for incomplete/rejected confirmation. A persistent fault shall override temporary feedback. The onboard LED shall indicate reply activity/button hold or fault as implemented. Trace: UN-04; D07; UT08 and ST06. Status: implemented; full physical indication checks pending.

### R13 Reproducible validation

Each future acceptance run shall record firmware commit and binary hash, configuration hash, board/ECU identity, test procedure, independent capture where relevant, raw-file hashes and deviations. A test shall not be marked passed without recorded actual results. Trace: UN-06; D08; AT01. Status: proposed process; older logs have incomplete provenance.

## 5 Architecture and interfaces

### D01 Configuration boundary

Config.h validates JSON into bounded jobs. The current supplied profile contains 51 entries: 19 Mode 01, 16 Mode 02, six Mode 06, seven Mode 09, and one each for Modes 03, 07 and 0A. These are requests, not 51 distinct live-data PIDs. Mode 09 targets 1000 ms; other entries target 10 ms. Discovery adds its own support queries. Enabled count depends on observed ECU maps; the supplied Bolero maps would retain 47 entries.

### D02 CAN and transport boundary

Teensy 4.1 CAN1 TX22/RX23 connects through TLE9371VSJ, with STB held low. Firmware uses classical CAN and OBD service requests over ISO-TP. DiagnosticCore.h owns response matching, reassembly, flow control and deadlines. Response bytes remain authoritative when decoding is absent or uncertain. Positive responses do not automatically validate every decoded measurement.

The prototype uses USB power from a laptop or power bank. OBD power-input conditioning is not part of this reviewed baseline. Hardware assembly must use the selected transceiver's correct supply/interface levels and a common signal ground. Bench termination must be checked separately from the already terminated vehicle bus. Final automotive power, EMC, environmental and connector qualification are not established.

### D03 Discovery boundary

Discovery.h tracks support maps per responder and mode. Base support queries are attempted twice; advertised higher ranges are followed with bounded retry. Filtering changes the in-memory schedule, not the SD JSON. Inventory reflects startup observations only. A late-joining ECU is not guaranteed to be represented until restart.

### D04 Scheduler boundary

Schedule.h selects due enabled jobs round-robin. The main sketch starts a request only when the transport is idle and the minimum gap has elapsed. A full schedule cycle depends on response times, collection windows, negative replies, timeouts and SD latency. Faster CAN bitrate alone does not guarantee faster ECU processing.

### D05 Capture and storage boundary

The receive callback copies frames into a 65536-slot external-memory ring. Main-loop processing handles diagnostics and formats records into a separate 4096-entry text queue. Batched SD writes reduce overhead. The callback does not format text or write the SD card. Both 8 MB PSRAM chips are part of the prototype hardware; automatic firmware checks that its allocations actually reside in external memory.

CaptureRing.h implements buffering; TraceFormat.h defines readable records; LogReport.h supplies supported decoding helpers; LogRotation.h defines the file-size rule; the sketch owns file creation, writes and shutdown. Rotation can split an exchange or long logical text record across files. Analysis must read consecutive segments in order. Counters are evidence of detected faults, not independent proof that no frame was lost.

### D06 Run control and USB boundary

The conceptual lifecycle is boot, SD/configuration validation, PSRAM validation, CAN startup, startup delay, discovery, filtering, polling, stopping, file access. Errors branch to fault/closure. These are descriptive states; the sketch represents them through flags and helpers rather than a single state enumeration.

RunControl.h handles button qualification. The sketch coordinates stopping and MTP visibility. usb_identity.c supplies device naming; src/mtp contains the locally adapted responder. MTP is portable-device access, not a Windows drive letter. Its visibility filter hides support files but is not a security boundary. JSON is applied only after reset.

### D07 Controls and feedback boundary

Buttons connect pin 35 and pin 40 to ground using internal pull-ups. External LED outputs are pins 4 green, 5 yellow, 6 blue and 7 red, each with its own current-limiting resistor. Final resistor values and assembled wiring must be recorded in the hardware baseline. Green remains on after initial communication; it is not a live disconnection watchdog. Red has no advance SD-space warning in this version.

Mode4Feedback.h evaluates per-ECU outcomes. Functional Mode 04 can affect more than one ECU, clear stored emissions diagnostics and reset readiness. Confirmation means acceptance of the clear request, not repair of the underlying fault. This test belongs on a deliberately prepared spare-ECU setup.

### D08 Test fixture and evidence boundary

The separate STM32F407G-DISC1 Stage 1 simulator uses one CAN controller to represent four logical OBD responders, 7E8 to 7EB. It does not emulate four independent electrical nodes or real ABS/transmission semantics. Its initial scope is single-frame support/live-data responses. The existing manual tester remains useful for isolated transactions.

Simulator expectations are 7E8 RPM 1000 and coolant 80 C; 7E9 RPM 2000 and speed 25 km/h; 7EA coolant 90 C; 7EB speed 50 km/h. The five-job simulator configuration should retain four jobs and filter 01 2F. It must never be connected as a transmitting fixture to a real vehicle.

## 6 Detailed design decisions

- Serialized transactions reduce reply ambiguity. OBD has no transaction sequence number; a very late reply to an identical repeated request can remain ambiguous.
- Known responder completion reduces waiting. Unknown support retains the collection window to avoid prematurely ending reception.
- The response window defaults to 70 ms and can reload on matching response starts. It is not a fixed 70 ms wait after every successful request and is not a documented standards-compliance claim.
- File access follows closure to avoid host edits while firmware is recording. USB connection alone does not stop logging.
- Existing filenames are preserved; 10000 possible OBDxxxx names bound the namespace.
- Startup delay is 500 ms in the supplied JSON, but the parser fallback is 5000 ms if omitted. Initialization and discovery add further time before periodic polling.
- The toolchain baseline is Teensy board package 1.62.0, ArduinoJson 7, and Serial plus MTP Disk Experimental USB selection. Requalify the locally adapted MTP code when upgrading the core.
- Current software does not guarantee loss-free operation at an unspecified bus load, indefinite power-loss resilience, all-vehicle compatibility, or commercial readiness.

## 7 Implementation and build control

The automatic logger runs on Teensy 4.1 with external PSRAM, TLE9371VSJ and the built-in SD interface. Keep the original manual tester and STM32 fixture separate from production firmware.

Build the complete sketch with its headers, USB identity file and local MTP source. Use the controlled board-package and library versions, compile the firmware, run relevant module tests, and record firmware and configuration hashes for each test release.

The current reviewed software baseline is commit 462b8bf. Hardware assembly, firmware build success and software tests are distinct evidence; none alone proves successful operation in a vehicle.

## 8 Unit testing

- UT01 covers R01/R04: test_config.cpp checks parsing, limits and scheduling. Pass when valid profiles load, invalid profiles fail, due jobs remain fair and overdue jobs do not produce catch-up bursts.
- UT02 covers R02/R04/R05: test_core.cpp checks addressing, matching, transport and timing. Pass when overlapping transactions are rejected, payloads match expected bytes, negative/pending outcomes differ and all deadlines terminate correctly.
- UT03 covers R03: test_discovery.cpp and test_filter.cpp. Pass when per-ECU maps remain separate, unknown support is retained and only conclusively unsupported jobs are disabled.
- UT04 covers R06/R07: test_trace.cpp. Pass when raw/assembled formatting and elapsed-time declarations match the specified format without foreign branding or fabricated dates.
- UT05 covers the queue part of R08: test_capture.cpp. Pass when queue order, capacity and overflow behavior match expectations. Hardware ISR/SD interaction still requires ST04.
- UT06 covers rotation arithmetic in R09: test_log_rotation.cpp. Pass at below-limit, exact-limit and exceeding-limit boundaries. Filesystem behavior requires ST03.
- UT07 covers R10: test_run_control.cpp and test_mtp_visibility.cpp. Pass when button timing and visibility rules hold. Real enumeration/copy behavior requires ST05.
- UT08 covers R11/R12: test_mode4_feedback.cpp plus test_core.cpp and test_run_control.cpp. Pass for all-confirmed, partial, negative, pending, malformed, cancellation and held-button cases within the tests' actual scope.

All listed files are under teensy-obd-sd-logger/tests. The test.ps1 script runs the host tests and Teensy compilation. Retain dated output and the tested revision when rerunning it for release evidence.

## 9 Integration testing

- IT01 covers R02/R03/R05: run stm32-obd-four-ecu-stage1/test.ps1, then the physical bench procedure in that project's README. Pass hardware acceptance when all four IDs and exact values appear, configured=5/enabled=4/skipped=1, and 01 2F has zero periodic submissions. Retain an independent CAN trace and investigate any missing/extra frames.
- IT02 covers R04/R05: use a fixture capable of controlled delayed, pending, malformed and multi-frame replies. Check 10 ms request spacing, per-ECU assembly, missing-responder handling, stop during reception and deadline termination. Stage 1 cannot yet generate all these cases; fixture expansion or replay tooling is required. Pass only with exact expected payloads/outcomes and no unexplained discrepancy.

## 10 System testing

- ST01 covers R01: try missing/truncated JSON, invalid modes, duplicate jobs and invalid IDs. Observe the bus independently; pass when no diagnostic requests are transmitted and the fault is indicated. Replace configuration through file access and confirm restart applies it.
- ST02 covers R02/R06: perform a controlled stationary vehicle run with an independent CAN recorder. Align captures, distinguish submitted TX from on-wire TX, and reconcile diagnostic RX frames and assembled payloads. Pass when all in-scope differences are explained and correct; zero software drop counters alone are insufficient.
- ST03 covers R07/R09: run until at least two file rotations. Verify every segment is at most 250000000 bytes, old files are unchanged, time origin continues, and reconstructed exchanges match independent capture across boundaries. Also exercise millisecond wrap with a controlled clock test. Hardware execution pending.
- ST04 covers R08/R09: inject SD-full/write failure, filename exhaustion, absent PSRAM, capture pressure and CAN faults on a bench. Pass when new requests stop and observed failure behavior matches the requirement without overwriting files. Record what evidence remains if SD writes are unavailable. Abrupt power removal is a separate characterization test, not an existing guarantee of complete final records.
- ST05 covers R10: while logging, verify SD is not listed; stop and wait for closure; copy logs through Windows and compare SHA-256 against direct-card reads. Check OBD logger naming, reconnect, boot-held access and configuration upload/restart. Pass when bytes match and no CAN requests occur in file-access-only boot.
- ST06 covers R11/R12: on a prepared spare ECU, test short/long holds, boot-held release, repeated press, simultaneous stop and positive/negative/missing responses. Confirm physical LED patterns and log outcome agree. Pass only with no unintentional clear and no false all-ECU success.

## 11 User acceptance

AT01 covers UN-01 through UN-06 and R13. The operator configures the unit, records a controlled session without Serial Monitor, interprets LEDs, stops safely, retrieves logs and identifies requested data and rejected requests. The lead checks evidence and intended-use suitability. Pass requires signed review of the agreed scope and disposition of open defects; commercial release is not implied by one successful vehicle session.

For each execution record: test ID, date/time source, operator, hardware revision, ECU/vehicle, firmware and config hashes, setup, steps, expected result, actual result, evidence locations/hashes, pass/fail, defect IDs and reviewer. A failed test shall produce a defect record and a traceable retest.

## 12 Existing test results

The five existing reports are historical observations, not acceptance records for commit 462b8bf. Their raw logs do not contain an exact firmware commit, so they cannot prove that this full baseline was tested.

- Test01_SpareECU_Mode04.md: Mode 04 confirmation observed; a later CAN communication fault stopped the run. Partial pass only.
- Test02_SpareECU_Startup.md: startup/capture evidence; unanswered requests remain.
- Test03_SpareECU_PIDFiltering.md: filtering/capture evidence; repeated timeouts remain.
- Test04_Bolero_Run01.md: one responder 7E8, 26 of 30 entries retained, zero no-response timeouts, five request types rejected. Median RPM request spacing 440 ms.
- Test05_Bolero_Run02.md: repeat single-responder evidence with the same filter result and rejection types. Median RPM request spacing 440 ms.

Reports reside in docs/test-reports and include source hashes. Their smaller files do not validate 250 MB rotation. One responding OBD address does not mean the vehicle contains only one ECU. Two reported clean runs do not close capture-integrity risks for all traffic loads.

The later third Bolero/Kvaser comparison is not one of these five controlled reports. The preliminary comparison identified diagnostic-frame differences and a bus-off event. Its raw evidence, root cause and retest should be formally recorded before closing ST02; it must not be silently treated as a successful loss-free test.

## 13 Planned product additions

The recent RTC battery-backup schematic is a related hardware proposal, not integrated calendar logging in this firmware baseline. It uses a TP4056 charger, MCP1700 3.0 V supply to Teensy VBAT, thermistor monitoring and DW01A/FS8205 protection. Exact schematic revision, battery specifications, electrical review and bench results must be attached before integration approval. RTC backup powers the timekeeping domain; it is not a logging power hold-up solution.

Future RTC requirements must define setting time, timezone/UTC policy, invalid-time indication, backup duration and acceptable drift. Future SD warning requirements must define warning reserve, operator indication and stopping policy. The Windows tool must define schema compatibility and validation before replacing manual JSON editing. These are open decisions, not accepted numerical requirements.

## 14 Product risks and actions

- RK01 Capture integrity: reconcile the independent CAN traces, reproduce missing-frame conditions and record a defect retest before closing ST02 and ST04.
- RK02 ECU inventory: test silent and late responders and define whether rediscovery is required. Close with reviewed behavior and IT01/IT02 evidence.
- RK03 Request load: approve polling profiles using measured response times and per-PID update rates. Do not promise a universal 10 ms refresh rate.
- RK04 SD capacity and power interruption: define advance-warning behavior and characterize data retained after abrupt power loss. Complete ST03/ST04.
- RK05 Accidental clearing: verify button hold/release and stop priority; preserve pre-clear diagnostics. Complete ST06 on a prepared spare ECU.
- RK06 Decoder assumptions: resolve the Bolero Mode 01 PID 11 payload-length anomaly before relying on decoded throttle position.
- RK07 Hardware readiness: finalize supply protection, enclosure and thermal limits, RTC battery specifications and the hardware revision. Record electrical bench results.



## 15 Release and maintenance

Release requires an approved scope and requirements baseline, reviewed design and hardware revision, passing relevant unit/build/integration/system tests, completed operator acceptance, and no unresolved defect that invalidates the intended release claim. Exceptions require an explicit reviewer decision with impact and restricted use recorded.

For each subsequent change, identify affected requirement/design/test IDs; implement the smallest coherent change; run the relevant regression checks; update evidence; then review and approve the release. Preserve raw evidence with hashes and control access to VIN/vehicle information. Do not upload identifiable raw logs automatically.

Record release firmware/configuration hashes, dependency versions and known limitations. Maintenance includes reproducing reported faults, controlled fixes, regression tests and retained release history. Replace this draft's status only after approval. Publication follows product-owner approval.

## 16 Review and approval

Product owner: Pending

Technical lead: Pending

Hardware reviewer: Pending

Verification lead: Pending

Approval date: Not assigned

Review the product scope, R01 to R13, test criteria, remaining risks and planned features. Record corrections and agreed responsibilities before approving the design baseline.
