# Teensy 4.1 manual OBD bench tester

Current target: the user's spare Audi ECU, identity unknown. No STM32, Android app,
Windows configuration tool or JSON loader is required for this first test.

## Open and upload

Open `TeensyOBDTester/TeensyOBDTester.ino` in Arduino IDE. Keep
`DiagnosticCore.h` and `LogReport.h` in the same sketch directory.

Select **Teensy 4.1**, USB type **Serial**, and the Teensy port. Use the Teensy
board package with its FlexCAN_T4 and SD libraries. This sketch was compiled with
Teensy package **1.62.0** using the bundled libraries. No PSRAM allocation is needed.
The existing two 8 MB PSRAM chips can remain fitted.

Upload using Arduino IDE, then open Serial Monitor, set **Newline** or **Both NL & CR**,
and enter commands below. USB Serial uses USB speed; the displayed 115200 selection
is not the CAN bitrate. No firmware has been uploaded to the physical board by
this development session.

## Confirmed hardware

- Teensy CAN1 TX **22** -> TLE9371VSJ TxD.
- Teensy CAN1 RX **23** <- TLE9371VSJ RxD.
- Transceiver STB grounded for normal operation.
- Transceiver VIO 3.3 V, VCC 5 V, shared reference ground.
- SD card in Teensy's built-in slot; use a FAT32 card for the initial test.

Use the ECU's actual documented power, ignition/wake and CAN pins. With the ECU
label missing, this project does not infer its pinout, wake conditions, CAN bitrate
or diagnostic protocol. Correct wiring and power are prerequisites; a silent ECU
does not prove the request is unsupported.

## First session

On startup, a new log such as `OBD0000.TXT` is created and passive bitrate detection
starts. Each candidate is observed for two seconds: **500000, 250000, 125000,
1000000 bit/s**. Three received data frames select a candidate.

Detection sends **no diagnostic probes and no ACKs while listening**. A standalone
ECU may be silent or stop transmitting without another acknowledging node. In that
case, `AUTO_NO_LOCK` is expected. The sketch remains listen-only and does not guess.

After `AUTO_LOCK`, begin with:

```text
01 00
01 0C
01 05
03
09 02
```

Enter **one line at a time** and wait for `REQUEST_END`. Start with supported-PID
discovery (`01 00`) and request supported parameters before trying others.
Unsupported requests may produce a negative response OR no reply.

If the ECU stays silent, use a bitrate you intend to test, for example:

```text
baud 500000
addr 11
01 00
```

This is an explicit manual selection, not a detected or verified rate. Hardware
ACK errors, wrong rate, sleeping ECU and wrong addressing can all prevent a reply.

## Commands

Commands are lowercase; hexadecimal diagnostic bytes accept either case.
Use space-separated hex bytes without a `0x` prefix.

- `help`: print available commands.
- `status`: rate, addressing, SD file and overflow counters.
- `date YYYY-MM-DD HH:MM:SS`: enter current local date/time when idle. Valid years
  are 2000..2099. Set it each session; it is not saved to a battery-backed RTC.
- `auto`: repeat passive detection; allowed when idle.
- `baud 500000`: manually select 125000, 250000, 500000 or 1000000.
- `addr 11`: standard functional request 7DF, responses 7E8..7EF (default).
- `addr 29`: extended functional request 18DB33F1, responses 18DAF1xx.
- `target 7E0 7E8`: physical request/response pair in the selected address format.
- `functional`: restore the functional address for the selected format.
- `stop`: cancel the request and scanning, clear pending transmissions, listen only.
- `close`: stop, drain and close the log; wait for LOG_CLOSED before removing SD.
  Reset Teensy to create another log.
- `01` or `09` alone: shorthand for `01 00` or `09 00`.
- `01 0C`: engine RPM; `01 05`: coolant temperature; `01 0D`: speed.
- `02 0C 00`: example freeze-frame RPM request, frame zero.
- `03`, `07`, `0A`: stored, pending, permanent emission-related DTC requests.
- `09 02`: VIN request, if supported.

Modes 01..0A can be entered, but this is a manual payload tester, **not** a complete
ten-mode interpreter or J1979 conformance implementation. Modes 05/06/08 need the
correct service-specific identifiers and semantics for the selected transport and
standard edition. Mode 05 is not the normal oxygen-monitor service on CAN.
Mode 04 clears diagnostic information; Mode 08 requests control actions. They
execute only if you explicitly type their request; neither is sent automatically.
The initial read tests above do not use either mode.

The tester accepts requests up to seven diagnostic bytes (one ISO-TP single frame).
For 01/06/09 use one PID/MID/info type. Mode 02 requires PID and frame number.
The firmware adds the transport length and zero padding; do NOT type those yourself.

## What is captured

Logs are now readable **.TXT reports**, with exactly two introductory fields:

```text
OBD-SkillLogger Version 1.0.0
Date:
```

There is no vendor copyright, UTC Date, license information, driver version or
snapshot path in this header. The blank date reserves 19 spaces for a later update.
No date or ECU information is copied from the reference logs.

To fill in the date, enter your **current local time**, for example:

```text
date 2026-10-15 09:43:10
```

The example is illustrative: substitute the actual time. The command updates the
fixed-size header in place without overwriting report data and estimates the log
start time from elapsed uptime. Request sections then show their local time.
The date stays blank until explicitly set; neither compile time nor an unverified
RTC is treated as current time. Setting it again corrects the header and future
request times, but does not rewrite earlier request timestamps. Sessions should
be shorter than the 49.7-day millisecond rollover period.

