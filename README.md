**Taken From Release:**
Bellow is a list of boot-args that may be used with this kext:

brcmfx-nss1=1     = may be used to force NSS:1 (for testing purposes) 
brcmfx-aspm       = (used to be needed to avoid this power management bug with c-state).
                    May still be applied manually if AutoASPM results aren't Satisfactory. 
brcmfx-delay       = existing initialization delay option to improve Airplay connectivity. 
-brcmfxdbg         = Existing AirportBrcmFixup debug logging flag

**Here’s the complete progression of changes made to this Kext so far:**

	•	1. Added NSS debugging/probing to find the NSS:1 problem. We used AirportBrcmFixup’s DEBUG-side NSS hooks and temporarily routed _wlc_stf_txchain_set so we could see what the Broadcom driver was actually doing rather than just trusting the reported link rate. The logs repeatedly showed the driver operating with NSS=1, and the temporary STF hook exposed the important call where the driver requested TX chain mask 1.
	•	2. Identified the actual cold-boot NSS bug. We narrowed it down to _wlc_stf_txchain_set() being called with reason == 2 and a low-byte chain mask of 1. That is the call that was constraining the known 2×2 card to one transmit chain. Instead of faking the reported NSS, our fix changes that specific request from chain mask 1 to 3.
	•	3. Made the NSS fix surgical. The callback only intervenes when all of the conditions match. It preserves all of the upper bits of the original argument and changes only the low byte: chain = (chain & ~0xFFULL) | 0x3. Everything else goes straight through untouched to the original Broadcom function.
	•	4. Converted the NSS experiment into a production Release-build fix. The early STF hook was DEBUG-only. We moved the _wlc_stf_txchain_set route outside the DEBUG-only machinery and route it directly when AirPort_BrcmNIC loads, so the correction remains active in a normal Release AirportBrcmFixup build.
	•	5. Added hardware gating for AutoNSS2. We do not apply the 1 → 3 chain correction to every BCM4360. AutoNSS2 is enabled only for 14e4:43a0 / 106b:0117, the Apple-compatible 2×2 identity we've actually established is appropriate for this correction. The 3×3 BCM94360 variants are deliberately not forced to mask 3.
	•	6. Added the brcmfx-nss1=1 escape hatch. The custom kext gained a new boot argument specifically for testing/regression purposes. brcmfx-nss1=1 disables AutoNSS2 and lets the original NSS:1 behavior occur. Internally we added force_nss1 and enable_nss2 state to support this.
	•	7. Added automatic PCI identity detection. AirportBrcmFixup now reads the Wi-Fi device's PCI vendor ID, device ID, subsystem vendor ID, and subsystem device ID itself. That lets the kext decide what policy to use based on the actual Broadcom card rather than requiring you to manually select behavior with boot arguments.
	•	8. Added userAspmOverride tracking. We changed the existing brcmfx-aspm handling so the kext remembers whether the value came explicitly from the user—either as a boot argument or IORegistry property. If you explicitly specify brcmfx-aspm=..., your value always wins and AutoASPM does not interfere.
	•	9. Added AutoASPM. For BCM4360/BCM94360 hardware (14e4:43a0), the kext can now choose an ASPM policy automatically instead of requiring brcmfx-aspm in boot-args. We use AirportBrcmFixup's existing 0xFF sentinel to mean essentially don't override pci-aspm-default; preserve the firmware/platform configuration. For appropriate non-Apple BCM4360 identities we use 0x102, decimal 258, meaning L1 + CLKREQ.
	•	10. Initially supported the known Apple BCM94360 identities explicitly. The earlier AutoASPM version recognized 106b:0111, 106b:0117, and 106b:0134 and preserved platform ASPM for those cards. Other 14e4:43a0 identities were moved to 258/L1+CLKREQ.
	•	11. Generalized non-Apple BCM4360 handling. Instead of recognizing only one particular third-party subsystem ID for the 258 policy, we expanded that so other/non-Apple 14e4:43a0 identities receive 0x102 / 258 automatically. This makes AutoASPM useful for a much wider range of BCM94360-family cards.
	•	12. Today's v5 broadened Apple-compatible BCM94360 support even further. We removed the hardcoded AutoASPM whitelist of only 0111/0117/0134. Now the rule is simply: if the device is 14e4:43a0 and the subsystem vendor is Apple 106b, preserve platform ASPM regardless of the subsystem-device ID. AutoNSS2 remains restricted to 106b:0117. The v5 script explicitly verifies that the old three-device AutoASPM whitelist is gone.
	•	13. Unknown Broadcom hardware remains untouched. We deliberately didn't start guessing about 43a3, 43ba, BCM43602, etc. If the device ID isn't 43a0, our AutoASPM addition leaves AirportBrcmFixup's original behavior alone. Current v5 therefore has a narrow BCM4360 policy rather than becoming a generic Broadcom power-management hack.
	•	14. Kept NSS correction and ASPM correction independent. This is an important design decision. A card can receive the appropriate automatic ASPM policy without receiving the NSS correction. That's what lets us safely support Apple 3×3 BCM94360 variants for ASPM while restricting the 1 → 3 TX-chain change to the known 2×2 identity.
	•	15. Made the modifications Release-friendly. DEBUG logging such as DBGLOG can disappear from the optimized Release binary, but the actual NSS callback and AutoASPM decisions are compiled into the Release kext. The _wlc_stf_txchain_set route has an error path that remains available even in Release. The current script validates all of those pieces before modifying the source.
	•	16. Built safety into the source-patching scripts. The scripts verify expected source anchors, support a --check dry run, validate that all required code markers exist before writing anything, create backups, and refuse partial modifications. v5 additionally understands an already-v4-patched source tree and upgrades only the AutoASPM portion instead of forcing us to reconstruct the NSS work.
So the current v5 behavior boils down nicely to:


<img width="468" height="543" alt="image" src="https://github.com/user-attachments/assets/a8b1dddd-dca8-4aac-bf7f-b66920eabe3b" />


The two substantial features added to AirportBrcmFixup are therefore AutoNSS2 and AutoASPM. Everything else done has been about making those two features correctly targeted, reversible, Release-capable, and more broadly compatible without blindly changing unrelated Broadcom hardware.


Disclaimer: This project contains AI generated code.I. I have only tested this with a BCM94360NG card. Others may build on this work or incorporate it in any way they would like, just please give me some credit for what has been performed so far. Please feel free to report any bugs you may find. Thank you.



AirportBrcmFixup
==================

[![Build Status](https://github.com/acidanthera/AirportBrcmFixup/actions/workflows/main.yml/badge.svg?branch=master)](https://github.com/acidanthera/AirportBrcmFixup/actions) [![Scan Status](https://scan.coverity.com/projects/16401/badge.svg?flat=1)](https://scan.coverity.com/projects/16401)

An open source kernel extension providing a set of patches required for non-native Airport Broadcom Wi-Fi cards.

#### Notes
Currently this kext requires macOS 10.10 or newer.

Note: ***Debug version of Lilu.kext should be put in the same folder as AirportBrcmFixup! And they should be also used together!***

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
- `-brcmfxbeta` enables loading on unsupported macOS
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

[14+] Use with OCLP
- AirPortBrcm4360: removed
- AirPortBrcm4331: removed
- AirPortBrcmNIC: removed
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

