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

#### v1.0.5
- Fix for High Sierra Developer Beta 7 (loading order)

#### v1.0.6
- Pass property table to the newly created instance of a driver. Disable FakeBrcm if processKext was called earlier.

#### v1.0.7
- New boot-args: wl_msg_level=XXXXX and wl_msg_level2=XXXXX pass specified values to respective fields in driver, allows to get a full log
