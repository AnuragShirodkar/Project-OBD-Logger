# SkiDrive OLED layout — 1.2.0-dev7-oled

For the existing SSD1306 128x32 display:

- Startup shows **SkiDrive**, then **ID:1757338** (hardware-derived numeric ID).
  It lasts approximately 1.5 seconds from the first display update after setup.
  There is no delay loop: capture, SD logging and USB service continue.
  A detected fault overrides the branding on the next display refresh.
- Top left: a rounded badge with **SD:45%**, the estimated percentage **used**.
  **SD:100%** fits at the same font size. **SD:NO** means no mounted card;
  **SD:--** means capacity is unavailable. No additional FAT scan is introduced.
- Top centre: rounded activity badge: **LIVE**, **WAIT**, **LOST**, **SAVE**,
  **USB**, **OFF** or **ERROR**, depending on the existing recording state.
- Top right: rounded **OBD** or **CAN** badge, or **---** if mode is invalid.
  Storage, activity and mode no longer alternate.
- Main line: existing large status text, including RECORDING, CONNECTING,
  NO TRAFFIC, SAVING, SD ACCESS and LOG ERROR. Messages longer than ten characters
  retain the smaller fitting font. OBD file-access status now also says SD ACCESS.
- Bottom: existing dotted connection track and traveling pulse. The pulse advances
  only with recent accepted mode activity while recording is active and healthy;
  its position freezes when activity stops or recording ends. The startup screen
  does not draw the track.

Geometry: SD badge x=0, width=45; activity x=46, width=36; mode x=103,
width=25. Top text uses the existing 6x8 cell, with visible glyph width 5.
The largest SD label needs 41 visible pixels and ERROR needs 29; both fit with
padding. The top row is 9 pixels tall, the main line starts at y=12, and the
connection track occupies y=29..31. Existing 16-byte chunked I2C transfers and
display-failure isolation remain in place.

This UI change does not alter JSON parsing, polling intervals, device identity,
log filenames, the dev5 receive path or its pending live-load validation.
The old OBD-only project is unchanged. Firmware build/installation checks are
recorded below; physical appearance and animation still need user observation.

## Status

27 November: Teensy 4.1 Serial + MTP build succeeded. Header widths and longest
status strings were reviewed against the 128x32 bounds. No recording-path logic
was changed.

Installed on OBD_1757338 via the hidden CLI loader, exit code 0. Before software
recovery entry, verified the previous firmware was stopped with the known USB-only
pending-TX fault, no storage/capture drops, and a readable drained log. Normal
update readiness remained failed; recovery was not presented as a passed normal
preparation. No physical Program-button press or graphical loader was needed.
The first post-upload serial open ran before Windows restored COM8 and failed;
retrying after enumeration succeeded without reflashing.

Verified USB `device_info`: firmware **1.2.0-dev7-oled**, unchanged ID
**OBD_1757338**, product/display name retained, SD mounted, **oled_ready=true**.
Read back `/LOGS/07_OBD_1757338.TXT` through SkiDrive_1757338 MTP. Its header
identifies dev7 and the unchanged config fingerprint **BBCB677B**, 64 jobs,
500 kbit/s OBD configuration. It reports no SD write failures, a 2,048-frame
staging capacity, GPT2 timing and CAPTURE_DRAINED.

HEX SHA-256:
`C57F9559662828F0BD1A93193737003C7FCA5F263FF285DDEE40F283FFF5DA02`

With CAN disconnected the retained OBD configuration stops on a pending transmit,
so LOG ERROR remains expected in this USB-only test. No live CAN/OBD traffic was
tested. OLED communication was checked, **not the physical pixels, animation or
startup-screen appearance**. User observation after restart is still required;
live receive-overrun validation remains pending.
