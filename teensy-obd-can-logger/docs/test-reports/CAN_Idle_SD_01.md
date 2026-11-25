# CAN idle stop and SD access - software validation

Date: 2026-11-25. Firmware: 1.2.0-dev4-idle-sd. No device was flashed.

Requested policy: after more than three seconds without a valid received CAN frame,
stop recording, save/close the file, expose SD access and indicate it on the OLED.
This intentionally supersedes the earlier keep-recording-through-silence policy.

Implementation uses a fixed 3000 ms threshold in CAN mode only. Before the first
frame, the timer starts when capture starts, excluding configured startup delay.
Later valid data/remote frames reset it. User stop has priority, followed by a
configured duration boundary, then idle expiry. Exactly 3000 ms is not expiry.

The ISR activity snapshot, idle decision and disabling of further capture are atomic.
The existing stop path drains software frames, appends final counters/end marker,
flushes/closes the file and only then exposes MTP storage. The OLED shows SAVING
during closure and SD ACCESS after completed CAN shutdown; fault indications take
priority. Capture remains disabled on returning traffic; restart is required after
file transfers. No hardware button or JSON schema change was added.

Checks passed:

- Initial no-frame timing, exact 3000/3001 ms boundary, latest-frame timer reset,
  recent ongoing traffic, startup-delay exclusion, user/duration priority and wrap.
- Full host regression suite, including OBD/ISO-TP/Mode 04 and ASC/rotation tests.
- Independent python-can ASC import for the regenerated example.
- Teensy 4.1/core 1.62.0 Serial + MTP build. Only the existing FlexCAN strncpy
  warning remains. Flash: code 161960, data 31788, headers 9000 bytes.

Physical checks remain pending: no frames at startup; live traffic then silence;
fresh traffic just before expiry; buffered shutdown; actual Windows SD enumeration
and OLED appearance; no automatic resumption during MTP; SD failures and OBD regression.
SD access occurs after draining/writing/enumeration, not necessarily at exactly three
seconds. Quiet connected buses, wrong bitrate and missing ACK can also cause this stop.
This is not physical unplug detection or battery-backed power-loss protection.
