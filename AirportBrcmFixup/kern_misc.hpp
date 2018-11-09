//
//  kern_misc.hpp
//  AirportBrcmFixup
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

static const char *symbolList[][8] {
	{"__ZN19AirPort_BrcmNIC_MFG5startEP9IOService",   "__ZN19AirPort_BrcmNIC_MFG5probeEP9IOServicePi", "_si_pmu_fvco_pllreg", "_wlc_set_countrycode_rev",
	 "__ZNK19AirPort_BrcmNIC_MFG15newVendorStringEv", "__ZN19AirPort_BrcmNIC_MFG12checkBoardIdEPKc",   "_wlc_wowl_enable",     nullptr },
	
	{"__ZN16AirPort_Brcm43605startEP9IOService",      "__ZN16AirPort_Brcm43605probeEP9IOServicePi" ,   "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",
	 "__ZNK16AirPort_Brcm436015newVendorStringEv",    "__ZN16AirPort_Brcm436012checkBoardIdEPKc",      "_wlc_wowl_enable",     nullptr },
	
	{"__ZN15AirPort_BrcmNIC5startEP9IOService",       "__ZN15AirPort_BrcmNIC5probeEP9IOServicePi",     "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",
	 "__ZNK15AirPort_BrcmNIC15newVendorStringEv",     "__ZN15AirPort_BrcmNIC12checkBoardIdEPKc" ,      "_wlc_wowl_enable",     "_osl_panic" }
};

inline int find_service_index(const char* service_name)
{
	for (int i=0; i<kextListSize; ++i)
		if (!strcmp(serviceNameList[i], service_name))
			return i;
	return -1;
}

#endif /* kern_misc_hpp */
