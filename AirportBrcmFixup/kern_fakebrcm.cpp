//
//  kern_fakebrcm.cpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/plugin_start.hpp>

#include "kern_config.hpp"
#include "kern_fakebrcm.hpp"
#include "kern_misc.hpp"
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOCatalogue.h>

OSDefineMetaClassAndStructors(FakeBrcm, IOService);


IOService 				  *PCIHookManager::service_provider{nullptr};
WIOKit::t_PCIConfigRead16  PCIHookManager::orgConfigRead16 {nullptr};
WIOKit::t_PCIConfigRead32  PCIHookManager::orgConfigRead32 {nullptr};

bool FakeBrcm::service_found {false};

//==============================================================================

bool PCIHookManager::isServiceSupported(IORegistryEntry* service)
{
	if (service == service_provider)
		return true;
	
	for (int i=0; i<kextListSize; i++)
	{
		if (strcmp(serviceNameList[i], safeString(service->getName())) == 0)
			return true;
	}
	return false;
}

//==============================================================================

UInt16 PCIHookManager::configRead16(IORegistryEntry *service, UInt32 space, UInt8 offset)
{
	UInt16 result = orgConfigRead16(service, space, offset);
	UInt16 newResult = result;
	
	if ((offset == WIOKit::PCIRegister::kIOPCIConfigVendorID || offset == WIOKit::PCIRegister::kIOPCIConfigDeviceID) && isServiceSupported(service))
	switch (offset)
	{
		case WIOKit::PCIRegister::kIOPCIConfigVendorID:
		{
			UInt32 vendor;
			if (WIOKit::getOSDataValue(service, "vendor-id", vendor))
				newResult = vendor;
			break;
		}
		case WIOKit::PCIRegister::kIOPCIConfigDeviceID:
		{
			UInt32 device;
			if (WIOKit::getOSDataValue(service, "device-id", device))
				newResult = device;
			break;
		}
	}
	
	if (newResult != result)
		DBGLOG("BRCMFX", "PCIHookManager::configRead16: name = %s, source value = 0x%04x replaced with value = 0x%04x", safeString(service->getName()), result, newResult);
	
	return newResult;
}

//==============================================================================

UInt32 PCIHookManager::configRead32(IORegistryEntry *service, UInt32 space, UInt8 offset)
{
	UInt32 result = orgConfigRead32(service, space, offset);
	UInt32 newResult = result;
	
	// OS X does a non-aligned read, which still returns full vendor / device ID
	if ((offset == WIOKit::PCIRegister::kIOPCIConfigVendorID || offset == WIOKit::PCIRegister::kIOPCIConfigDeviceID) && isServiceSupported(service))
	{
		UInt32 vendor, device;
		if (WIOKit::getOSDataValue(service, "vendor-id", vendor))
			newResult = (newResult & 0xFFFF0000) | vendor;
		if (WIOKit::getOSDataValue(service, "device-id", device))
			newResult = (device << 16) | (newResult & 0xFFFF);
	}
	
	if (newResult != result)
		DBGLOG("BRCMFX", "PCIHookManager::configRead32: name = %s, source value = 0x%08x replaced with value = 0x%08x", safeString(service->getName()), result, newResult);
	
	return newResult;
}

//==============================================================================

