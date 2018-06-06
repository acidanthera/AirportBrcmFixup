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

const char *Configuration::bootargBrcmCountry    {"brcmfx-country"};     // 5 Ghz patch - change default country
const char *Configuration::bootargWlanMsgLevel   {"wl_msg_level"};       // message level for wlan kexts
const char *Configuration::bootargWlanMsgLevel2  {"wl_msg_level2"};      // message level for wlan kexts
const char *Configuration::bootargBrcmEnableWowl {"-brcmfxwowl"};        // enable WOWL 


Configuration ADDPR(brcmfx_config);


void Configuration::readArguments() {
    
    country_code_overrided = PE_parse_boot_argn(bootargBrcmCountry, country_code, sizeof(country_code));

    
    if (!PE_parse_boot_argn(bootargWlanMsgLevel, &wl_msg_level, sizeof(wl_msg_level)))
        wl_msg_level = 0;
    if (!PE_parse_boot_argn(bootargWlanMsgLevel2, &wl_msg_level2, sizeof(wl_msg_level2)))
        wl_msg_level2 = 0;
    
    char tmp[16];
    disabled = PE_parse_boot_argn(ADDPR(brcmfx_config).bootargOff[0], tmp, sizeof(tmp));
    enable_wowl = PE_parse_boot_argn(bootargBrcmEnableWowl, tmp, sizeof(tmp));
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
	KernelVersion::HighSierra,
	[]() {
        ADDPR(brcmfx_config).readArguments();
		brcmfx.init();
	}
};





