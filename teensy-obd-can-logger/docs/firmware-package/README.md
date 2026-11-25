# SkiDrive normal-Teensy firmware package handoff

Status: integration template, package format version 1. No logger firmware changed or flashed.
This is a Windows-team implementation contract, not a completed customer updater.

## What to give the software team

Import `template/SkiDrive_0.0.0-integration_TEMPLATE_DO_NOT_FLASH.skfw` into the app.
It is a ZIP container with a product extension. Use a ZIP library; do not execute it.
The expanded files beside it are for developers to inspect.

The sample HEX contains four zero bytes, not an application. The app must show
"Integration template - cannot install" and disable installation. Never send it to
Teensy Loader, even when its HEX checksums are valid. Do not rename this template
as a release or substitute a build without completing release checks and signing.

For a customer release, email a download link to one file such as
`SkiDrive_1.2.0.skfw`. Customers already have the Windows software installed.
They select **Firmware update > Choose update file**, review the version, then
click **Install update**. No Arduino IDE or extra customer-facing loader window
is needed. The team must implement and bench-test this workflow.

## Container contents

Only these root entries are allowed; no directories, nested ZIPs or extra files:

- `manifest.json`: UTF-8 JSON, at most 16 KiB; reject duplicate JSON properties.
- `firmware.hex`: Intel HEX for the standard Teensy 4.1, not source code.
- `release-notes.txt`: UTF-8 plain text, at most 64 KiB. Never render as executable HTML.
- `signature.ed25519`: release packages only, exactly 64 raw signature bytes.

Limit both the package file and total expanded content to 16 MiB. Reject duplicate
entry names, ZIP links, encrypted ZIPs, unexpected paths and malformed archives.
Read entries into bounded memory without extracting untrusted paths. If the uploader
needs a disk file, write the already-verified image into an app-owned temporary
directory with a fixed filename. Recheck its hash immediately before programming,
close other access to it, and remove temporary files after the attempt.

The ZIP file's name is cosmetic. Decide acceptance using authenticated contents.
The 16 MiB transport limit is not the available program-flash size.

## Manifest fields

The supplied `manifest.schema.json` documents structure. Schema validation alone
does not verify authenticity, a HEX image, compatibility or device readiness.

- `format`: exactly `skidrive-firmware-package`.
- `format_version`: integer `1`; reject unknown versions.
- `package_kind`: `integration-template` or `release`. Templates can be previewed
  but must never be installed. There is no customer override for this block.
- `product`: `SkiDrive`; `device_type`: `OBD_LOGGER`.
- `firmware_version`: expected exact value of the device's `firmware` field after
  reboot. Example `1.2.0`; the example version is not an existing release.
- `target.board`: `teensy41-standard`; `target.mcu`: `IMXRT1062`.
- `target.hardware_revisions`: an explicit list of approved product revisions.
  Do not use wildcards. `unassigned` is allowed in the blocked template only.
- `target.usb_update_protocol`: integer `1`, matching device `protocol`.
- `payload.file`: `firmware.hex`; `payload.format`: `intel-hex`.
- `payload.size_bytes`: number of bytes in the exact HEX text file, not ELF size,
  not decoded flash bytes and not compressed ZIP size.
- `payload.sha256`: lowercase SHA-256 hex of that exact file, including line endings.
- `release_notes`: `release-notes.txt`.
- `signing`: null for templates; for releases an object with `algorithm` =
  `Ed25519`, `key_id` = approved public-key identifier, `file` = `signature.ed25519`.

