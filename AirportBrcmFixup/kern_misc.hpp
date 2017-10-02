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

static const char *symbolList[][6] {
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK16AirPort_Brcm436015newVendorStringEv",
        "__ZN16AirPort_Brcm436012checkBoardIdEPKc",       "__ZN16AirPort_Brcm43605startEP9IOService",      "_wlc_wowl_enable" },
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK15AirPort_BrcmNIC15newVendorStringEv",
        "__ZN15AirPort_BrcmNIC12checkBoardIdEPKc" ,       "__ZN15AirPort_BrcmNIC5startEP9IOService",       "_wlc_wowl_enable" },
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK19AirPort_BrcmNIC_MFG15newVendorStringEv",
        "__ZN19AirPort_BrcmNIC_MFG12checkBoardIdEPKc",    "__ZN19AirPort_BrcmNIC_MFG5startEP9IOService",   "_wlc_wowl_enable" }
};


struct VMTOffset {
    enum {
        configRead16  = 0x860/sizeof(uintptr_t),
        configWrite16 = 0x868/sizeof(uintptr_t),
        configRead32  = 0x850/sizeof(uintptr_t),
        configWrite32 = 0x858/sizeof(uintptr_t),
        configRead8   = 0x870/sizeof(uintptr_t),
        configWrite8  = 0x878/sizeof(uintptr_t),
    };
};

/* Definitions of PCI Config Registers */
enum {
    kIOPCIConfigVendorID                = 0x00,
    kIOPCIConfigDeviceID                = 0x02,
    kIOPCIConfigCommand                 = 0x04,
    kIOPCIConfigStatus                  = 0x06,
    kIOPCIConfigRevisionID              = 0x08,
    kIOPCIConfigClassCode               = 0x09,
    kIOPCIConfigCacheLineSize           = 0x0C,
    kIOPCIConfigLatencyTimer            = 0x0D,
    kIOPCIConfigHeaderType              = 0x0E,
    kIOPCIConfigBIST                    = 0x0F,
    kIOPCIConfigBaseAddress0            = 0x10,
    kIOPCIConfigBaseAddress1            = 0x14,
    kIOPCIConfigBaseAddress2            = 0x18,
    kIOPCIConfigBaseAddress3            = 0x1C,
    kIOPCIConfigBaseAddress4            = 0x20,
    kIOPCIConfigBaseAddress5            = 0x24,
    kIOPCIConfigCardBusCISPtr           = 0x28,
    kIOPCIConfigSubSystemVendorID       = 0x2C,
    kIOPCIConfigSubSystemID             = 0x2E,
    kIOPCIConfigExpansionROMBase        = 0x30,
    kIOPCIConfigCapabilitiesPtr         = 0x34,
    kIOPCIConfigInterruptLine           = 0x3C,
    kIOPCIConfigInterruptPin            = 0x3D,
    kIOPCIConfigMinimumGrant            = 0x3E,
    kIOPCIConfigMaximumLatency          = 0x3F
};

#endif /* kern_misc_hpp */
