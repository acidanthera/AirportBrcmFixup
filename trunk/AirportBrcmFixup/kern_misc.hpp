//
//  kern_misc.hpp
//  HBFX
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#ifndef kern_misc_hpp
#define kern_misc_hpp

static const size_t kextListSize {3};

static const char *serviceNameList[] {
    "AirPort_Brcm4360",
    "AirPort_BrcmNIC",
    "AirPort_BrcmNIC_MFG"
};


static const char *idList[] {
    "com.apple.driver.AirPort.Brcm4360",
    "com.apple.driver.AirPort.BrcmNIC",
    "com.apple.driver.AirPort.BrcmNIC-MFG"
};

static const char *binList[] {
    "/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcm4360.kext/Contents/MacOS/AirPortBrcm4360",
    "/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcmNIC.kext/Contents/MacOS/AirPortBrcmNIC",
    "/System/Library/Extensions/AirPortBrcmNIC-MFG.kext/Contents/MacOS/AirPortBrcmNIC-MFG"
};

// 4360: __ZN16AirPort_Brcm436014getPCIPropertyEP9IOServicePKcRj
// NIC:  __ZN15AirPort_BrcmNIC14getPCIPropertyEP9IOServicePKcR
// MFG:  __ZN19AirPort_BrcmNIC_MFG14getPCIPropertyEP9IOServicePKcRj

static const char *symbolList[][5] {
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK16AirPort_Brcm436015newVendorStringEv",
        "__ZN16AirPort_Brcm436012checkBoardIdEPKc",       "__ZN16AirPort_Brcm43605startEP9IOService"    },
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK15AirPort_BrcmNIC15newVendorStringEv",
        "__ZN15AirPort_BrcmNIC12checkBoardIdEPKc" ,       "__ZN15AirPort_BrcmNIC5startEP9IOService"     },
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK19AirPort_BrcmNIC_MFG15newVendorStringEv",
        "__ZN19AirPort_BrcmNIC_MFG12checkBoardIdEPKc",    "__ZN19AirPort_BrcmNIC_MFG5startEP9IOService" }
};


#endif /* kern_misc_hpp */
