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

#### Credits
- [Apple](https://www.apple.com) for macOS  
- [vit9696](https://github.com/vit9696) for [Lilu.kext](https://github.com/vit9696/Lilu) and great help in implementing some features
- [PMheart](https://github.com/PMheart) for the initial idea and implementation
- [lvs1974](https://applelife.ru/members/lvs1974.53809/) for writing the software and maintaining it
