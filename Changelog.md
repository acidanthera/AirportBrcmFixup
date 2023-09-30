AirportBrcmFixup Changelog
============================
#### v2.1.8
- Added constants for macOS 14 support for use with OCLP

#### v2.1.7
- Override methods AirPort_BrcmNIC::setTX_NSS, AirPort_BrcmNIC::getTX_NSS and AirPort_BrcmNIC::getNSS to investigate NSS issues. 
  Original method is called, and debug version of kext (with boot-arg -brcmfxdbg) prints info into log

#### v2.1.6
- Added constants for macOS 13 support

#### v2.1.5
- Fix compilation issues in Xcode 13.3.1
- Fix patching of AirPortBrcmNIC for Monterey (kext located in IO80211FamilyLegacy.kext/PlugIns)

#### v2.1.4
- boot-arg and property `brcmfx-delay` (Number) delays start of native broadcom driver for specified amount of milliseconds. Can be required for successful start in macOS Monterey 12.1 and newer versions. 

#### v2.1.3
- Added constants for macOS 12 support

#### v2.1.2
- Do not patch airport drivers missing in the system (set of available drivers depends on the system version). 
For systems with manually added airport drivers this behaviour can be overridden by boot-arg or property `brcmfx-alldrv`

#### v2.1.1
- Fix an issue with posponed matching (method IOTimerEventSource::timerEventSource could fail)

#### v2.1.0
- Add pci14e4,4331, pci14e4,4353  and pci14e4,4357  into AirPortBrcmNIC_Injector.kext  (in 11.0 only AirPortBrcmNIC can support these devices)
- Added MacKernelSDK with Xcode 12 compatibility
- Fixed macOS 10.8 compatibility (without ASPM support)

#### v2.0.9
- boot-arg and property `brcmfx-aspm` supports special value `255` in order to skip logic disabling APSM for 0x14e4:0x43a3 (DW1820A). 
It can be used if you have masked pin 53 (CLKREQ#) and APSM L0|L1 is working.
- Improve service matching (the old implementation could cause hangs on boot)

#### v2.0.8
- Added constants for 11.0 support
- Property `pci-aspm-default` with value 0 is not required for Broadcom BCM4350 chipset (with non-apple `subsystem-vendor-id`), 
since now it is injected/corrected and method IOPCIFamily::setASPMState called for provider to disable ASPM immediately.
- Add required dependencies into OSBundleLibraries section
- Remove injectors for AirPortBrcm4360 and AirPortBrcmNIC from main Info.plist and move them into separate plugins AirPortBrcm4360_Injector and 
AirPortBrcmNIC_Injector (kexts with plist only).
Under 11.0 (Big Sur) plugin AirPortBrcm4360_Injector.kext must be blocked by MaxKernel 19.9.9 or just removed, otherwise it will block loading of AirPortBrcmNIC
since class AirPortBrcm4360 is unsupported.
- Check whether brcmfx-driver value is incorrect (if specified value is unsupported in current osx system)
- Support boot-arg and property `brcmfx-aspm` to override value used for pci-aspm-default 

#### v2.0.7
- Fix for kernel panic when FakeBrcm is detached from provider (provider->close(service))

#### v2.0.6
- Add previous implementation of wowl disabler (both methods wlc_wowl_enable and wowCapablePlatform must be overridden)
- Add support for DW1820A CN-096JNT & Fix freezing issues (thanks to Sniki)
- Read property brcmfx-driver from provider's IOReg entry (can be injected by bootloader as well as brcmfx-country property)

#### v2.0.5
- Re-implement wowl disabler (fix bug #615)

#### v2.0.4
- Minor improvement in method findService (based on panic report analysis)
- Do not use removeObject, use getNextObject

#### v2.0.3
- Unified binary archive names

#### v2.0.2
- Fixed compatibility with 10.15

#### v2.0.1
- Allow loading on 10.15 without `-lilubetaall`

#### v2.0.0
- checkBoardId patch has been improved: returns false for boardId in cpmChanSwitchWhitelist
- Service provider name is changed to ARPT automatically

#### v1.1.9
- Fix issue with IOProbeScore for Airport_BrcmNIC

#### v1.1.8
- Support AirPort_Brcm4331

#### v1.1.7
- Use separate variables and methods wlc_set_countrycode_rev & siPmuFvcoPllreg for every driver
- Introduce a new boot-arg brcmfx-driver=[0|1|2], 0 - AirPort.BrcmNIC-MFG, 1 - AirPort.Brcm4360, 2 - AirPort.BrcmNIC
- Remove redundancy boot-args wl_msg_level and wl_msg_level2 (they are supported by Apple)
- pci14e4,43b1 & pci14e4,43b2 matched also to AirPort_BrcmNIC

#### v1.1.6
- Try to resolve linker issue with __ZN13Configuration18bootargBrcmCountryE (with older Xcode versions)

#### v1.1.5
- Mojave compatible

#### v1.1.4
- Add patch to disable LPO panic in AirPortBrcmNIC: on some cards (DW1820A/BCM4350), the external LPO clock is sometimes
not set upon initialization. While harmless in AirPortBrcm4360, AirPortBrcmNIC panics if this occurs. As it wasn't present previously,
_osl_panic for string "32KHz LPO Clock not running" does not produce panic in AirPortBrcmNIC. Idea by al3xtjames.

#### v1.1.3
- Allow to load all matched drivers for broadcom wi-fi (in case if there is a concurrency), except AirPortBrcmNIC-MFG.kext
- AirPortBrcmNIC-MFG probe & start are hooked to prevent using of this driver
- Fix a name conflict for config variable

#### v1.1.2
- FakeBrcm is destroyed & removed from ioreg, there is no need to start original ariport driver, gIOCatalogue->removeDrivers called for FakeBrcm does this.

#### v1.1.1
- Cosmetic change

#### v1.1.0
- XCode 9 & Lilu 1.2.0 compatibility fixes (Lilu 1.2.0 is required)

#### v.1.0.11
- Fix co-existing with FakePCIID_Broadcom_WiFi.kext & FakePCIID

#### v1.0.10
- WOWL (WoWLAN) is disabled by default. Use boot-arg -brcmfxwowl to enable it.

#### v1.0.9
- Parameter brcmfx-country can be located in ioreg (in provider, ARPT for example)

#### v1.0.8
- 5 Ghz/country code patch fixed (low speed after wake up)

#### v1.0.7
- New boot-args: wl_msg_level=XXXXX and wl_msg_level2=XXXXX pass specified values to respective fields in driver, allows to get a full log
- FakePCIID functionality is built-in

#### v1.0.6
- Pass property table to the newly created instance of a driver. Disable FakeBrcm if processKext was called earlier.

#### v1.0.5
- Fix for High Sierra Developer Beta 7 (loading order)

#### v1.0.4
- Bug in release version has been fixed (EXPORT added).
- Debugging output was improved.

#### v1.0.3
- Don't use FakeBrcm under 10.13 and higher.

#### v1.0.2
- 5 Ghz/country code patch reimplemented. Boot arg brcmfx-country=XX is supported.

#### v1.0.1
- Refactoring, patch for "Failed PCIe configuration" added"
- Patch for 5Ghz / Country code added
- A new pseudo device FakeBrcm is introduced. It is started instead of original broadcom driver. Original driver is started in processKext.

#### v1.0.0
- Initial release
