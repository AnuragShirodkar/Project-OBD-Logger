# SD folder layout - USB-only bench check

Date: 2026-11-24. Device: OBD_1757338. Firmware: 1.1.0-dev6-folders.
User confirmed USB-only power and CAN disconnected.

- Storage-path regression passed: new config precedence, root fallback, directory
  creation/reuse, existing-file collision and directory creation failure.
- Existing rotation and MTP visibility tests passed. Teensy 4.1 build passed using
  core 1.62.0 and USB Serial + MTP. An existing FlexCAN strncpy warning remains;
  the new log-path truncation warning was resolved by enlarging the path buffer.
- Old firmware gave no identity response over repeated serial queries. A stop
  command was sent, but closure was not confirmed. The user then pressed Program
  for recovery entry. The previous run's log completeness is not established.
- Hidden command-line loader programmed and booted the new image successfully.
  device_info returned the same ID and exact expected firmware version, with
  sd_mounted=true and oled_ready=true (not a visual OLED quality test).
- MTP showed CONFIG and LOGS directories, and a new LOGS/01_OBD_1757338.TXT.
  Downloaded header confirmed the new firmware and complete /LOGS path.
- Existing root config was backed up locally in the ignored build directory and
  copied into CONFIG. A fresh MTP readback matched SHA-256
  40B6E572DF28EB31CC8AA09DEC530D447004684C8C2331FA6C14480C847C6981.
  Original root config and existing root logs were not moved or deleted.
- That initial run loaded the legacy root config before the copy: 48 jobs, 500 kbps,
  11-bit functional addressing. It stopped with "TX pending at transaction end;
  stopped without resetting RX" while CAN was disconnected. Status had sd_failures=0,
  log_drops=0, capture_drops=0 and can_esr1=262800. This is not a vehicle logging pass
  and does not establish the reason for the pre-upload serial silence.

Pending physical checks: restart and confirm canonical config selection; ECU-connected
logging and clean stop; actual size-triggered rotation within LOGS; missing/invalid
canonical config and actual SD directory-write failure handling. Canonical precedence
and related failure cases were tested in software, not all on this physical card.
