"""Create/inspect SkiDrive packages. Never programs or contacts a logger.

Python 3.10+. Release inspection also needs cryptography and a trusted public key.
"""
import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path

LIMIT = 16 * 1024 * 1024
FILES = {"manifest.json", "firmware.hex", "release-notes.txt"}
DEMO_HEX = b":0200000460009A\n:0400000000000000FC\n:00000001FF\n"


class PackageError(ValueError):
    pass


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise PackageError("Duplicate JSON property: " + key)
        result[key] = value
    return result


def exact_fields(value, names, label):
    if not isinstance(value, dict) or set(value) != set(names.split()):
        raise PackageError("Unexpected or missing fields in " + label)


def validate_hex(payload):
    """Check Intel HEX framing/checksums and Teensy 4.1 program address range.

    This does not prove that an image contains a bootable or correct application.
    """
    try:
        lines = payload.decode("ascii").splitlines()
    except UnicodeDecodeError as exc:
        raise PackageError("Firmware is not ASCII Intel HEX") from exc
    base, eof, occupied = 0, False, set()
    for line in lines:
        if eof or not re.fullmatch(r":[0-9a-fA-F]+", line):
            raise PackageError("Invalid Intel HEX record")
        try:
            record = bytes.fromhex(line[1:])
        except ValueError as exc:
            raise PackageError("Invalid Intel HEX encoding") from exc
        if len(record) < 5 or len(record) != record[0] + 5 or sum(record) % 256:
            raise PackageError("Invalid Intel HEX length/checksum")
        count, address, kind = record[0], int.from_bytes(record[1:3], "big"), record[3]
        data = record[4:-1]
        if kind == 0:
            if address + count > 0x10000:
                raise PackageError("HEX record crosses address boundary")
            for offset in range(count):
                location = base + address + offset
                if not 0x60000000 <= location < 0x607C0000:
                    raise PackageError("HEX data outside Teensy 4.1 program flash")
                if location in occupied:
                    raise PackageError("Overlapping Intel HEX data")
                occupied.add(location)
        elif kind == 1 and count == 0 and address == 0:
            eof = True
        elif kind == 4 and count == 2 and address == 0:
            base = int.from_bytes(data, "big") << 16
        elif kind == 5 and count == 4 and address == 0:
            pass  # Entry address; the uploader determines boot behaviour.
        else:
            raise PackageError("Unsupported Intel HEX record type/shape")
    if not eof or not occupied:
        raise PackageError("HEX needs data and a final EOF record")
    return len(occupied)


