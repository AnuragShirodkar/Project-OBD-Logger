# OBD and Kvaser comparison — 26 November 2026

## Conclusion

Both supplied OBD logs stopped with `Logging or CAN failure; reset required`
after one hardware receive-overrun indication. This is the same recorded stop
trigger as the preceding CAN-mode investigation. Neither run reported an SD
write failure, a software capture drop or a log queue drop. Continuous recording
therefore remains a failed test even though the saved diagnostic frames match.

The underlying receive-service problem is not established by these files.
The internal-RAM staging proposal and timing investigation in the CAN report
also apply to this shared capture path; neither has been implemented here.

## Files and comparison method

Inputs: `E:/LOGS/01_OBD_1757338.TXT`, `E:/LOGS/02_OBD_1757338.TXT`, and
`C:/Users/LENOVO/Desktop/OBD_1.asc`. Original files were not modified.
Snapshots were parsed locally. Compared ordered CAN IDs, DLC and payload bytes
from the logger's raw `TX-CAN`/`RX-CAN` records against Kvaser's diagnostic
traffic. Kvaser direction labels are observer-relative and not used to infer
logger direction. Different recording origins prevent comparing absolute times
directly. Decoded TX/RX lines were not counted again as raw CAN frames.

Kvaser contains **571,517 CAN frame rows** in total. Restricting it to the actual
diagnostic IDs `7DF`, `7E0` and `7E8` gives **39,925 frames**. The two logger
files contain **368 + 39,557 = 39,925 raw diagnostic frames**. Their ordered
streams match the entire filtered Kvaser stream in two consecutive blocks:

- OBD `01`: all 368 frames match diagnostic positions 1–368 in Kvaser.
  Logger times 3.986–7.929 seconds after power-on correspond to Kvaser times
  5.410960–9.351480 seconds. Raw directions: 176 TX and 192 RX.
- OBD `02`: all 39,557 frames match diagnostic positions 369–39,925 in Kvaser.
  Logger times 3.992–304.836 seconds after power-on correspond to Kvaser times
  23.799440–324.636730 seconds. Raw directions: 18,787 TX and 20,770 RX.

The logger TX counts include ISO-TP flow-control frames to `7E0`, as well as
functional diagnostic requests to `7DF`. Do not mistake flow-control frames for
extra PID requests. `7E8` is the sole diagnostic responder in these recordings;
other background CAN IDs are not additional demonstrated OBD responders.

This proves equality of the saved diagnostic frame streams, not lossless
recording of all background bus traffic. In the current source, OBD raw RX trace
export is limited to accepted response IDs while a diagnostic transaction is
active. The `seen`/`drained` counters also count background traffic processed by
the shared capture queue. Their much higher totals are not numbers of exported
diagnostic frames. The recorded hardware overflow still prevents a general
lossless-capture claim at the stop boundary.

## Fault counters

- OBD `01`: stop uptime **7,937 ms**; first-overrun uptime **7,931,002 us**;
  `seen=6437 drained=6437 buffered=0 peak=24 capacity=65536 hw_overruns=1`;
  SD queue peak 67/4,096; 14 writes; maximum write 2,649 us;
  `write_failures=0 unwritten_bytes=0`.
- OBD `02`: stop uptime **304,842 ms**, approximately five minutes;
  first-overrun uptime **304,837,002 us**;
  `seen=449094 drained=449094 buffered=0 peak=105 capacity=65536 hw_overruns=1`;
  SD queue peak 67/4,096; 1,329 writes; maximum write 20,964 us;
  `write_failures=0 unwritten_bytes=0`.

Both have `RXdrops=0 logDrops=0 consoleDrops=0`, overrun flags `000000E0`,
ESR `00040080`, and final `CAPTURE_DRAINED`. One overrun indication is not
an exact count of lost wire frames. Maximum interrupt cycle values are
1,073,280,420 and 1,073,590,748 at a reported 600 MHz, again near the suspicious
1.79-second nominal duration seen in CAN mode. Validate this instrumentation
before claiming a measured handler stall. These files do not establish a fix.

## Discovery and requests

Both headers report firmware `1.2.0-dev4-idle-sd`, device `OBD_1757338`, config
fingerprint `BBCB677B`, 500 kbit/s, 11-bit addressing and functional TX `7DF`.

Each configuration has **64 request jobs**. Discovery keeps **47** and skips
**17** as unsupported by all discovered ECUs. Of those retained, 44 are marked
supported and three have no PID bitmap (Modes 03, 07 and 0A). Retention of a
service without a bitmap is not proof that the service is supported.

Requests cover Modes **01, 02, 03, 06, 07, 09 and 0A**, including support-map
discovery, current values, freeze-frame values, fault-code services, monitoring
results and vehicle information. No Mode 04 clear request was found.
There are **154** diagnostic request payloads in OBD `01` and **16,112** in OBD
`02`, including discovery. Each has a corresponding reassembled response trace;
this does not mean all responses passed validation or were positive.

The requested 10 ms intervals are scheduling targets, not achieved per-PID
periods. In the longer run, most enabled 10 ms jobs were submitted about 346–347
times, roughly once per 0.86 seconds over the approximately 299-second polling
phase. The schedule reports about 29,588 skipped periods for these jobs. Mode 09
jobs were submitted 299 times. One active request at a time and response handling
limit the achievable rate; do not advertise these results as every PID at 10 ms.

## Response warnings separate from the recording failure

1. Mode 01 PID 11 received payload `41 11 00 00`: two data bytes rather than
   the one required by the firmware's reviewed PID-11 layout.
   Mode 02 PID 11 frame 00 received `42 11 00 00 00`, also with an extra data
   byte. These responses produce `INVALID_LENGTH`, then
   `EXPECTED_RESPONSE_MISSING` because no valid response completed that request.
   The first run has 3 + 3 such cases; the second has 347 + 346.
   Kvaser contains the identical raw bytes, so this length problem was present
   on the bus rather than introduced by log formatting. Correct the simulation
   response definition if this test uses a simulator; do not weaken validation
   merely to accept the malformed response.
2. Mode 0A returns `7F 0A 11`: a negative response, service not supported.
   This occurs twice in the first run and 346 times in the second. The negative
   response is logged correctly; it is not evidence of permanent DTC values.
3. Elapsed request-to-reassembled-response trace times are 2–27 ms (median
   10 ms) in the short run and 0–28 ms (median 10 ms) in the long run.
   These are millisecond-resolution logger measurements for responses actually
   traced, including negative/invalid replies. They are not guaranteed ECU
   execution times or proof that the 70 ms response-window policy is unnecessary.

No firmware was changed or flashed and no connected recording was stopped during
this review. Next work is the shared receive/timing investigation followed by
simultaneous Kvaser validation in both CAN and OBD modes.

## Input SHA-256

- 01_OBD_1757338.TXT: `321d7295359f3f9b0eb59a566219c06b17633e3f581081a0b48cfa08c54aca81`
- 02_OBD_1757338.TXT: `f7c761e7f25cd8d9c749b8c3afbf2c5c1ba71c84f1a71e9cacc93fdc7a4d788e`
- OBD_1.asc: `9B917CE0A61F36146EE41F3BD1D8C61695811B6679A74EA2E168761A8A026413`
