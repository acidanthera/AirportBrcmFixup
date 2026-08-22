//
//  kern_start.cpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>
#include <IOKit/pci/IOPCIDevice.h>

#include "kern_config.hpp"
#include "kern_brcmfx.hpp"

static BRCMFX brcmfx;

Configuration ADDPR(brcmfx_config);

const char *bootargOff[] {
	ADDPR(brcmfx_config).bootargOff
};

const char *bootargDebug[] {
	ADDPR(brcmfx_config).bootargDebug
};

const char *bootargBeta[] {
	ADDPR(brcmfx_config).bootargBeta
};


bool Configuration::awaitPublishing(IORegistryEntry *obj)
{
	size_t counter = 0;
	while (counter < 256) {
		if (obj->inPlane(gIOServicePlane)) {
			DBGLOG("BRCMFX", "pci device %s is in service plane %lu", safeString(obj->getName()), counter);
			return true;
		}
		DBGLOG("BRCMFX", "pci device %s is not in service plane %lu, polling", safeString(obj->getName()), counter);
		
		if (obj->getProperty("IOPCIConfigured")) {
			DBGLOG("dev", "pci bridge %s is configured %lu", safeString(obj->getName()), counter);
			break;
		}
		DBGLOG("BRCMFX", "pci bridge %s is not configured %lu, polling", safeString(obj->getName()), counter);
		++counter;
		IOSleep(20);
	}

	SYSLOG("BRCMFX", "found dead pci device %s", safeString(obj->getName()));
	return false;
}

