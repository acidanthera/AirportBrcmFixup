AirportBrcmFixup
==================

An open source kernel extension providing a set of patches required for non-native Airport Broadcom Wi-Fi cards.

#### Notes
This repository should be compiled with [Lilu](https://github.com/vit9696/Lilu) ***v1.1.6*** and greater, otherwise the compilation will be failed!

Note: ***Debug version of Lilu.kext should be put in the same folder as BrcmWLFixup! And they should be also used together!***


#### Features
- Supports AirPort_Brcm4360, AirPort_BrcmNIC and AirPort_BrcmNIC_MFG
- Implements patch for chip identificator checking (0xaa52), in <=10.12 it's required for successful driver starting
- Implements patch for "Failed PCIe configuration patch", required for some cards 
- Implements patch for "Third Party Device" (returns vendor name "Apple")
- Implements patch for removing of white-list check
- Implements 5Ghz and country code patch (US by default)
- Implements WOWL (WoWLAN) disabler (turned on by default) - no low speed after wake

#### Installation types
- Clover Airport Fix + AirportBrcmFixup
- Clover Airport Fix + AddProperties(fake device-id) + AirportBrcmFixup (preferable)
- Clover Airport Fix + AirportBrcmFixup + FakePCIID + FakePCIID_Broadcom_WiFi.kext
- AirportBrcmFixup + FakePCIID + FakePCIID_Broadcom_WiFi.kext

#### Boot-args
- `brcmfx-country=XX` changes the country code to XX (US, CN, #a, ...)
- `-brcmfxdbg` turns on debugging output
- `-brcmfxbeta` enables loading on unsupported osx
- `-brcmfxoff` disables kext loading
- `wl_msg_level=0xXXXX` & `wl_msg_level2=0xXXXX` set log level for native network kexts
- `-brcmfxwowl` enables WOWL (WoWLAN) - it is disabled by default
- option `brcmfx-country` also can be injected via DSDT or AddProperties/Arbitrary in Clover

#### Credits
- [Apple](https://www.apple.com) for macOS  
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/vit9696/Lilu) and great help in implementing some features
- [PMheart](https://github.com/PMheart) for the initial idea and implementation
- [darkvoid & RehabMan] for FakePCIID implementation (partly it is used in AirportBrcmFixup)
- [al3xtjames](https://github.com/al3xtjames) for disabling LPO panic in AirPortBrcmNIC 
- [lvs1974](https://applelife.ru/members/lvs1974.53809/) for writing the software and maintaining it
