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
- FakePCIID functionality is built-in

#### v1.0.8
- 5 Ghz/country code patch fixed (low speed after wake up)

#### v1.0.9
- Parameter brcmfx-country can be located in ioreg (in provider, ARPT for example)

#### v1.0.10
- WOWL (WoWLAN) is disabled by default. Use boot-arg -brcmfxwowl to enable it.

#### v.1.0.11
- Fix co-existing with FakePCIID_Broadcom_WiFi.kext & FakePCIID

#### v1.1.0
- XCode 9 & Lilu 1.2.0 compatibility fixes (Lilu 1.2.0 is required)

#### v1.1.1
- Cosmetic change

#### v1.1.2
- FakeBrcm is destroyed & removed from ioreg, there is no need to start original ariport driver, gIOCatalogue->removeDrivers called for FakeBrcm does this.

#### v1.1.3
- Allow to load all matched drivers for broadcom wi-fi (in case if there is a concurrency), except AirPortBrcmNIC-MFG.kext
- AirPortBrcmNIC-MFG probe & start are hooked to prevent using of this driver
- Fix a name conflict for config variable

#### v1.1.4
- Add patch to disable LPO panic in AirPortBrcmNIC: on some cards (DW1820A/BCM4350), the external LPO clock is sometimes
not set upon initialization. While harmless in AirPortBrcm4360, AirPortBrcmNIC panics if this occurs. As it wasn't present previously,
_osl_panic for string "32KHz LPO Clock not running" does not produce panic in AirPortBrcmNIC. Idea by al3xtjames.

#### v1.1.5
- Mojave compatible

#### v1.1.6
- Try to resolve linker issue with __ZN13Configuration18bootargBrcmCountryE (with older Xcode versions)

#### v1.1.7
- Use separate variables and methods wlc_set_countrycode_rev & siPmuFvcoPllreg for every driver
- Introduce a new boot-arg brcmfx-driver=[0|1|2], 0 - AirPort.BrcmNIC-MFG, 1 - AirPort.Brcm4360, 2 - AirPort.BrcmNIC
- Remove redundancy boot-args wl_msg_level and wl_msg_level2 (they are supported by Apple)
- pci14e4,43b1 & pci14e4,43b2 matched also to AirPort_BrcmNIC
