# OLED review previews

Source firmware commit 18e052c. These software previews reproduce its 128x32 layout using the installed Adafruit GFX classic font. Blue approximates the physical OLED. OBD_51234 is an example ID. Hardware appearance and I2C page-transfer effects remain unverified.

The sheets contain 23 numbered screen views plus a blank display-failure illustration (24). Screens 22 and 23 are defensive rendering fallbacks rather than expected normal states. Native PNGs are 128x32; individual enlarged PNGs use nearest-neighbour scaling.

Pulse positions are illustrative. The pulse advances with recent diagnostic replies (within five seconds), outside shutdown/fault states. Slow polling can pause it. SAVE/USB/ERR badges take precedence during shutdown or faults. LINK LOST overrides Mode 04 feedback. Firmware can mark replies stale before connection checks finish.

Fault variants share LOG ERROR. Dedicated SD FULL, BUS OFF and INVALID CONFIG screens do not exist in this version. OLED failure may leave a blank, frozen, partially updated or corrupted display; no on-screen message is guaranteed.

Animations use the nominal maximum ten frames per second. Actual updates depend on loop load. The connection-loss animation illustrates state order, not actual probe/shutdown durations. It begins at the final period of live traffic; loss detection itself waits for the five-second reply-silence threshold.
