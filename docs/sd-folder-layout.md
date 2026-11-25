# SD configuration and recording folders

Firmware `1.1.0-dev6-folders` creates `/CONFIG` and `/LOGS` on a writable SD card.
The Windows app should save settings to `/CONFIG/config.json` and enumerate new
recordings under `/LOGS`. The JSON schema itself is unchanged. Restart the logger
after finishing configuration transfers so it loads the changed settings.

Configuration precedence:

1. If `/CONFIG/config.json` exists, use it.
2. Otherwise try the legacy `/config.json` at the card root.
3. From dual-mode firmware `1.2.0-dev1-dual-mode`, a missing or invalid selected file
   stops without enabling CAN and exposes SD access with CONFIG ERROR on the OLED.
   The earlier automatic Mode 01 fallback is removed to prevent unexpected traffic.

An invalid canonical config does not silently select a different old root config.
Existing settings and recordings are not moved or deleted automatically. To migrate,
copy a known-good root config into CONFIG while recording is safely stopped, verify
the copy, then restart. Retain the root copy as a backup if desired; it will be ignored
while the canonical file exists. Support/root files such as mtpindex.dat are separate
from user logs and are not relocated.

New files keep the existing ID and index format: `/LOGS/01_OBD_1757338.TXT` for OBD,
or `/LOGS/01_OBD_1757338.ASC` for passive CAN. The OBD_ prefix remains the stable
device identity; it does not select the operating mode.
Number allocation and 250 MB rotation check the LOGS directory, avoid existing names
and create the next segment there. Old root-level logs remain available. The first
new folder log can have the same basename as an old root file because paths differ;
Windows downloaders must preserve paths or resolve destination name collisions.

If a regular file occupies CONFIG or LOGS, or the directory cannot be created/opened,
recording cannot start; the firmware reports SD_DIRECTORY_ERROR on USB and retains
the existing storage-failure shutdown path. It never overwrites the conflicting file.
USB SD access is still exposed only after recording stops through the existing path.

The software team's migration reader should list both LOGS and legacy root recordings
without deleting either, but all new configuration writes should use CONFIG.

Validation: storage-path regression covers canonical precedence, legacy fallback,
directory creation/reuse, file collisions and creation failure. Full Teensy build and
physical-board checks are recorded separately; this layout does not change CAN polling.