Release signing example (replace the template's null signing field):

```json
{
  "algorithm": "Ed25519",
  "key_id": "skidrive-release-2026-01",
  "file": "signature.ed25519"
}
```

The identifier is an example, not a provisioned production key.

## Authentication and release preparation

For normal Teensy, package signing protects the official app's update path. It
does not prevent someone bypassing the app and programming a different HEX.
It does not encrypt the firmware or conceal the Teensy platform.

1. Assign an actual hardware revision and implement that same value in device_info.
   Current local firmware reports `hardware: "unassigned"`; customer installation
   must remain blocked until this compatibility identity is implemented and tested.
2. Compile the approved source using the approved Teensy core, USB type and libraries.
   Export the `.hex`. Record source commit, tool versions and bench results in the
   internal release record. The package filename is not proof of build provenance.
3. Bench-test that exact HEX, including logger/SD operation, identity and update
   preparation. Its compiled firmware version must match the manifest.
4. Fill the release manifest, sizes and image SHA-256; include normal release notes.
5. Sign the **exact bytes of manifest.json**, including its whitespace/newline, with
   Ed25519. Store the raw detached signature as `signature.ed25519`. Do not reformat
   the manifest after signing. Its authenticated SHA-256 binds the firmware image.
6. Keep the private signing key outside the package, source repository and customer
   application. Embed approved public keys in the Windows app's trust store.
   `key_id` selects among those keys; never trust a key supplied by the package.
7. Validate, test and publish the four files as one `.skfw` ZIP. Email an HTTPS link.

Release notes are informational and unsigned in version 1; do not use them for
version selection, policy, commands or any security decision. Product information
used for those decisions must come from the signed manifest.

Do not silently permit downgrades. Block older versions in the ordinary customer
flow using a real version parser. Equal-version reinstall may be a separate explicit
repair action after the same package and device checks. Normal hardware does not
provide hardware-enforced rollback protection.

## Windows acceptance and installation sequence

1. **Import:** check container limits, manifest structure, supported package version,
   and package kind. Show templates as non-installable. No device command yet.
2. **Authenticate:** require a release signature verified with an app-trusted public
   key. Reject unknown keys, altered manifests, missing signatures and image size/hash
   mismatches. Validate Intel HEX checksums/addresses; never treat valid HEX as proof
   of a bootable or suitable application.
3. **Identify:** discover USB serial candidates, send `device_info\n`, and read complete
   newline-delimited replies beginning `@UPDATE `. Ignore other console lines.
   Use product/type/protocol/hardware for compatibility; USB names alone are insufficient.
   The user must select a logger if several are connected. For this initial updater,
   require exactly one Teensy connected before entering bootloader mode, including
   any device already in bootloader mode. Do not flash a guessed device.
4. **Match:** match product, type, protocol and approved hardware revision; require a
   known normal Teensy 4.1 from provisioning records. Current protocol does not report
   secure/normal board capability. Do not infer it from the ID. Reject unknown/locked
   boards. Save selected `id`, old version and intended new version for verification.
5. **Prepare:** require vehicle/CAN disconnection, reliable USB power and finished host
   SD transfers. Send `prepare_update\n`; poll `update_status\n` until `state: "ready"`.
   A failed or unresponsive preparation aborts the normal update, even if package
   validation passed. Do not bypass shutdown failures or clear fault counters.
6. **Program:** after the above checks, set the selected serial port to 134 baud and
   close it; wait for the tested HalfKay bootloader (VID `16C0`, PID `0478`). Use the
   approved Windows command-line Teensy loader for the normal HEX. Launch it hidden,
   capture stdout/stderr/exit code, use a finite timeout and prevent duplicate jobs.
   Do not fall back to Arduino upload or pop up the GUI. Bootloader entry is separate
   from `prepare_update`; current replies always contain `flash_enabled: false`.
   That field describes the absence of an application flashing API, not permission
   to skip validation or readiness. Do not wait for it to become true.
7. **Verify:** after restart, rediscover and query the application. Require the **same
   logger ID**, exact expected firmware version and compatible product/hardware.
   A successful loader exit by itself is not update success. Show a verification
   failure if the app does not return or identity/version is wrong.

Suggested application timeouts: 3 seconds per identity/status query with up to two
query retries, 30 seconds total preparation, 15 seconds for bootloader appearance,
120 seconds total programming, 30 seconds for application return. These are updater
defaults to validate on the bench, not vehicle OBD response timings. Never retry an
actual flash automatically when its result is uncertain. Cancel is safe before
programming; once writing starts, tell the user to keep USB connected and wait.
Do not claim percentage progress unless the uploader exposes a reliable measure.

Existing protocol examples and limitations are in `../firmware-update-usb.md`.
The recovery button enters programming mode when application firmware is unresponsive;
recovery needs a separate validated workflow because normal identity/preparation may
be unavailable. Interrupted programming does not guarantee the old firmware survives.
This package does not add dual-image storage, automatic rollback or an SD bootloader.

## App messages and test cases

Use clear messages: "Invalid update file", "Update signature not trusted",
"Firmware file is damaged", "Update does not match this logger",
"Integration template - cannot install", "Disconnect other Teensy devices",
"Could not safely stop recording", "Installing - keep USB connected",
"Checking logger after restart", "Update successful", and
"Update could not be verified - use recovery instructions".

Test the template import/block, corrupted HEX with and without an updated hash,
wrong signature/key, wrong product/hardware, unexpected ZIP entries/duplicates,
oversized expansion, older firmware policy, multiple boards, safe shutdown failure,
USB removal, programming timeout and wrong ID/version after restart. Test a valid
signed package end to end on the actual normal board before customer release.

## Reference tooling

From the project root, Python 3.10+:

```powershell
python tools/firmware-package/package_tool.py inspect docs/firmware-package/template/SkiDrive_0.0.0-integration_TEMPLATE_DO_NOT_FLASH.skfw
python -m unittest discover -s tools/firmware-package -p 'test_*.py' -v
```

`package_tool.py` reads packages without extracting or programming them. An imported
template returns `package_approved: false`. For a release, install the `cryptography`
Python dependency and supply an independently trusted raw 32-byte Ed25519 public key:

```powershell
python tools/firmware-package/package_tool.py inspect release.skfw --public-key approved-public-key.bin --key-id skidrive-release-2026-01
```

Even a verified release returns `device_ready_to_flash: false`: this offline utility
cannot perform device checks. Port the validation rules into the team's chosen
Windows stack using maintained ZIP, JSON, SHA-256 and Ed25519 libraries. The helper
is a tested integration reference, not a replacement for app/release security review.

No private keys, programming executables, drivers, source code, customer logs or
config.json belong inside the customer package. Firmware stays in Teensy's program
flash; preserve existing SD files and the hardware-derived logger ID during updates.
