# SkiDrive OBD/CAN Logger — product name

Firmware build: `1.2.0-dev6-skidrive`. This name update includes the dev5 receive
and independent-timer changes. It does not introduce configurable CAN filters.

- Brand: **SkiDrive**, retaining the name selected for the product.
- Product label: **SkiDrive OBD/CAN Logger**.
- Windows USB/MTP friendly name: **SkiDrive_1757338** for this logger, with the
  hardware-derived number changing automatically for other units.
- USB/MTP manufacturer: **Skillicon Technologies**.
- Permanent device/API ID: **OBD_1757338**, unchanged.
- Existing OBD/CAN file naming: unchanged. From dev7 the numeric ID appears below
  SkiDrive on the startup screen; it no longer occupies the recording header.
- OBD and CAN log titles: **SkiDrive OBD/CAN Logger**. The separate Firmware field
  records the actual build; the stale SkillLogger Version 1.0.0 heading is removed.

The USB serial number, VID/PID and fuse-derived numeric identity are unchanged.
Product branding is a display label, not a new device identity. Existing saved
logs are not rewritten and the old OBD-only project is not renamed.

## Windows software integration

The `@UPDATE` reply retains `product: "SkiDrive"`, stable `id`, and protocol 1.
It adds `display_name: "SkiDrive OBD/CAN Logger"`. Display the product label
alongside the ID, and use the permanent ID to bind configurations, downloads and
updates. Fall back to `product` when older firmware lacks `display_name`.

The legacy machine-readable `device_type: "OBD_LOGGER"` is preserved to avoid
breaking existing clients. It is not a description of which mode is currently
active; use `operating_mode` for that. Do not require an `OBD_` prefix in the
Windows friendly name when detecting the renamed device. Windows can cache
earlier device labels until it refreshes enumeration.

## Validation boundary

Existing header-format tests now expect the SkiDrive title, and the full host
suite and Teensy build are run for the change. USB enumeration, the Windows
friendly name, the new display-name reply and new on-card headers still require
post-installation verification. Compilation is not proof of those device results.

27 November results: all 14 host test executables and the layout reporter passed;
the final Serial + MTP Teensy 4.1 build succeeded. Compiled-image string inspection
confirmed the dev6 version, log title, USB name format and `display_name` field.
Dev6 was subsequently installed and its Windows name, API reply and on-card OBD
header verified; see [installation results](test-reports/2026-11-27-dev6-skidrive-installation.md).
Live receive-load validation remains pending.

Prepared HEX SHA-256:
`9B7C6D58E8897BF462E8E5B5A6F6FF516D5DC17E42EE3F8C1237AE1248364DF3`