Each manual request starts a **mode section**, and matched responses are grouped
under their actual CAN ECU identifier. The logger does not label an ECU as
ECM/EngineControl just because its response ID is 7E8.

Supported identifier bitmaps for modes 01, 02, 06 and 09 become readable rows.
They describe only the ECU and range actually queried; this does not automatically
scan all modes or infer support from missing responses. Mode 02 includes its
freeze-frame number. Common live/freeze-frame PID values (load, temperatures,
pressure, RPM, speed, airflow, throttle and voltage) are decoded. CAN DTC lists
show code numbers; descriptions not installed are explicitly labelled. Mode 09
VIN and ECU-name responses have a limited ASCII presentation.

This is not the full Silver Scan-Tool interpretation database. Unknown identifiers,
complex monitor/readiness data and unimplemented decoders retain raw response bytes.
Raw events remain in the same text file and Serial Monitor, showing uptime,
request number, CAN identifier, event, detail and configured bitrate.

- `TX_SUBMIT`: request placed in a hardware mailbox; this alone does not prove ACK.
- `FLOW_TX_SUBMIT`: ISO-TP flow-control frame submitted.
- `RX` / `RX_REMOTE`: raw bus traffic, including unrelated and unmatched frames.
- `POSITIVE`: matching complete diagnostic payload.
- `NEGATIVE`: complete 7F/service/NRC reply.
- `RESPONSE_PENDING`: NRC 78; continue waiting, with a bounded overall deadline.
- `NRC`: a short name for known codes, or an explicit unmapped-code description.
- `NO_RESPONSE_TIMEOUT`: no matched reply; never fabricated as a negative response.
- `ISOTP_TIMEOUT`, `ISOTP_SEQUENCE_ERROR`, `RESPONSE_DEADLINE`: incomplete transport.
- `CAN_ERROR`: controller status, including ACK/CRC and transmit/receive counters.
- `REQUEST_END`: collection window finished.

Complete payloads are printed in 24-byte chunks with a total length and offset,
so long replies remain readable. Raw CAN bytes are retained independently.
The request number on raw traffic means the current test interval, not proof that
the frame belongs to that request. Only matched POSITIVE/NEGATIVE events make
that association. Same-mode late responses cannot be perfectly correlated because
OBD has no transaction identifier on the bus.

## Transport and logging limits

Classical CAN only; normal ISO-TP addressing in 11- or 29-bit CAN identifiers.
No CAN FD, K-line, extended-address-byte ISO-TP or UDS discovery is implemented.
A detected CAN bitrate does not prove that the ECU supports standard OBD.
It may require a different address, wake condition or diagnostic protocol.

Up to eight responding ECU contexts, each with a 4095-byte ISO-TP receive buffer.
Each receives flow control on its physical request ID, block size 8 and STmin 10 ms.
Sequence checks, block renewal and sequence-number wrap are implemented. Custom
physical addressing uses the configured TX ID for flow control.

Initial response collection: **1500 ms**. Consecutive-frame gap: **1000 ms**.
Overall request deadline including response-pending: **10000 ms**.
These are bench-test timeouts, not a claim of standard-prescribed timing.
The first version has no automatic diagnostic retries or polling.
CAN hardware may retry an unacknowledged frame during the bounded transaction;
controller reinitialization clears pending hardware transmissions at end/stop.
A negative response is recorded, not automatically retried.

RX interrupts only copy frames to a bounded buffer. SD and USB processing occurs
in the main loop. SD logging and console output are separately buffered; the
console may drop output if the host cannot keep up, and status reports the count.
Disconnected Serial Monitor does not stop SD recording. Reconnect and type help.
This is for sparse diagnostic testing, not guaranteed lossless full-bus logging.

An SD initialization/write/flush failure prevents further requests. Receive or
log queue overflow is reported and also prevents new requests until reset.
A log can lose its final buffered records on sudden power loss. Use close to
finish cleanly. No existing log is overwritten; after OBD9999.TXT, free/archive
logs before restarting.

## Verification

- Host tests exercise parsing, ID matching, positive/negative/pending responses,
  absent response, multi-frame reassembly, flow-control blocks, sequence wrap and
  errors, custom IDs, 29-bit flow control, time rollover and send failure.
- Report tests verify the exact header, fixed-size date update layout, invalid dates,
  leap years, supported PID rows, decoded values, DTCs and malformed replies.
- Teensy target compile: successful with board package 1.62.0.
- An upstream FlexCAN_T4 string-copy warning is emitted by the CAN error decoder;
  the sketch uses numeric ESR bits for bus-off detection.
- Not yet validated: physical ECU communication, observed bitrate, actual SD
  reliability under traffic, electrical timing, or live ECU responses.
- Local build files are excluded from Git.

Run `test.ps1` from PowerShell to reproduce host checks and the Teensy compile.
It uses desktop g++ and Arduino IDE's bundled arduino-cli by default.

## References

- [FlexCAN_T4](https://github.com/tonton81/FlexCAN_T4)
- [Teensy 4.1 hardware](https://www.pjrc.com/store/teensy41.html)
- [SAE J1979](https://saemobilus.sae.org/standards/j1979_201202-e-e-diagnostic-test-modes)
- [SAE J1979DA](https://saemobilus.sae.org/standards/j1979da_202510-j1979-da-digital-annex-e-e-diagnostic-test-modes)

Full licensed standard editions have not been supplied or audited. This initial
tool captures exchanges for investigation; it does not claim SAE certification.
