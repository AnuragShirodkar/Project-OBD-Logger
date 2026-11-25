# Named MTP responder

OBD_MTP.cpp is copied from Teensy core 1.62.0 `cores/teensy4/MTP_Teensy.cpp` (PJRC/PaulStoffregen cores project, https://github.com/PaulStoffregen/cores). The original MIT copyright/license notice is retained. Original source SHA-256: 7064c36328445ba734cd7aea69d8a359f9f8add35303b824dded4e6354690e01.

Changes: normalize trailing whitespace; include Arduino.h to use the core MTP declarations, then override MTP_MANUF, MTP_MODEL and MTP_NAME to `OBD logger`. The project responder definitions satisfy the linker before the core archive responder is pulled in; the stock core storage engine and USB transport are reused. Do not install an additional MTP library for this sketch. Tested against Teensy 1.62.0; recheck this replacement when updating core versions.

The USB product descriptor is independently overridden in usb_identity.c. Global board-package files and the old manual sketch are untouched.

Local enumeration change: GetNumObjects and both GetObjectHandles passes use VisibleObjects.h to omit System Volume Information, mtpindex.dat, and their descendants. Core storage/index handling is retained.
