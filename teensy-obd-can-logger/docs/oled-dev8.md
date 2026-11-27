# OLED correction — 1.2.0-dev8-oled-spacing

## Current revision: 1.2.0-dev9-oled-gaps

After reviewing the user's physical-display photo, move activity towards OBD/CAN
to equalize both gaps instead of centring it within the entire screen. Mode stays
at x=0, width=25; SD stays at x=85, width=43. ERROR is now at x=38, width=34,
giving **13 blank columns on each side**. LIVE/WAIT are at x=41, width=28,
giving **16 blank columns on each side**. Existing font, main message, startup
screen and moving connection track are unchanged. The older geometry and dev8
installation results below are historical. New compilation/installation checks
will be recorded after verification.

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
five-pixel activity/storage gap.

Installed on **OBD_1757338** using the hidden CLI loader, exit code 0. Before
software recovery entry, verified dev7 was stopped with its known USB-only
pending-TX fault, zero SD/log/capture failure counters and a readable drained
log. Normal update readiness was failed; this was recovery installation after
closure verification. Existing config and logs were not changed.

Post-restart USB verified firmware **1.2.0-dev8-oled-spacing**, unchanged device
ID and SkiDrive display name, SD mounted and **oled_ready=true**. The startup
query reported `not_prepared`, no fault and no drops at that instant; this is not
a sustained traffic test. No manual button or graphical loader was needed.

Image SHA-256:
`72924C500AB3809102681A000DDE407C81180FAECA0519F068266DE5A60DF658`

Physical appearance and animation still require user observation. Live capture
and the receive-overrun fix are not validated by this UI-only installation check.
