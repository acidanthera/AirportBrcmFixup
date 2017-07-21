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


Configuration config;


void Configuration::readArguments() {
}



PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
	config.bootargOff,
	sizeof(config.bootargOff)/sizeof(config.bootargOff[0]),
	config.bootargDebug,
	sizeof(config.bootargDebug)/sizeof(config.bootargDebug[0]),
	config.bootargBeta,
	sizeof(config.bootargBeta)/sizeof(config.bootargBeta[0]),
	KernelVersion::MountainLion,
	KernelVersion::HighSierra,
	[]() {
        config.readArguments();
		brcmfx.init();
	}
};





