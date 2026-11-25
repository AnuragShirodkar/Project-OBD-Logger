# CAN ASC layout revision

Date: 2026-11-25. Firmware: 1.2.0-dev3-asc-layout.

The supplied Desktop CAN_1.asc was inspected as a formatting reference. Its content
was not modified or copied into the repository. Rows now use its aligned elapsed
time, channel, identifier, direction, frame-type, DLC and payload columns.

Product, firmware, stable logger ID, file path, bitrate and configuration fingerprint
remain above the trace as comment metadata. Only physical channel 1 is advertised.
No calendar date or synthetic trigger event is invented. Each new CAN segment has
Begin Triggerblock / End Triggerblock markers. Direction is Rx from our logger's
perspective; remote frames retain DLC but do not contain zero-filled fake payloads.

The existing 01_CAN_<ID>.ASC naming fix is retained. Older dev1 firmware generated
CAN ASC files using 01_OBD_<ID>.ASC; that prefix did not mean those recordings were
OBD request/response logs. The last successful device query in the earlier check
reported dev1. A read-only query during this revision gave no identity response;
no current installation claim is made. No device was flashed and existing logs were
not renamed, reformatted or deleted.

Validation completed:

- Entire OBD/CAN host suite passed, including OBD regressions, aligned-row golden
  strings, standard/extended/remote/zero-length frames and bounded buffers.
- Header tests check metadata, field order, channel, trigger marker and truncation.
- Rotation tests reserve the footer within the 250,000,000-byte limit, including
  exact-boundary and one-byte-over cases. Firmware writes a single footer on segment
  rotation or final close and treats short footer writes as storage failures.
- Independent python-can 4.6.1 import passed with the actual header/row/footer
  format and a timestamp beyond micros rollover. The committed synthetic example
  also imports correctly. It is explicitly not a vehicle recording.
- Teensy 4.1/core 1.62.0 Serial + MTP build passed. The existing FlexCAN strncpy
  warning remains. Flash: code 161832, data 31788, headers 9128 bytes.

Pending: installation on the intended logger, physical capture/stop/rotation, and
import of those recordings into the user's CANKING/other intended analysis software.
Unexpected power loss or an SD error may leave a segment without the end marker.
The separate OBD-only project was not changed.
