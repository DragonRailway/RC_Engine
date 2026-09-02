## 1. Release Packaging Implementation

- [x] 1.1 Implement manifest generation and segmented binary packaging in `scripts/package_release.py`
- [x] 1.2 Add SHA-256 calculation and `boot_app0.bin` dynamic resolution/generation in `scripts/package_release.py`
- [x] 1.3 Update packaging logic to produce `dist/RC_Engine-<version>-<chip>-<board>.zip` with `manifest.json`, `bootloader.bin`, `partitions.bin`, `otadata.bin`, and `app.bin`

## 2. Verification and Spec Sync

- [x] 2.1 Test `package_release.py` packaging on local build artifacts and verify archive contents and manifest schema
- [x] 2.2 Verify GitHub Actions workflow compatibility in `.github/workflows/release.yml`
