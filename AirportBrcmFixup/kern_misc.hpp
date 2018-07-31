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
    "AirPort_BrcmNIC_MFG",
    "AirPort_Brcm4360",
    "AirPort_BrcmNIC"
};


static const char *idList[] {
    "com.apple.driver.AirPort.BrcmNIC-MFG",
    "com.apple.driver.AirPort.Brcm4360",
    "com.apple.driver.AirPort.BrcmNIC"
};

static const char *binList[] {
    "/System/Library/Extensions/AirPortBrcmNIC-MFG.kext/Contents/MacOS/AirPortBrcmNIC-MFG",
    "/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcm4360.kext/Contents/MacOS/AirPortBrcm4360",
    "/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcmNIC.kext/Contents/MacOS/AirPortBrcmNIC"
};

static const char *symbolList[][6] {
    {"__ZN19AirPort_BrcmNIC_MFG5startEP9IOService", "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK19AirPort_BrcmNIC_MFG15newVendorStringEv",
     "__ZN19AirPort_BrcmNIC_MFG12checkBoardIdEPKc", "_wlc_wowl_enable" },
    {"__ZN16AirPort_Brcm43605startEP9IOService",    "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK16AirPort_Brcm436015newVendorStringEv",
     "__ZN16AirPort_Brcm436012checkBoardIdEPKc",    "_wlc_wowl_enable" },
    {"__ZN15AirPort_BrcmNIC5startEP9IOService",     "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK15AirPort_BrcmNIC15newVendorStringEv",
     "__ZN15AirPort_BrcmNIC12checkBoardIdEPKc" ,    "_wlc_wowl_enable" }
};

#endif /* kern_misc_hpp */
