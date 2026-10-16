# Project to-do list

## Continuous logging and USB SD-card access

Requested on 2026-10-16. Implemented in the automatic sketch; hardware acceptance remains pending.

- [x] Continuous logging with run_duration_ms=0, with optional timed runs retained.
- [x] Safe stop using a one-second hold on a pin 2 to GND button, or USB stop/close command; drain buffers and close the log.
- [x] SD access over USB MTP after safe stop, plus button-held file-access-only boot.
- [x] Set USB product and MTP friendly/model names to OBD logger without changing installed core files.
- [x] Protect logging from host edits: advertise no SD filesystem during capture; expose it after the log handle closes.
- [x] Apply JSON changes only after restart; no live reload.
- [x] Rotate logs near 128 MiB, preserve old logs, stop on SD write failure/name exhaustion.
- [ ] Hardware verification: Windows This PC name and enumeration, log copy/hash comparison, configuration upload, USB reconnect, safe stop, file-access-only boot, long-run capture, rotation and SD-full behavior.

See teensy-obd-sd-logger/README.md for wiring, Arduino USB selection and the first acceptance test. The original manual sketch remains unchanged.
