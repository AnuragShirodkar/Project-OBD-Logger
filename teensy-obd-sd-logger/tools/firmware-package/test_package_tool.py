import hashlib
import json
import tempfile
import unittest
import zipfile
from pathlib import Path

from package_tool import PackageError, create_template, inspect_package

try:
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
except ImportError:
    Ed25519PrivateKey = None


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = create_template(self.directory.name)
        with zipfile.ZipFile(self.path) as archive:
            self.files = {name: archive.read(name) for name in archive.namelist()}
        self.manifest = json.loads(self.files["manifest.json"])

    def save(self):
        self.files["manifest.json"] = json.dumps(self.manifest).encode()
        with zipfile.ZipFile(self.path, "w") as archive:
            for name, content in self.files.items():
                archive.writestr(name, content)

    def test_template_imports_but_never_approves_install(self):
        result = inspect_package(self.path)
        self.assertFalse(result["package_approved"])
        self.assertFalse(result["device_ready_to_flash"])

    def test_modified_firmware_is_rejected(self):
        self.files["firmware.hex"] += b"broken"
        self.save()
        with self.assertRaisesRegex(PackageError, "hash mismatch"):
            inspect_package(self.path)

    def test_bad_hex_rejected_even_with_matching_hash(self):
        image = self.files["firmware.hex"].replace(b"60009A", b"600000")
        self.files["firmware.hex"] = image
        self.manifest["payload"].update(size_bytes=len(image), sha256=hashlib.sha256(image).hexdigest())
        self.save()
        with self.assertRaisesRegex(PackageError, "checksum"):
            inspect_package(self.path)

    def test_zip_path_traversal_rejected(self):
        self.files["../firmware.hex"] = b"bad"
        self.save()
        with self.assertRaisesRegex(PackageError, "ZIP entries"):
            inspect_package(self.path)
        self.assertFalse((Path(self.directory.name).parent / "firmware.hex").exists())

    def test_duplicate_zip_entries_rejected(self):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)
            with zipfile.ZipFile(self.path, "a") as archive:
                archive.writestr("manifest.json", b"{}")
        with self.assertRaisesRegex(PackageError, "Duplicate ZIP"):
            inspect_package(self.path)

    def test_wrong_board_rejected(self):
        self.manifest["target"]["board"] = "teensy40"
        self.save()
        with self.assertRaisesRegex(PackageError, "Unsupported board"):
            inspect_package(self.path)

    def test_renaming_template_release_does_not_authorize_it(self):
        self.manifest["package_kind"] = "release"
        self.save()
        with self.assertRaisesRegex(PackageError, "Release cannot"):
            inspect_package(self.path)

    def test_duplicate_json_property_rejected(self):
        self.files["manifest.json"] = b'{"format":1,"format":2}'
        with zipfile.ZipFile(self.path, "w") as archive:
            for name, content in self.files.items():
                archive.writestr(name, content)
        with self.assertRaisesRegex(PackageError, "Duplicate JSON"):
            inspect_package(self.path)

    def test_release_without_signature_rejected(self):
        image = self.files["firmware.hex"].replace(b"00000000FC", b"01000000FB")
        self.files["firmware.hex"] = image
        self.manifest["payload"].update(size_bytes=len(image), sha256=hashlib.sha256(image).hexdigest())
        self.manifest["package_kind"] = "release"
        self.manifest["target"]["hardware_revisions"] = ["REV_A"]
        self.manifest["signing"] = {
            "algorithm": "Ed25519", "key_id": "release-test", "file": "signature.ed25519"}
        self.save()
        with self.assertRaisesRegex(PackageError, "Missing/invalid"):
            inspect_package(self.path)

    @unittest.skipIf(Ed25519PrivateKey is None, "cryptography not installed")
    def test_signed_release_and_manifest_tampering(self):
        from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
        private = Ed25519PrivateKey.generate()  # Ephemeral test key; never persisted.
        public = private.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw)
        image = self.files["firmware.hex"].replace(b"00000000FC", b"01000000FB")
        self.files["firmware.hex"] = image
        self.manifest["payload"].update(size_bytes=len(image), sha256=hashlib.sha256(image).hexdigest())
        self.manifest["package_kind"] = "release"
        self.manifest["target"]["hardware_revisions"] = ["REV_A"]
        self.manifest["signing"] = {
            "algorithm": "Ed25519", "key_id": "test", "file": "signature.ed25519"}
        self.files["signature.ed25519"] = private.sign(json.dumps(self.manifest).encode())
        self.save()
        result = inspect_package(self.path, {"test": public})
        self.assertTrue(result["package_approved"])
        self.assertFalse(result["device_ready_to_flash"])
        with self.assertRaisesRegex(PackageError, "trust store"):
            inspect_package(self.path)
        wrong_key = Ed25519PrivateKey.generate().public_key().public_bytes(Encoding.Raw, PublicFormat.Raw)
        with self.assertRaisesRegex(PackageError, "signature verification failed"):
            inspect_package(self.path, {"test": wrong_key})
        self.manifest["firmware_version"] = "99.0.0"
        self.save()
        with self.assertRaisesRegex(PackageError, "signature verification failed"):
            inspect_package(self.path, {"test": public})

    def test_expanded_size_limit(self):
        self.files["firmware.hex"] = b"0" * (16 * 1024 * 1024 + 1)
        with zipfile.ZipFile(self.path, "w", zipfile.ZIP_DEFLATED) as archive:
            for name, content in self.files.items():
                archive.writestr(name, content)
        with self.assertRaisesRegex(PackageError, "Expanded package"):
            inspect_package(self.path)

    def test_template_matches_json_schema(self):
        try:
            import jsonschema
        except ImportError:
            self.skipTest("jsonschema not installed")
        schema_path = Path(__file__).resolve().parents[2] / "docs/firmware-package/manifest.schema.json"
        schema = json.loads(schema_path.read_text(encoding="utf-8"))
        jsonschema.Draft202012Validator.check_schema(schema)
        jsonschema.validate(self.manifest, schema)


if __name__ == "__main__":
    unittest.main()
