AirportBrcmFixup
==================

[![Build Status](https://github.com/acidanthera/AirportBrcmFixup/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/AirportBrcmFixup/actions) [![Scan Status](https://scan.coverity.com/projects/16401/badge.svg?flat=1)](https://scan.coverity.com/projects/16401)

An open source kernel extension providing a set of patches required for non-native Airport Broadcom Wi-Fi cards.

#### Notes
This repository should be compiled with [Lilu](https://github.com/vit9696/Lilu) ***v1.2.4*** or greater, otherwise the compilation will fail! Currently requires macOS 10.10 or newer.

Note: ***Debug version of Lilu.kext should be put in the same folder as BrcmWLFixup! And they should be also used together!***

#### Features
- Supports AirPort_Brcm4360, AirPort_BrcmNIC and AirPort_BrcmNIC_MFG
- Implements patch for chip identificator checking (0xaa52), in <=10.12 it's required for successful driver starting
- Implements patch for "Failed PCIe configuration patch", required for some cards 
- Implements patch for "Third Party Device" (returns vendor name "Apple")
- Implements patch for removing of white-list check
- Implements 5Ghz and country code patch (US by default)
- Implements WOWL (WoWLAN) disabler (turned on by default) - no low speed after wake
- Implements patch to disable LPO panic in AirPortBrcmNIC

#### Installation types
- Clover Airport Fix + AirportBrcmFixup
- Clover Airport Fix + AddProperties(fake device-id) + AirportBrcmFixup (preferable)
- Clover Airport Fix + AirportBrcmFixup + FakePCIID + FakePCIID_Broadcom_WiFi.kext
- AirportBrcmFixup + FakePCIID + FakePCIID_Broadcom_WiFi.kext

#### Boot-args
- `-brcmfxdbg` turns on debugging output
- `-brcmfxbeta` enables loading on unsupported osx
- `-brcmfxoff` disables kext loading
- `-brcmfxwowl` enables WOWL (WoWLAN) - it is disabled by default
- `-brcmfx-alldrv` allows patching for all supported drivers, disregarding current system version (see  `Matching device-id and kext name in different macOS versions`)

#### Specific boot-args and ioreg properties
- `brcmfx-country=XX` changes the country code to XX (US, CN, #a, ...), also can be injected via DSDT or Properties → DeviceProperties in bootloader
- `brcmfx-aspm`  overrides value used for pci-aspm-default
- `brcmfx-wowl` enables/disables WoWLAN patch
- `brcmfx-delay` delays start of native broadcom driver for specified amount of milliseconds. It can solve panics or missing wi-fi device in Monterey. You can start with 15 seconds (brcmfx-delay=15000) and successively reduce this value until you notice instability in boot.
- `brcmfx-alldrv` allows patching for all supported drivers, disregarding current system version (see  `Matching device-id and kext name in different macOS versions`)
- `brcmfx-driver=0|1|2|3` enables only one kext for loading, 0 - AirPortBrcmNIC-MFG, 1 - AirPortBrcm4360, 2 - AirPortBrcmNIC, 3 - AirPortBrcm4331, also can be injected via DSDT or Properties → DeviceProperties in bootloader

Possible values for brcmfx-aspm (and pci-aspm-default):
- `0` disables ASPM
- `kIOPCIExpressASPML0s` = 0x00000001,
- `kIOPCIExpressASPML1` = 0x00000002,
- `kIOPCIExpressCommonClk` = 0x00000040,
- `kIOPCIExpressClkReq` = 0x00000100



#### Credits
- [Apple](https://www.apple.com) for macOS  
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/vit9696/Lilu) and great help in implementing some features
- [PMheart](https://github.com/PMheart) for the initial idea and implementation
- darkvoid & RehabMan for FakePCIID implementation (partly used in AirportBrcmFixup)
- [al3xtjames](https://github.com/al3xtjames) for disabling LPO panic in AirPortBrcmNIC 
- [lvs1974](https://applelife.ru/members/lvs1974.53809/) for writing the software and maintaining it


#### Matching device-id and kext name in different macOS versions
[10.8]
- AirPortBrcm4360: 43a0, IOProbeScore = 641
- AirPortBrcm4331: 4331, 4353, 432b, IOProbeScore = 615

[10.9]
- AirPortBrcm4360: 43a0, IOProbeScore = 842
- AirPortBrcm4331: 4331, 4353, 432b, IOProbeScore = 700

[10.10]
- AirPortBrcm4360: 43ba, 43a3, 43a0, 4331, 4353, IOProbeScore = 930
- AirPortBrcm4331: 4331, 4353, 432b, IOProbeScore = 900

[10.11]
- AirPortBrcm4360: 43ba, 43a3, 43a0, 4331, 4353, IOProbeScore = 1040
- AirPortBrcm4331: 4331, 4353, 432b, IOProbeScore = 800

[10.12]
- AirPortBrcm4360: 43ba, 43a3, 43a0, 4331, 4353, IOProbeScore = 1152
- AirPortBrcm4331: 4331, 4353, 432b, IOProbeScore = 800

[10.13]
- AirPortBrcm4360: 4331, 4353, IOProbeScore = 1240
- AirPortBrcm4331: 4331, 4353, 432b, IOProbeScore = 800
- AirPortBrcmNIC: 43ba, 43a3, 43a0, IOProbeScore = 1241
- AirPortBrcmNIC-MFG: 43ba, 43a3, 43a0, IOProbeScore = -1000

[10.14]
- AirPortBrcm4360: 4331, 4353, IOProbeScore = 1400
- AirPortBrcm4331: 4331, 4353, 432b, IOProbeScore = 800
- AirPortBrcmNIC: 43ba, 43a3, 43a0, IOProbeScore = 1400
- AirPortBrcmNIC-MFG: 43ba, 43a3, 43a0, IOProbeScore = -1000

[10.15]
- AirPortBrcm4360: 4331, 4353, IOProbeScore = 1400
- AirPortBrcm4331: removed
- AirPortBrcmNIC: 43ba, 43a3, 43a0, IOProbeScore = 1400
- AirPortBrcmNIC-MFG: removed

[11]
- AirPortBrcm4360: removed
- AirPortBrcm4331: removed
- AirPortBrcmNIC: 43ba, 43a3, 43a0, IOProbeScore = 1400
- AirPortBrcmNIC-MFG: removed

[12]
- AirPortBrcm4360: removed
- AirPortBrcm4331: removed
- AirPortBrcmNIC: 43ba, 43a3, 43a0, IOProbeScore = 1400, kext is moved to the new location - IO80211FamilyLegacy.kext/Contents/PlugIns
- AirPortBrcmNIC-MFG: removed

[13]
- AirPortBrcm4360: removed
- AirPortBrcm4331: removed
- AirPortBrcmNIC: 43ba, 43a3, 43a0, IOProbeScore = 1400, kext is moved to the new location - IO80211FamilyLegacy.kext/Contents/PlugIns
- AirPortBrcmNIC-MFG: removed

Explanation in russian language: (https://applelife.ru/threads/airportbrcmfixup-lilu-plagin-s-naborom-patchej-dlja-wi-fi-kart-broadcom.2355103/page-16#post-751173)

Until version [10.13] there were only two kexts and for the most cases FakeID = 43a0 was enough to get working Wi-Fi.
In [10.13] (and later) one family was separated into AirPortBrcm4360 (probably will be removed in 10.15), AirPortBrcmNIC and AirPortBrcmNIC-MFG.
A proper fake-id has to be considered to load appropriate kext. Some device-id have limitations in one kext (like 5 Ghz range), but do not have it another.

#### Please pay attention
In 11+ class AirPortBrcm4360 has been completely removed. Using of injector kext with such class name and matched ```vendor-id:device-id``` blocks
loading of original airport kext. To address this issue and keep compatibility with older systems injectors for AirPortBrcm4360 and AirPortBrcmNIC were removed
from main Info.plist file. Instead, the two new kext injectors are deployed in PlugIns folder: AirPortBrcm4360_Injector.kext and AirPortBrcmNIC_Injector.kext.
***You have to block (or remove) AirPortBrcm4360_Injector.kext in 11+.*** In OpenCore you can specify MaxKernel 19.9.9 for AirPortBrcm4360_Injector.kext.
In Clover you can have two different AirportBrcmFixup.kext, but in kext folder with version name 11 and 12 and 13 AirportBrcmFixup.kext must not contain AirPortBrcm4360_Injector.kext. You don't need these injectors at all if your ```vendor-id:device-id``` is natively supported by AirPortBrcmNIC or AirPortBrcm4360 (your device-id is included into Info.plist in these kexts).

