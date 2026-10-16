# Project to-do list

## Continuous logging and USB SD-card access

Requested on 2026-10-16. Planned; not implemented yet.

- [ ] Remove the mandatory 60-second run limit. Support continuous logging until the user explicitly stops it; retain an optional timed run for testing.
- [ ] Define and implement a user-operated stop control that stops new requests, handles the active transaction, drains capture/log buffers, flushes the SD card, and confirms completion before power is disconnected.
- [ ] Allow users to browse and copy files from the Teensy's installed SD card over USB on Windows, without removing the card.
- [ ] Show the device in Windows **This PC** as **OBD logger**, rather than Teensy. Verify the displayed name on Windows with the selected USB file-access implementation.
- [ ] Evaluate MTP for device-managed file access. Do not expose the same filesystem to simultaneous independent host/device writers. Define access to the active log and restrict host deletion/editing of files being used by the logger.
- [ ] Specify when host configuration edits take effect; do not change a running diagnostic schedule because a partially written JSON file becomes visible.
- [ ] Verify USB connection/disconnection, file transfers, safe stop, long runs, log rotation/file-size limits, SD-full behavior, and capture performance while transferring files.

The current firmware remains unchanged: it still uses the configured finite run duration and does not yet provide USB SD-card browsing. Preserve the original manual sketch when implementing these features in the automatic logger.
