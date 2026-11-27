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
was changed. Installation verification pending; physical rendering is not claimed.
