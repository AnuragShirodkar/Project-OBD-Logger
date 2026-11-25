# Project separation verification

Date: 2026-11-25. No logger was flashed or SD configuration changed.

The workspace now contains two independent project folders:

- teensy-obd-sd-logger: previous OBD-only project from 7021f2f, firmware
  1.1.0-dev6-folders, including original manual tester and OLED demo.
- teensy-obd-can-logger: current dual-mode project from 7d33b4a, firmware
  1.2.0-dev2-can-names, including separate config examples, tests and OLED demo.

Both have local docs and firmware-package tools. Source and configuration behaviour
were not combined or changed. Only README/document paths and project work lists
were adapted. Existing historical design and test documents are retained as context;
they do not establish new hardware acceptance for dual-mode firmware.

Verification completed:

- OBD: 36 source/config/test files match 7021f2f, excluding the adapted README.
- OBD/CAN: 41 source/config/test files match 7d33b4a, excluding the adapted README.
  Comparisons normalize Git working-copy line endings only.
- Before restoration, hashes verified preservation of 148 current tracked/user files.
- Current untracked functional/non-functional requirement maps and the raw test log
  remain identical in both docs folders. They are preserved as untracked working
  files rather than silently publishing unfinished material.
- Both project test.ps1 suites passed and each compiled independently with Teensy
  core 1.62.0 and USB Serial + MTP. The pre-existing FlexCAN strncpy warning remains.
- All 12 firmware-package tests passed in each project, including relocated schema
  lookup, signature verification and unsigned-template blocking.
- Independent python-can ASC import passed from the new project's generated fixture.
- Root/project README links were checked and resolve to the new locations.

The named stm32-obd-simulator and stm32-obd-four-ecu-stage1 directories were removed
from disk. They were already excluded from the current Git tree; no remote history
was rewritten. Root docs/tools/manual/OLED duplicate folders were removed only after
copy verification. Shared Schematics and third-party datasets under New folder were
preserved. Existing build/export artifacts were not intentionally purged.

Physical bus validation, display appearance, loss/load testing, battery-backed save
and the customer updater remain pending as recorded in each project. This is a
repository organization check, not a new hardware test or certification result.