def inspect_package(path, trusted_keys=None):
    if Path(path).stat().st_size > LIMIT:
        raise PackageError("Package exceeds 16 MiB")
    try:
        with zipfile.ZipFile(path) as archive:
            entries = archive.infolist()
            names = [item.filename for item in entries]
            if len(names) != len(set(names)):
                raise PackageError("Duplicate ZIP entries")
            if set(names) not in (FILES, FILES | {"signature.ed25519"}):
                raise PackageError("Unexpected/missing ZIP entries; no folders allowed")
            if sum(item.file_size for item in entries) > LIMIT:
                raise PackageError("Expanded package exceeds 16 MiB")
            if any(item.flag_bits & 1 or item.compress_type not in
                   (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED) or
                   ((item.external_attr >> 16) & 0o170000) == 0o120000
                   for item in entries):
                raise PackageError("Encrypted, linked or unsupported ZIP entry")
            # No extraction: untrusted ZIP paths never reach the filesystem.
            content = {name: archive.read(name) for name in names}
    except (zipfile.BadZipFile, RuntimeError, NotImplementedError) as exc:
        raise PackageError("Unreadable ZIP package") from exc
    if len(content["manifest.json"]) > 16384 or len(content["release-notes.txt"]) > 65536:
        raise PackageError("Metadata exceeds size limit")
    try:
        manifest = json.loads(content["manifest.json"].decode("utf-8"),
                              object_pairs_hook=unique_object)
        content["release-notes.txt"].decode("utf-8")
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PackageError("Invalid UTF-8/JSON metadata") from exc
    exact_fields(manifest, "format format_version package_kind product device_type "
                 "firmware_version target payload release_notes signing", "manifest")
    for key, expected in {"format": "skidrive-firmware-package", "format_version": 1,
                          "product": "SkiDrive", "device_type": "OBD_LOGGER",
                          "release_notes": "release-notes.txt"}.items():
        if type(manifest[key]) is not type(expected) or manifest[key] != expected:
            raise PackageError("Unsupported " + key)
    if not isinstance(manifest["firmware_version"], str) or not re.fullmatch(
            r"[0-9]+\.[0-9]+\.[0-9]+(?:-[A-Za-z0-9.-]+)?", manifest["firmware_version"]):
        raise PackageError("Invalid firmware_version")
    target = manifest["target"]
    exact_fields(target, "board mcu hardware_revisions usb_update_protocol", "target")
    if target["board"] != "teensy41-standard" or target["mcu"] != "IMXRT1062" or \
            type(target["usb_update_protocol"]) is not int or target["usb_update_protocol"] != 1:
        raise PackageError("Unsupported board/MCU/protocol")
    revisions = target["hardware_revisions"]
    if not isinstance(revisions, list) or not revisions or any(
            not isinstance(rev, str) or not re.fullmatch(r"[A-Za-z0-9_.-]{1,32}", rev)
            for rev in revisions) or len(set(revisions)) != len(revisions):
        raise PackageError("Invalid explicit hardware revision list")
    payload = manifest["payload"]
    exact_fields(payload, "file format size_bytes sha256", "payload")
    image = content["firmware.hex"]
    if payload["file"] != "firmware.hex" or payload["format"] != "intel-hex" or \
            type(payload["size_bytes"]) is not int or payload["size_bytes"] != len(image) or \
            payload["sha256"] != hashlib.sha256(image).hexdigest():
        raise PackageError("Firmware name/format/size/hash mismatch")
    data_bytes = validate_hex(image)
    signature_verified = False
    if manifest["package_kind"] == "integration-template":
        if manifest["signing"] is not None or "signature.ed25519" in content:
            raise PackageError("Template must be explicitly unsigned")
    elif manifest["package_kind"] == "release":
        if "unassigned" in revisions or image == DEMO_HEX:
            raise PackageError("Release cannot use unassigned hardware or demo firmware")
        signing = manifest["signing"]
        exact_fields(signing, "algorithm key_id file", "signing")
        if signing["algorithm"] != "Ed25519" or signing["file"] != "signature.ed25519" or \
                not isinstance(signing["key_id"], str) or not re.fullmatch(
                    r"[A-Za-z0-9_-]{1,64}", signing["key_id"]):
            raise PackageError("Unsupported signature declaration")
        if len(content.get("signature.ed25519", b"")) != 64:
            raise PackageError("Missing/invalid Ed25519 signature")
        if not trusted_keys or signing["key_id"] not in trusted_keys:
            raise PackageError("Signing key is not in the application's trust store")
        try:
            from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
        except ImportError as exc:
            raise PackageError("Install cryptography to verify signed releases") from exc
        try:
            Ed25519PublicKey.from_public_bytes(trusted_keys[signing["key_id"]]).verify(
                content["signature.ed25519"], content["manifest.json"])
        except Exception as exc:
            raise PackageError("Release signature verification failed") from exc
        signature_verified = True
    else:
        raise PackageError("Unknown package_kind")
    return {"manifest": manifest, "signature_verified": signature_verified,
            "image_data_bytes": data_bytes,
            "package_approved": signature_verified,
            "device_ready_to_flash": False,
            "message": "Device identity, compatibility and safe shutdown checks are still required."
            if signature_verified else "INTEGRATION TEMPLATE ONLY - installation prohibited."}


def create_template(destination):
    manifest = {
        "format": "skidrive-firmware-package", "format_version": 1,
        "package_kind": "integration-template", "product": "SkiDrive",
        "device_type": "OBD_LOGGER", "firmware_version": "0.0.0-integration",
        "target": {"board": "teensy41-standard", "mcu": "IMXRT1062",
                   "hardware_revisions": ["unassigned"], "usb_update_protocol": 1},
        "payload": {"file": "firmware.hex", "format": "intel-hex",
                    "size_bytes": len(DEMO_HEX), "sha256": hashlib.sha256(DEMO_HEX).hexdigest()},
        "release_notes": "release-notes.txt", "signing": None,
    }
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    files = {
        "manifest.json": (json.dumps(manifest, indent=2) + "\n").encode(),
        "firmware.hex": DEMO_HEX,
        "release-notes.txt": b"INTEGRATION TEMPLATE ONLY.\nNot executable logger firmware.\n"
                             b"Use to test package import, preview and blocked installation.\n"
                             b"Never pass this HEX to a programmer.\n",
    }
    for name, content in files.items():
        (destination / name).write_bytes(content)
    package = destination / "SkiDrive_0.0.0-integration_TEMPLATE_DO_NOT_FLASH.skfw"
    with zipfile.ZipFile(package, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, content in files.items():
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, content)
    return package


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    create = commands.add_parser("template")
    create.add_argument("destination", type=Path)
    inspect = commands.add_parser("inspect")
    inspect.add_argument("package", type=Path)
    inspect.add_argument("--public-key", type=Path, help="Trusted Ed25519 raw 32-byte public key")
    inspect.add_argument("--key-id", help="Trusted identifier paired with --public-key")
    args = parser.parse_args()
    try:
        if args.command == "template":
            print(create_template(args.destination))
        else:
            if bool(args.public_key) != bool(args.key_id):
                raise PackageError("Supply --public-key and --key-id together")
            keys = {args.key_id: args.public_key.read_bytes()} if args.public_key else None
            print(json.dumps(inspect_package(args.package, keys), indent=2))
    except (PackageError, OSError) as exc:
        parser.exit(1, "REJECTED: " + str(exc) + "\n")


if __name__ == "__main__":
    main()
