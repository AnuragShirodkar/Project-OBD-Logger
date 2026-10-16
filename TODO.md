# Project to-do list

## Continuous logging and USB SD-card access

Requested on 2026-10-16. Implemented in the automatic sketch; hardware acceptance remains pending.

- [x] Continuous logging with run_duration_ms=0, with optional timed runs retained.
- [x] Safe stop using a three-second hold on a pin 35 to GND button, or USB stop/close command; drain buffers and close the log.
- [x] SD access over USB MTP after safe stop, plus button-held file-access-only boot.
- [x] Set USB product and MTP friendly/model names to OBD logger without changing installed core files.
- [x] Protect logging from host edits: advertise no SD filesystem during capture; expose it after the log handle closes.
- [x] Apply JSON changes only after restart; no live reload.
- [x] Rotate logs near 128 MiB, preserve old logs, stop on SD write failure/name exhaustion.
- [ ] Hardware verification: Windows This PC name and enumeration, log copy/hash comparison, configuration upload, USB reconnect, safe stop, file-access-only boot, long-run capture, rotation and SD-full behavior.

See teensy-obd-sd-logger/README.md for wiring, Arduino USB selection and the first acceptance test. The original manual sketch remains unchanged.

- [x] Manual Mode 04 on pin 40: two-second hold, once per press, serialized with polling; stop takes priority.
- [ ] Hardware-check pin 35 stop/file access and pin 40 manual request, including held-button and simultaneous-stop behavior.