void PCIHookManager::hookProvider(IOService *provider)
{
	awaitPublishing(provider);
	
	auto pciDevice = OSDynamicCast(IOPCIDevice, provider);
	if (!pciDevice) {
		DBGLOG("BRCMFX", "Provider is not IOPCIDevice");
		return;
	}
	
	if (service_provider == nullptr)
	{
		bool aspm_value_forced = false;
		IOOptionBits brcmfx_aspm = 0;
		if ((aspm_value_forced = PE_parse_boot_argn(Configuration::bootargBrcmAspm, &brcmfx_aspm, sizeof(brcmfx_aspm)))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", Configuration::bootargBrcmAspm, brcmfx_aspm);
		} else if ((aspm_value_forced = WIOKit::getOSDataValue(provider, Configuration::bootargBrcmAspm, brcmfx_aspm))) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", Configuration::bootargBrcmAspm, brcmfx_aspm);
		}
		
		if (brcmfx_aspm != 0xFF)
		{
			uint16_t vendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigVendorID);
			uint16_t deviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigDeviceID);
			uint16_t subSystemVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemVendorID);
			// change APSM flags if value has been forced or for Broadcom BCM4350 chipset
			if (aspm_value_forced || (vendorID == 0x14e4 && deviceID == 0x43a3 && subSystemVendorID != 0x106b))
			{
				DBGLOG("BRCMFX", "PCIHookManager::hookProvider: Broadcom BCM4350 chipset is detected, subsystem-vendor-id = 0x%04x, subsystem-id = 0x%04x",
					   subSystemVendorID, pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemID));
				auto pci_aspm_default = OSDynamicCast(OSNumber, provider->getProperty("pci-aspm-default"));
				if (pci_aspm_default == nullptr || pci_aspm_default->unsigned32BitValue() != brcmfx_aspm)
				{
					DBGLOG("BRCMFX", "PCIHookManager::hookProvider: pci-aspm-default needs to be set to %d", brcmfx_aspm);
					provider->setProperty("pci-aspm-default", brcmfx_aspm, 32);
				}
				
				auto result = pciDevice->setASPMState(provider, brcmfx_aspm);
				if (result != KERN_SUCCESS)
					SYSLOG("BRCMFX", "setASPMState failed with result %x", result);
			}
		}
		
		UInt32 enable_wowl = 0;
		if (PE_parse_boot_argn(Configuration::bootargBrcmEnableWowl, &enable_wowl, sizeof(enable_wowl))) {
			DBGLOG("BRCMFX", "%s in boot-arg is set to %d", Configuration::bootargBrcmEnableWowl, enable_wowl);
			ADDPR(brcmfx_config).enable_wowl = (enable_wowl != 0);
		}
		else if (WIOKit::getOSDataValue(provider, Configuration::bootargBrcmEnableWowl, enable_wowl)) {
			DBGLOG("BRCMFX", "%s in ioreg is set to %d", Configuration::bootargBrcmEnableWowl, enable_wowl);
			ADDPR(brcmfx_config).enable_wowl = (enable_wowl != 0);
		}
	}

	service_provider = provider;

	UInt32 vendor, device;
	if (WIOKit::getOSDataValue(provider, "device-id", device) && WIOKit::getOSDataValue(provider, "vendor-id", vendor))
	{
		if (WIOKit::readPCIConfigValue(provider, WIOKit::PCIRegister::kIOPCIConfigDeviceID) == device &&
			WIOKit::readPCIConfigValue(provider, WIOKit::PCIRegister::kIOPCIConfigVendorID) == vendor)
		{
			DBGLOG("BRCMFX", "PCIHookManager::hookProvider: hook is not required since vendor-id && device-id are the same");
			return;
		}
	}
	
	if (orgConfigRead16 == nullptr)
		if (KernelPatcher::routeVirtual(provider, WIOKit::PCIConfigOffset::ConfigRead16, configRead16, &orgConfigRead16))
			DBGLOG("BRCMFX", "PCIHookManager::hookProvider for configRead16 was successful");
	
	if (orgConfigRead32 == nullptr)
		if (KernelPatcher::routeVirtual(provider, WIOKit::PCIConfigOffset::ConfigRead32, configRead32, &orgConfigRead32))
			DBGLOG("BRCMFX", "PCIHookManager::hookProvider for configRead32 was successful");
}

//==============================================================================

bool PCIHookManager::awaitPublishing(IORegistryEntry *obj)
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


//==============================================================================

bool FakeBrcm::init(OSDictionary *propTable)
{
	if (ADDPR(brcmfx_config).disabled)
	{
		DBGLOG("BRCMFX", "FakeBrcm::init(): FakeBrcm disabled");
		return false;
	}
	
	DBGLOG("BRCMFX", "FakeBrcm::init()");

	bool ret = super::init(propTable);
	if (!ret)
	{
		SYSLOG("BRCMFX", "FakeBrcm super::init returned false\n");
		return false;
	}
	
	return true;
}

