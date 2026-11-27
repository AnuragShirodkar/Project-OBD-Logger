# OLED correction — 1.2.0-dev8-oled-spacing

Supersedes the dev7 top-row order. Startup branding, main status text and the
moving connection track remain as in [dev7](oled-dev7.md).

- Top left: **OBD** or **CAN**, or **---** for an unknown mode.
- Top centre: **LIVE**, **WAIT** or **ERROR** only. The main status line retains
  details such as SAVING, LINK LOST and SD ACCESS.
- Top right: **SD:45%**, the estimated percentage used; SD:NO or SD:-- when
  unavailable. SD:100% also fits without reducing font size.
- Middle: existing RECORDING, CONNECTING, SD ACCESS and other state messages.
- Bottom: existing moving connection pulse, frozen when recording/link activity
  stops. The non-blocking SkiDrive/numeric-ID startup screen remains unchanged.

128x32 geometry at the existing font: mode x=0, width=25; activity is centred
within the screen with width 27 for LIVE/WAIT and 33 for ERROR; SD x=85, width=43.
ERROR occupies x=47..79, leaving five blank columns before SD begins at x=85.
LIVE/WAIT occupy x=50..76, leaving eight blank columns. The mode badge ends at
x=24, also leaving a visible gap to the centred badge. Visible SD:100% text is
41 pixels wide, fitting with one pixel padding on each side. Rounded badge
backgrounds, large main text and 16-byte I2C updates are preserved.

No recording, CAN, JSON, storage, file-naming or permanent-ID behaviour changes.
27 November: Teensy 4.1 Serial + MTP compilation succeeded. Checked all three
activity labels and the longest storage text against the font bounds and minimum
five-pixel activity/storage gap. Installation verification pending. Physical
appearance and animation still require user observation.
