# OLED review previews

Source firmware commit 18e052c. These software previews reproduce its 128x32 layout using the installed Adafruit GFX classic font. Blue approximates the physical OLED. OBD_51234 is an example ID. Hardware appearance and I2C page-transfer effects remain unverified.

The sheets contain 23 numbered screen views plus a blank display-failure illustration (24). Screens 22 and 23 are defensive rendering fallbacks rather than expected normal states. Native PNGs are 128x32; individual enlarged PNGs use nearest-neighbour scaling.

Pulse positions are illustrative. The pulse advances with recent diagnostic replies (within five seconds), outside shutdown/fault states. Slow polling can pause it. SAVE/USB/ERR badges take precedence during shutdown or faults. LINK LOST overrides Mode 04 feedback. Firmware can mark replies stale before connection checks finish.

Fault variants share LOG ERROR. Dedicated SD FULL, BUS OFF and INVALID CONFIG screens do not exist in this version. OLED failure may leave a blank, frozen, partially updated or corrupted display; no on-screen message is guaranteed.

Animations use the nominal maximum ten frames per second. Actual updates depend on loop load. The connection-loss animation illustrates state order, not actual probe/shutdown durations. It begins at the final period of live traffic; loss detection itself waits for the five-second reply-silence threshold.

## Storage display update

05-storage-update.png supersedes the earlier layout: main status is now size 1 at y=11; storage is at y=21; ID, badge and bottom pulse remain. Earlier sheets illustrate the previous layout. SD ~42% USED is estimated card-wide usage, including files already on the card. SD LOW appears at 90% used. Missing cards show SD MISSING; unavailable capacity shows SD SIZE UNKNOWN. Capacity is measured before capture; successful log writes update the estimate without scanning FAT while recording. Allocation overhead can make the estimate slightly low. USB mode refreshes actual allocation every five seconds. These images use example values; hardware testing remains required.

## Compact storage header

06-compact-storage.png is the latest layout, superseding 05 and earlier sheets. The top-left now displays SD ~42% (estimated percentage used), or SD ~95% LOW from 90%. SD MISSING and SD --% indicate missing media and unavailable capacity. This replaces the OLED logger ID; USB identity and log metadata are unchanged. Main status uses compact size-1 text centered at y=17, with CONNECTING replacing FIND ECU. Connection badge and bottom pulse remain.

## Large status with centered badge

07-centered-badge.png is the latest layout. Large size-2 main text at y=12 restores the original screen style. The 32-pixel LIVE/ERR/WAIT/USB badge is centered at x=48; SD usage is size 1 at the top-left (SD42%, maximum SD100%!). An exclamation mark means 90% or more used. Values remain estimated during capture. Missing media shows SD NONE; unknown capacity shows SD --. CONNECTING remains the startup label. The bottom animation is unchanged.

## Three-item header

08-three-item-header.png is the latest layout: full logger ID on the left, status badge centered, and SD usage right-aligned. A compact 3x5 font (four-pixel advance) fits the maximum 12-character OBD identity in 47 pixels, with a centered 24-pixel badge and up to seven SD characters in 27 pixels. Large main text and the bottom animation remain unchanged. Physical readability needs OLED verification.