//==============================================================================

bool FakeBrcm::attach(IOService *provider)
{
	PCIHookManager::hookProvider(provider);
	
	return super::attach(provider);
}

//==============================================================================

IOService* FakeBrcm::probe(IOService * provider, SInt32 *score)
{
	DBGLOG("BRCMFX", "FakeBrcm::probe(), version %s", xStringify(MODULE_VERSION));

	if (!ADDPR(startSuccess))
	{
		return nullptr;
	}

	IOService* ret = super::probe(provider, score);
	if (!ret)
	{
		SYSLOG("BRCMFX", "FakeBrcm super::probe returned nullptr\n");
		return nullptr;
	}

	if (ADDPR(brcmfx_config).disabled)
	{
		DBGLOG("BRCMFX", "FakeBrcm::probe(): FakeBrcm disabled");
		return ret;
	}
	
	DBGLOG("BRCMFX", "FakeBrcm::probe(): service provider is %s", safeString(provider->getName()));
	
	for (int i = 0; i < kextListSize; i++)
	{
		int brcmfx_driver = checkBrcmfxDriverValue(i, true);
		if (i != brcmfx_driver)
			continue;

		auto bundle = OSSymbol::withCStringNoCopy(idList[i]);
		if (bundle) {
			OSDictionary * dict = OSDictionary::withCapacity(1);
			if (dict) {
				dict->setObject(kCFBundleIdentifierKey, bundle);
				SInt32 generation = 0;
				OSOrderedSet *set = gIOCatalogue->findDrivers(dict, &generation);
				if (set) {
					if (set->getCount() > 0)
					{
						DBGLOG("BRCMFX", "FakeBrcm::probe(): gIOCatalogue->findDrivers() returned non-empty ordered set for bundle %s", idList[i]);
						service_found = true;
						
					}
					else
						DBGLOG("BRCMFX", "FakeBrcm::probe(): gIOCatalogue->findDrivers() returned empty ordered set for bundle %s", idList[i]);
				}
				else {
					SYSLOG("BRCMFX", "FakeBrcm::probe(): gIOCatalogue->findDrivers() failed for bundle %s", idList[i]);
				}
				OSSafeReleaseNULL(dict);
				OSSafeReleaseNULL(set);
			}
		}
	}
	
	if (service_found)
	{
		SInt32 new_probe_score = 6000;
		DBGLOG("BRCMFX", "FakeBrcm::probe() will change score from %d to %d", *score, new_probe_score);
		*score = new_probe_score;  // change probe score to be the first in the list
	}
	else
	{
		*score = -2000;
		DBGLOG("BRCMFX", "FakeBrcm::probe(): fallback to original driver");
	}

	return ret;
}

//==============================================================================

bool FakeBrcm::start(IOService *provider)
{
	DBGLOG("BRCMFX", "FakeBrcm::start()");

	if (!ADDPR(startSuccess))
	{
		return nullptr;
	}

	if (ADDPR(brcmfx_config).disabled)
	{
		DBGLOG("BRCMFX", "FakeBrcm::start(): FakeBrcm disabled");
		return false;
	}
	
	if (!super::start(provider))
	{
		SYSLOG("BRCMFX", "FakeBrcm super::start returned false\n");
		return false;
	}
	
	if (!service_found)
	{
		SYSLOG("BRCMFX", "FakeBrcm::start(): fallback to original driver");
		return false;
	}
	
	PCIHookManager::hookProvider(provider);

	return true;
}

//==============================================================================

void FakeBrcm::stop(IOService *provider)
{
	DBGLOG("BRCMFX", "FakeBrcm::stop()");
	super::stop(provider);
}

//==============================================================================

void FakeBrcm::free()
{
	DBGLOG("BRCMFX", "FakeBrcm::free()");
	super::free();
}



