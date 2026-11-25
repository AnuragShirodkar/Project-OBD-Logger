# OBD/CAN Logger - current work list

- [x] Separate project with code, tests, JSON examples, docs and package tooling.
- [x] Config-selected OBD or passive Classical CAN on pins 22/23.
- [x] Separate OBD TXT / CAN ASC names and selected-mode OLED indications.
- [x] Version-1 OBD compatibility and explicit version-2 mode selector.
- [x] Invalid/missing config stops without enabling CAN; no automatic OBD fallback.
- [x] Fixed >3-second CAN idle stop, save/close and OLED SD ACCESS; restart to resume.
- [ ] Flash and validate both modes on actual hardware when requested.
- [ ] Reference-capture load/loss, no-ACK listen-only and intended-tool ASC import tests.
- [ ] Physical storage rotation, stop/drain and SD/OLED fault tests.
- [ ] Windows integration for mode selection, per-device transfer and status.
- [ ] Assign the hardware revision and implement approved-release signing/packaging.
- [ ] Complete the customer updater and recovery acceptance tests.
- [ ] Battery-backed main-power-loss detection and safe shutdown hardware/firmware.

See docs/dual-mode-software-handoff.md and docs/test-reports/Dual_Mode_01.md.
CAN FD, configurable ID filters, XCP and CCP are not implemented by this project.