void Configuration::readArguments(IOService* provider)
{
	if (!provider)
	{
		disabled = checkKernelArgument(bootargOff);

		PE_parse_boot_argn(bootargBrcmCountry, country_code, sizeof(country_code));

		PE_parse_boot_argn(bootargBrcmDriver, &brcmfx_driver, sizeof(brcmfx_driver));
		brcmfx_driver = checkBrcmfxDriverValue(brcmfx_driver);

		enable_wowl = checkKernelArgument("-brcmfxwowl") || checkKernelArgument(bootargBrcmEnableWowl);
		enable_all_drv = checkKernelArgument(bootargBrcmAllDrv);

		uint32_t nss1_value = 0;
		if (PE_parse_boot_argn(bootargBrcmNss1, &nss1_value, sizeof(nss1_value)))
			force_nss1 = (nss1_value != 0);
		
		if (PE_parse_boot_argn(bootargBrcmAspm, &brcmfx_aspm, sizeof(brcmfx_aspm)))
			override_aspm = true;
		PE_parse_boot_argn(bootargDelay, &start_delay, sizeof(start_delay));
	}
	else if (!config_is_ready)
	{
		awaitPublishing(provider);

		auto pciDevice = OSDynamicCast(IOPCIDevice, provider);
		if (!pciDevice) {
			DBGLOG("BRCMFX", "Provider is not IOPCIDevice");
			return;
		}
	
		if (PE_parse_boot_argn(bootargBrcmAspm, &brcmfx_aspm, sizeof(brcmfx_aspm))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargBrcmAspm, brcmfx_aspm);
			override_aspm = true;
		} else if (WIOKit::getOSDataValue(provider, bootargBrcmAspm, brcmfx_aspm)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", bootargBrcmAspm, brcmfx_aspm);
			override_aspm = true;
		}
		
		bool userAspmOverride = override_aspm;

		if (PE_parse_boot_argn(bootargDelay, &start_delay, sizeof(start_delay))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargDelay, start_delay);
		} else if (WIOKit::getOSDataValue(provider, bootargDelay, start_delay)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", bootargDelay, start_delay);
		}


		// AutoNSS2/AutoASPM policy.
		uint16_t autoVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigVendorID);
		uint16_t autoDeviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigDeviceID);
		uint16_t autoSubVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemVendorID);
		uint16_t autoSubDeviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemID);

		// Apple BCM94360CS2 / common BCM94360NG identity is 2x2:2.
		enable_nss2 = !force_nss1 &&
			autoVendorID == 0x14e4 && autoDeviceID == 0x43a0 &&
			autoSubVendorID == 0x106b && autoSubDeviceID == 0x0117;

		if (enable_nss2)
			DBGLOG("BRCMFX", "AutoNSS2: enabled for %04x:%04x subsystem %04x:%04x",
				autoVendorID, autoDeviceID, autoSubVendorID, autoSubDeviceID);
		else if (force_nss1)
			DBGLOG("BRCMFX", "AutoNSS2: disabled by brcmfx-nss1=1");

		// Explicit brcmfx-aspm always wins over automatic selection.
		if (!userAspmOverride && autoVendorID == 0x14e4 && autoDeviceID == 0x43a0) {
			if (autoSubVendorID == 0x106b) {
				// Apple-subsystem BCM4360 identity: preserve platform ASPM (0xFF sentinel).
				brcmfx_aspm = 0xFF;
				override_aspm = false;
				DBGLOG("BRCMFX", "AutoASPM: preserving platform ASPM for subsystem %04x:%04x",
					autoSubVendorID, autoSubDeviceID);
			} else {
				// Non-Apple BCM4360 identity: L1 + CLKREQ (2 + 256 = 258).
				brcmfx_aspm = 0x102;
				override_aspm = true;
				DBGLOG("BRCMFX", "AutoASPM: using 258 (L1+CLKREQ) for subsystem %04x:%04x",
					autoSubVendorID, autoSubDeviceID);
			}
		}

		if (brcmfx_aspm != 0xFF)
		{
			uint16_t vendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigVendorID);
			uint16_t deviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigDeviceID);
			uint16_t subSystemVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemVendorID);
			bool     bcm4350  = (vendorID == 0x14e4 && deviceID == 0x43a3 && subSystemVendorID != 0x106b);
			// change APSM flags if value has been forced or for Broadcom BCM4350 chipset
			if (override_aspm || bcm4350 || (getKernelVersion() >= KernelVersion::Monterey))
			{
				override_aspm = true;
				DBGLOG("BRCMFX", "Configuration::readArguments: override aspm, subsystem-vendor-id = 0x%04x, subsystem-id = 0x%04x",
					   subSystemVendorID, pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemID));
				auto pci_aspm_default = OSDynamicCast(OSNumber, provider->getProperty("pci-aspm-default"));
				if (pci_aspm_default == nullptr || pci_aspm_default->unsigned32BitValue() != brcmfx_aspm)
				{
					DBGLOG("BRCMFX", "Configuration::readArguments: pci-aspm-default needs to be set to %d", brcmfx_aspm);
					provider->setProperty("pci-aspm-default", brcmfx_aspm, 32);
				}
			}
		}
		else
			override_aspm = false;

		UInt32 value = 0;
		if ((enable_wowl = checkKernelArgument(bootargBrcmEnableWowl))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargBrcmEnableWowl, enable_wowl);
		}
		else if (WIOKit::getOSDataValue(provider, &bootargBrcmEnableWowl[1], enable_wowl)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", &bootargBrcmEnableWowl[1], enable_wowl);
		}
		else if (WIOKit::getOSDataValue(provider, &bootargBrcmEnableWowl[1], value)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", &bootargBrcmEnableWowl[1], value);
			enable_wowl = (value != 0);
		}

		if ((enable_all_drv = checkKernelArgument(bootargBrcmAllDrv))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargBrcmAllDrv, enable_all_drv);
		}
		else if (WIOKit::getOSDataValue(provider, &bootargBrcmAllDrv[1], enable_all_drv)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", &bootargBrcmAllDrv[1], enable_all_drv);
		}
		else if (WIOKit::getOSDataValue(provider, &bootargBrcmAllDrv[1], value)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", &bootargBrcmAllDrv[1], value);
			enable_all_drv = (value != 0);
		}
		
		if (PE_parse_boot_argn(bootargBrcmDriver, &brcmfx_driver, sizeof(brcmfx_driver))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargBrcmDriver, brcmfx_driver);
		} else if (WIOKit::getOSDataValue(provider, bootargBrcmDriver, brcmfx_driver)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", bootargBrcmDriver, brcmfx_driver);
		}
		brcmfx_driver = checkBrcmfxDriverValue(brcmfx_driver);

		if (PE_parse_boot_argn(bootargBrcmCountry, country_code, sizeof(country_code))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargBrcmCountry, country_code);
		} else {
			auto data = OSDynamicCast(OSData, provider->getProperty(bootargBrcmCountry));
			if (data) {
				lilu_os_strncpy(country_code, reinterpret_cast<const char*>(data->getBytesNoCopy()), data->getLength());
				DBGLOG("BRCMFX", "%s in ioreg is set to %s", bootargBrcmCountry, country_code);
			}
		}

		config_is_ready = true;
	}
}

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff, arrsize(bootargOff),
	bootargDebug, arrsize(bootargDebug),
	bootargBeta, arrsize(bootargBeta),
	KernelVersion::MountainLion,
	KernelVersion::Sequoia,
	[]() {
		ADDPR(brcmfx_config).readArguments();
		brcmfx.init();
	}
};
