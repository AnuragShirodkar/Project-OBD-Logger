# CAN filter configuration — software-team proposal

Status: template only; **not supported by installed firmware 1.2.0-dev4-idle-sd**.
The current parser rejects the `filters` key. Do not deploy this example to an
existing logger expecting it to filter traffic. Firmware support and validation
are still required. The existing `config.can.example.json` remains usable.

Share `examples/config.can.filters.proposed.json` with the software team as the
proposed extension to the version-2 CAN configuration. Its final supported schema
version must be agreed when firmware implements this extension; `version: 2`
alone is not proof of filter capability.

## Proposed meaning

- `operating_mode: "can"`: passive Classical CAN recording, not OBD requests.
- `bitrate: 500000`: 500 kbit/s.
- `startup_delay_ms: 0`: no configured startup wait.
- `run_duration_ms: 0`: no fixed duration limit; normal stop/error/idle rules apply.
- `filters.classic_can`: list of CAN ID ranges to include in the recording.
- `filter_id`: unique, nonempty label within the list, such as `F1` or `F2`.
- `type: "id_range"`: include IDs between the two endpoints, inclusive.
- `id_start` and `id_end`: hexadecimal strings with `0x` prefix. The example
  includes IDs `0x100` through `0x1FF`, equivalent to decimal 256 through 511.
- `extended_id: false`: match standard 11-bit frames only. `true` matches
  extended 29-bit frames only, even when the numeric ID is below `0x800`.

Proposed list behaviour: include a frame if it matches **any** configured range.
A frame matching overlapping ranges is logged once. Missing `filters`, or an
empty `classic_can` list, means record all supported Classical CAN IDs, preserving
the current unfiltered behaviour. An invalid filter must reject the configuration
rather than silently record everything. Filters select frames for recording;
they do not enable transmission, acknowledgements or CAN FD.

## Software validation and handoff

Check `id_start <= id_end`. For standard frames both endpoints must be between
`0x0` and `0x7FF`; for extended frames between `0x0` and `0x1FFFFFFF`.
Require actual JSON booleans for `extended_id`, unique labels and the exact
supported filter type. Reuse the existing bitrate and timing validation rules.
Additional ranges are additional objects in the same array, with unique labels.

Once a compatible firmware build is available, write the generated file as
`/CONFIG/config.json` through the existing stopped/file-access workflow. Keep the
template's descriptive filename for sharing; that is not the device's active
configuration filename. Do not offer filter deployment to older firmware.

Firmware implementation must define a maximum filter count, report filter
capability to the app, update parsing (including nesting capacity), validate
boundary cases and standard/extended separation, and test the saved frames
against Kvaser. Preserve bus-activity detection independently of recording
filters so excluded traffic alone does not falsely imply a disconnected bus.
These are proposed requirements, not completed firmware changes.
