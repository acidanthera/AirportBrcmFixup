AirportBrcmFixup Changelog
============================
#### v1.0.0
- Initial release

#### v1.0.1
- Refactoring, patch for "Failed PCIe configuration" added"
- Patch for 5Ghz / Country code added
- A new pseudo device FakeBrcm is introduced. It is started instead of original broadcom driver. Original driver is started in processKext.

#### v1.0.2
- 5 Ghz/country code patch reimplemented. Boot arg brcmfx-country=XX is supported.

#### v1.0.3
- Don't use FakeBrcm under 10.13 and higher.

#### v1.0.4
- Bug in release version has been fixed (EXPORT added).
- Debugging output was improved.
