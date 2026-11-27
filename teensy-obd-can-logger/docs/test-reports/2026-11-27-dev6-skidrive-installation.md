# SkiDrive dev6 installation — 27 November 2026

User explicitly requested flashing the naming build. The previously confirmed
USB-only setup remained the session context. Queried the selected COM8 device:
ID OBD_1757338, dev5, stopped with a pending-TX fault, and no SD/log/capture drops.
Normal preparation returned failed, not ready. Before recovery, read back the
closed dev5 log 04_OBD_1757338.TXT through MTP and verified CAPTURE_DRAINED and
zero pending software frames. This was recovery installation, not a claim that
normal update preparation passed.

Entered HalfKay over USB using the documented 134-baud trigger, then detected
VID 16C0/PID 0478. Installed the hash-verified image using the hidden CLI loader.
It reported programming and booting and returned exit code 0. No physical button
press or graphical loader was needed for this installation.

Image SHA-256:
`9B7C6D58E8897BF462E8E5B5A6F6FF516D5DC17E42EE3F8C1237AE1248364DF3`

Verified after restart:

- Firmware **1.2.0-dev6-skidrive**, product **SkiDrive**.
- New API display_name **SkiDrive OBD/CAN Logger**.
- Permanent ID **OBD_1757338**, unchanged; COM8, VID/PID unchanged.
- Both Windows PnP and This PC/MTP show **SkiDrive_1757338**.
- SD mounted and OLED communication reported ready (visual OLED not inspected).
- Read back **/LOGS/05_OBD_1757338.TXT**. Its first line is
  **SkiDrive OBD/CAN Logger**, followed by the actual dev6 Firmware field.
- Configuration fingerprint **BBCB677B**, 64 jobs, 500 kbit/s OBD configuration
  retained. No configuration or existing logs were rewritten.
- New log reports staging capacity 2,048 and GPT2 timing source, maximum ISR 1 us,
  clock disagreements 0, and CAPTURE_DRAINED.

The retained OBD configuration again stops on an unacknowledged pending transmit
with CAN disconnected; the API consequently reports state=failed. This was not a
receive-overrun or SD-write failure. No ECU traffic was tested, so this check does
not establish lossless capture, timer accuracy under load, CAN-mode header
readback, or resolution of the previously reported overruns. Restart with the
ECU/simulator and Kvaser test setup for those checks.
