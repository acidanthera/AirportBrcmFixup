AirportBrcmFixup Changelog
============================
#### v1.0.0
- Initial release

#### v1.0.1
- Refactoring, patch for "Failed PCIe configuration" added"
- Patch for 5Ghz / Country code added
- A new pseudo device FakeBrcm is introduced. It is started instead of original broadcom driver. Original driver is started in processKext.
