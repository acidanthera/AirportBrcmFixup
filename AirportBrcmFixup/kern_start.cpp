//
//  kern_start.cpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_config.hpp"
#include "kern_brcmfx.hpp"

static BRCMFX brcmfx;

const char *Configuration::bootargOff[] {
	"-brcmfxoff"
};

const char *Configuration::bootargDebug[] {
	"-brcmfxdbg"
};

const char *Configuration::bootargBeta[] {
	"-brcmfxbeta"
};

Configuration ADDPR(brcmfx_config);

void Configuration::readArguments() {
	country_code_overrided = PE_parse_boot_argn(ADDPR(brcmfx_config).bootargBrcmCountry, country_code, sizeof(country_code));

	PE_parse_boot_argn(ADDPR(brcmfx_config).bootargWlanMsgLevel, &wl_msg_level, sizeof(wl_msg_level));
	PE_parse_boot_argn(ADDPR(brcmfx_config).bootargWlanMsgLevel2, &wl_msg_level2, sizeof(wl_msg_level2));

	disabled = checkKernelArgument(ADDPR(brcmfx_config).bootargOff[0]);
	enable_wowl = checkKernelArgument(ADDPR(brcmfx_config).bootargBrcmEnableWowl);
}

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	ADDPR(brcmfx_config).bootargOff,
	arrsize(ADDPR(brcmfx_config).bootargOff),
	ADDPR(brcmfx_config).bootargDebug,
	arrsize(ADDPR(brcmfx_config).bootargDebug),
	ADDPR(brcmfx_config).bootargBeta,
	arrsize(ADDPR(brcmfx_config).bootargBeta),
	KernelVersion::MountainLion,
	KernelVersion::Mojave,
	[]() {
		ADDPR(brcmfx_config).readArguments();
		brcmfx.init();
	}
};
