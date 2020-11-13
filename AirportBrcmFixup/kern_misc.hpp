//
//  kern_misc.hpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#ifndef kern_misc_hpp
#define kern_misc_hpp

#include <Headers/kern_util.hpp>
#include "kern_config.hpp"

#define kCFBundleIdentifierKey					"CFBundleIdentifier"
#define kCFBundleIdentifierKernelKey			"CFBundleIdentifierKernel"
#define KIOClass                  				"IOClass"

enum AirportServices {
	AirPort_BrcmNIC_MFG = 0,
	AirPort_Brcm4360,
	AirPort_BrcmNIC,
	AirPort_Brcm4331,
	MaxServices
};

static const char *serviceNameList[MaxServices] {
	"AirPort_BrcmNIC_MFG",
	"AirPort_Brcm4360",
	"AirPort_BrcmNIC",
	"AirPort_Brcm4331"
};

static const char *idList[MaxServices] {
	"com.apple.driver.AirPort.BrcmNIC-MFG",
	"com.apple.driver.AirPort.Brcm4360",
	"com.apple.driver.AirPort.BrcmNIC",
	"com.apple.driver.AirPort.Brcm4331"
};

static const char *binList[MaxServices] {
	"/System/Library/Extensions/AirPortBrcmNIC-MFG.kext/Contents/MacOS/AirPortBrcmNIC-MFG",
	"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcm4360.kext/Contents/MacOS/AirPortBrcm4360",
	"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcmNIC.kext/Contents/MacOS/AirPortBrcmNIC",
	"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcm4331.kext/Contents/MacOS/AirPortBrcm4331"
};

static const char *symbolList[MaxServices][9] {
	{"__ZN19AirPort_BrcmNIC_MFG5startEP9IOService",     "__ZN19AirPort_BrcmNIC_MFG5probeEP9IOServicePi", "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",
	 "__ZNK19AirPort_BrcmNIC_MFG15newVendorStringEv",   "__ZN19AirPort_BrcmNIC_MFG12checkBoardIdEPKc",   nullptr,
	 "__ZN19AirPort_BrcmNIC_MFG18wowCapablePlatformEv", "_wlc_wowl_enable" },
	
	{"__ZN16AirPort_Brcm43605startEP9IOService",        "__ZN16AirPort_Brcm43605probeEP9IOServicePi" ,   "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",
	 "__ZNK16AirPort_Brcm436015newVendorStringEv",      "__ZN16AirPort_Brcm436012checkBoardIdEPKc",      nullptr,
	 "__ZN16AirPort_Brcm436018wowCapablePlatformEv",    "_wlc_wowl_enable" },
	
	{"__ZN15AirPort_BrcmNIC5startEP9IOService",         "__ZN15AirPort_BrcmNIC5probeEP9IOServicePi",     "_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",
	 "__ZNK15AirPort_BrcmNIC15newVendorStringEv",       "__ZN15AirPort_BrcmNIC12checkBoardIdEPKc",       "_osl_panic",
	 "__ZN15AirPort_BrcmNIC18wowCapablePlatformEv",     "_wlc_wowl_enable" },
	
	{"__ZN16AirPort_Brcm43315startEP9IOService",        "__ZN16AirPort_Brcm43315probeEP9IOServicePi",    nullptr,                "_wlc_set_countrycode_rev",
	 "__ZNK16AirPort_Brcm433115newVendorStringEv",       nullptr,                                        nullptr,
	 "__ZN16AirPort_Brcm433118wowCapablePlatformEv",    "_wlc_wowl_enable"   }
};

/**
* find service internal index by name
*/

inline int find_service_index(const char* service_name)
{
	for (int i=0; i<MaxServices; ++i)
		if (!strcmp(serviceNameList[i], service_name))
			return i;
	return -1;
}

/**
 * check and fix brcmfx_driver agrument, return -1 if it cannot be applied
 */

inline int checkBrcmfxDriverValue(int brcmfx_driver, bool skip_log = false)
{
	if (brcmfx_driver != -1 && !ADDPR(brcmfx_config).enable_all_drv)
	{
		if (getKernelVersion() <= KernelVersion::Sierra)
		{
			if (brcmfx_driver == AirPort_BrcmNIC_MFG || brcmfx_driver == AirPort_BrcmNIC) {
				if (!skip_log) {
					DBGLOG("BRCMFX", "brcmfx-driver %d is not supported in osx Sierra and earlier", brcmfx_driver);
				}
				return -1;
			}
		}
		else if (getKernelVersion() >= KernelVersion::Catalina)
		{
			if (brcmfx_driver == AirPort_BrcmNIC_MFG || brcmfx_driver == AirPort_Brcm4331) {
				if (!skip_log) {
					DBGLOG("BRCMFX", "brcmfx-driver %d is not supported in osx Catalina and over", brcmfx_driver);
				}
				return -1;
			}
			if (getKernelVersion() >= KernelVersion::BigSur)
			{
				if (brcmfx_driver == AirPort_Brcm4360) {
					if (!skip_log) {
						DBGLOG("BRCMFX", "brcmfx-driver %d is not supported in osx Big Sur and over", brcmfx_driver);
					}
					return -1;
				}
			}
		}
	}
	
	return brcmfx_driver;
}

#endif /* kern_misc_hpp */
