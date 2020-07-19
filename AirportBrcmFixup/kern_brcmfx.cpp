//
//  kern_brcmfx.cpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>

#include "kern_config.hpp"
#include "kern_brcmfx.hpp"
#include "kern_fakebrcm.hpp"

#include <IOKit/IOCatalogue.h>

#define kCFBundleIdentifierKey                  "CFBundleIdentifier"
#define KIOClass                  				"IOClass"

// Only used in apple-driven callbacks
static BRCMFX *callbackBRCMFX {nullptr};

static KernelPatcher::KextInfo kextList[kextListSize] {
	{ idList[0], &binList[0], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
	{ idList[1], &binList[1], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
	{ idList[2], &binList[2], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
	{ idList[3], &binList[3], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded }
};

// progress
static bool kext_handled[kextListSize] {};

//==============================================================================

bool BRCMFX::init()
{
	callbackBRCMFX = this;
	
	lilu.onPatcherLoadForce(
		[](void *user, KernelPatcher &patcher) {
			callbackBRCMFX->processKernel(patcher);
		}, this);

	lilu.onKextLoadForce(kextList, kextListSize,
		[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
			callbackBRCMFX->processKext(patcher, index, address, size);
		}, this);


	return true;
}

//==============================================================================

void BRCMFX::deinit()
{
}

//==============================================================================

template <size_t index>
bool BRCMFX::checkBoardId(void *that, const char *boardID)
{
	if (index == AirPort_Brcm4360 && callbackBRCMFX && callbackBRCMFX->cpmChanSwitchWhitelist)
	{
		const char **cpmChanSwitchWhitelist = callbackBRCMFX->cpmChanSwitchWhitelist;
		while (*cpmChanSwitchWhitelist)
		{
			if (boardID == *cpmChanSwitchWhitelist)
			{
				DBGLOG("BRCMFX", "checkBoardId is called with boardID from cpmChanSwitchWhitelist: %s", boardID);
				return false;
			}
			cpmChanSwitchWhitelist++;
		}
	}
	return true;
}

//==============================================================================

const OSSymbol* BRCMFX::newVendorString(void)
{
	DBGLOG("BRCMFX", "newVendorString is called");
	return OSSymbol::withCString("Apple");
}

//==============================================================================

bool  BRCMFX::wlc_wowl_enable(int64_t **a1)
{
	DBGLOG("BRCMFX", "wlc_wowl_enable is called, change returned value to false");
	return false;
}

//==============================================================================

bool BRCMFX::wowCapablePlatform(void *that)
{
	DBGLOG("BRCMFX", "wowCapablePlatform is called, change returned value to false");
	return false;
}

//==============================================================================
// 23 61 means '#' 'a',  "#a" is universal for all countries
// 55 53 -> US
// 43 4e -> CN

//==============================================================================

template <size_t index>
int64_t BRCMFX::wlc_set_countrycode_rev(int64_t a1, const char *country_code, int a3)
{
	DBGLOG("BRCMFX", "wlc_set_countrycode_rev%ld is called, a3 = %d, country_code = %s", index, a3, country_code);

	const char *new_country_code = ADDPR(brcmfx_config).country_code;
	if (!ADDPR(brcmfx_config).country_code_overrided && strlen(callbackBRCMFX->provider_country_code))
	{
		new_country_code = callbackBRCMFX->provider_country_code;
		DBGLOG("BRCMFX", "country code is overrided in ioreg");
	}

	int64_t result = FunctionCast(wlc_set_countrycode_rev<index>, callbackBRCMFX->orgWlcSetCountryCodeRev[index])(a1, new_country_code, -1);
	DBGLOG("BRCMFX", "country code is changed from %s to %s, result = %lld", country_code, new_country_code, result);
	IOSleep(100);
	
	return result;
}

//==============================================================================

int64_t BRCMFX::wlc_set_countrycode_rev_4331(int64_t a1, int64_t a2, const char *country_code, int a4)
{
	int index = AirPort_Brcm4331;
	DBGLOG("BRCMFX", "wlc_set_countrycode_rev_4331 is called, a4 = %d, country_code = %s", a4, country_code);
	
	const char *new_country_code = ADDPR(brcmfx_config).country_code;
	if (!ADDPR(brcmfx_config).country_code_overrided && strlen(callbackBRCMFX->provider_country_code))
	{
		new_country_code = callbackBRCMFX->provider_country_code;
		DBGLOG("BRCMFX", "country code is overrided in ioreg");
	}
	
	int64_t result = FunctionCast(wlc_set_countrycode_rev_4331, callbackBRCMFX->orgWlcSetCountryCodeRev[index])(a1, a2, new_country_code, -1);
	DBGLOG("BRCMFX", "country code is changed from %s to %s, result = %lld", country_code, new_country_code, result);
	IOSleep(100);
	
	return result;
}


//_si_pmu_fvco_pllreg (10.11 - 10.13) compares value stored at [rdi+0x3c] with 0xaa52. This value is a chip identificator: 43602, details:
// https://chromium.googlesource.com/chromiumos/third_party/kernel/+/chromeos-3.18/drivers/net/wireless/bcmdhd/include/bcmdevs.h#396
// In 10.12 and earlier _si_pmu_fvco_pllreg will fail if the value stored at [rdi+0x3c] is not 0xaa52, driver won't be loaded

template <size_t index>
int32_t BRCMFX::siPmuFvcoPllreg(uint32_t *a1, int64_t a2, int64_t a3)
{
	uint32_t original = a1[15];
	a1[15] = 0xaa52;
	
	DBGLOG("BRCMFX", "siPmuFvcoPllreg%ld, original chip identificator = %04x", index, original);
	auto ret = FunctionCast(siPmuFvcoPllreg<index>, callbackBRCMFX->orgSiPmuFvcoPllreg[index])(a1, a2, a3);
	
	a1[15] = original;
	
	return ret;
}

//==============================================================================

bool BRCMFX::start(IOService* service, IOService* provider)
{
	DBGLOG("BRCMFX", "start is called, service name is %s, provider name is %s", service->getName(), provider->getName());
	
	int index = find_service_index(service->getName());
	int brcmfx_driver = ADDPR(brcmfx_config).brcmfx_driver;
	if (brcmfx_driver == -1 && WIOKit::getOSDataValue(provider, Configuration::bootargBrcmDriver, brcmfx_driver)) {
		DBGLOG("BRCMFX", "%s in ioreg is set to %d", Configuration::bootargBrcmDriver, brcmfx_driver);
	} else {
		DBGLOG("BRCMFX", "%s in boot-arg is set to %d", Configuration::bootargBrcmDriver, brcmfx_driver);
	}
	brcmfx_driver = checkBrcmfxDriverValue(brcmfx_driver);
		
	bool disable_driver = (brcmfx_driver == -1 && index == AirPort_BrcmNIC_MFG) || (brcmfx_driver != -1 && brcmfx_driver != index);
	if (index < 0 || disable_driver)
	{
		DBGLOG("BRCMFX", "start: disable service %s", service->getName());
		return nullptr;
	}
	
	auto data = OSDynamicCast(OSData, provider->getProperty(Configuration::bootargBrcmCountry));
	if (data)
	{
		lilu_os_strncpy(callbackBRCMFX->provider_country_code, reinterpret_cast<const char*>(data->getBytesNoCopy()), data->getLength());
		DBGLOG("BRCMFX", "%s in ioreg is set to %s", Configuration::bootargBrcmCountry, callbackBRCMFX->provider_country_code);
	}
	
	auto name = provider->getName();
	
	// There could be only one ARPT
	if (!name || strcmp(name, "ARPT") != 0)
		WIOKit::renameDevice(provider, "ARPT");
	
	PCIHookManager::hookProvider(provider);

	bool result = FunctionCast(start, callbackBRCMFX->orgStart[index])(service, provider);
	DBGLOG("BRCMFX", "start is finished with result %d", result);
	return result;
}

//==============================================================================

IOService* BRCMFX::probe(IOService *service, IOService * provider, SInt32 *score)
{
	DBGLOG("BRCMFX", "probe is called, service name is %s, provider name is %s", service->getName(), provider->getName());
	int index = find_service_index(service->getName());
	int brcmfx_driver = ADDPR(brcmfx_config).brcmfx_driver;
	if (brcmfx_driver == -1 && WIOKit::getOSDataValue(provider, Configuration::bootargBrcmDriver, brcmfx_driver)) {
		DBGLOG("BRCMFX", "%s in ioreg is set to %d", Configuration::bootargBrcmDriver, brcmfx_driver);
    } else {
		DBGLOG("BRCMFX", "%s in boot-arg is set to %d", Configuration::bootargBrcmDriver, brcmfx_driver);
	}
	brcmfx_driver = checkBrcmfxDriverValue(brcmfx_driver);
	
	bool disable_driver = (brcmfx_driver == -1 && index == AirPort_BrcmNIC_MFG) || (brcmfx_driver != -1 && brcmfx_driver != index);
	if (index < 0 || disable_driver)
	{
		DBGLOG("BRCMFX", "probe: disable service %s", service->getName());
		return nullptr;
	}
	
	PCIHookManager::hookProvider(provider);

	IOService *result = FunctionCast(probe, callbackBRCMFX->orgProbe[index])(service, provider, score);
	DBGLOG("BRCMFX", "probe is finished with result %s", (result != nullptr) ? "success" : "failed");
	return result;
}

//==============================================================================

void BRCMFX::osl_panic(const char *format, ...)
{
	if (!strcmp(format, "32KHz LPO Clock not running"))
	{
		// Ignore LPO panic!
		return;
	}
	
	char buf[0x800];
	memset(buf, 0, sizeof(buf));
	
	va_list va;
	va_start(va, format);
	vsnprintf(buf, sizeof(buf), format, va);
	va_end(va);
	
	panic("\"%s\"@/BuildRoot/Library/Caches/com.apple.xbs/Sources/AirPortDriverBrcmNIC/AirPortDriverBrcmNIC-1241.31.1.9/src/shared/macosx_osl.cpp:2029", buf);
}

//==============================================================================
//
// Find service by name in a specified registry plane (gIO80211FamilyPlane or gIOServicePlane)
//

IOService* LIBKERN_RETURNS_NOT_RETAINED findService(const IORegistryPlane* plane, const char *service_name)
{
	IOService            * service = nullptr;
	IORegistryIterator   * iterator = IORegistryIterator::iterateOver(plane, kIORegistryIterateRecursively);

	if (iterator)
	{
		size_t len = strlen(service_name);
		
		IORegistryEntry *res {nullptr};
		while ((res = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) != nullptr)
		{
			const char *resname = res->getName();
			if (resname && !strncmp(service_name, resname, len))
			{
				service = OSDynamicCast(IOService, res);
				if (service != nullptr)
					break;
			}
		}
		
		iterator->release();
	}

	return service;
}

//==============================================================================

void BRCMFX::processKernel(KernelPatcher &patcher)
{
	if (!startMatching_symbol && !startMatching_dictionary)
	{
		startMatching_symbol = reinterpret_cast<IOCatalogue_startMatching_symbol>(patcher.solveSymbol(KernelPatcher::KernelID, "__ZN11IOCatalogue13startMatchingEPK8OSSymbol"));
		if (!startMatching_symbol)
			startMatching_dictionary = reinterpret_cast<IOCatalogue_startMatching_dictionary>(patcher.solveSymbol(KernelPatcher::KernelID, "__ZN11IOCatalogue13startMatchingEP12OSDictionary"));
		if (!startMatching_symbol && !startMatching_dictionary)
			SYSLOG("BRCMFX", "Fail to resolve IOCatalogue::startMatching method, error = %d", patcher.getError());
	}
	
	if (!removeDrivers)
	{
		removeDrivers = reinterpret_cast<IOCatalogue_removeDrivers>(patcher.solveSymbol(KernelPatcher::KernelID, "__ZN11IOCatalogue13removeDriversEP12OSDictionaryb"));
		if (!removeDrivers)
			SYSLOG("BRCMFX", "Fail to resolve IOCatalogue::removeDrivers method, error = %d", patcher.getError());
	}
	
	// Ignore all the errors for other processors
	patcher.clearError();
}

//==============================================================================

void BRCMFX::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	static const mach_vm_address_t wlc_set_countrycode_rev[kextListSize] {
		reinterpret_cast<mach_vm_address_t>(BRCMFX::wlc_set_countrycode_rev<0>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::wlc_set_countrycode_rev<1>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::wlc_set_countrycode_rev<2>),
		reinterpret_cast<mach_vm_address_t>(wlc_set_countrycode_rev_4331)
	};
	
	static const mach_vm_address_t siPmuFvcoPllreg[kextListSize] {
		reinterpret_cast<mach_vm_address_t>(BRCMFX::siPmuFvcoPllreg<0>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::siPmuFvcoPllreg<1>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::siPmuFvcoPllreg<2>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::siPmuFvcoPllreg<3>)
	};
	
	static const mach_vm_address_t checkBoardId[kextListSize] {
		reinterpret_cast<mach_vm_address_t>(BRCMFX::checkBoardId<0>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::checkBoardId<1>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::checkBoardId<2>),
		reinterpret_cast<mach_vm_address_t>(BRCMFX::checkBoardId<3>)
	};
	
	for (size_t i = 0; i < kextListSize; i++)
	{
		if (kextList[i].loadIndex == index && !kext_handled[i])
		{
			kext_handled[i] = true;

			while (true)
			{
				DBGLOG("BRCMFX", "found %s", idList[i]);

				// IOServicePlane should keep a pointer to broadcom driver only if it was successfully started
				IOService* running_service = findService(gIOServicePlane, serviceNameList[i]);
				if (running_service != nullptr)
				{
					SYSLOG("BRCMFX", "%s driver is already loaded, too late to do patching", serviceNameList[i]);
					break;
				}
				
				KernelPatcher::RouteRequest requests[] {
					// Failed PCIe configuration (device-id checking)
					{symbolList[i][0], start, orgStart[i]},
					// Hook probe method
					{symbolList[i][1], probe, orgProbe[i]},
					// Chip identificator checking patch
					{symbolList[i][2], siPmuFvcoPllreg[i], orgSiPmuFvcoPllreg[i]},
					// Wi-Fi 5 Ghz/Country code patch (required for 10.11)
					{symbolList[i][3], wlc_set_countrycode_rev[i], orgWlcSetCountryCodeRev[i]},
					// Third party device patch
					{symbolList[i][4], newVendorString},
					// White list restriction patch
					{symbolList[i][5], checkBoardId[i]},
					// Disable "32KHz LPO Clock not running" panic in AirPort_BrcmXXX
					{symbolList[i][6], osl_panic}
				};

				if (!patcher.routeMultiple(index, requests, address, size))
					SYSLOG("BRCMFX", "at least one basic patch is failed, error = %d", patcher.getError());
				else
					DBGLOG("BRCMFX", "all patches are successfuly applied to %s", idList[i]);
				
				if ((ADDPR(brcmfx_config).brcmfx_driver == -1 && i == AirPort_BrcmNIC_MFG) ||
					(ADDPR(brcmfx_config).brcmfx_driver != -1 && ADDPR(brcmfx_config).brcmfx_driver != i))
					break;
				
				if (cpmChanSwitchWhitelist == nullptr && i == AirPort_Brcm4360)
				{
					cpmChanSwitchWhitelist = reinterpret_cast<const char**>(patcher.solveSymbol(index, "__cpmChanSwitchWhitelist"));
					if (cpmChanSwitchWhitelist != nullptr)
						DBGLOG("BRCMFX", "symbol __cpmChanSwitchWhitelist successfuly solved");
				}

				// Disable WOWL (WoWLAN)
				if (!ADDPR(brcmfx_config).enable_wowl)
				{
					patcher.clearError();
					KernelPatcher::RouteRequest requests[] {
						{symbolList[i][7], wowCapablePlatform},
						{symbolList[i][8], wlc_wowl_enable}
					};
					if (!patcher.routeMultiple(index, requests, address, size))
						SYSLOG("BRCMFX", "wowl disable patch is failed, error = %d", patcher.getError());
					else
						DBGLOG("BRCMFX", "wowl disable patch is successfuly applied to %s", idList[i]);
				}
				break;
			}
			
			ADDPR(brcmfx_config).disabled = true;
			
			IOService *service = findService(gIOServicePlane, "FakeBrcm");
			if (service && service->getProvider() != nullptr)
			{
				auto bundle  = OSDynamicCast(OSString, service->getProperty(kCFBundleIdentifierKey));
				auto ioclass = OSDynamicCast(OSString, service->getProperty(KIOClass));
				bool success = false;

				IOService *provider = service->getProvider();
				if (provider != nullptr)
				{
					provider->retain();
					if (service->terminate()) {
						service->stop(provider);
						success = true;
					}
					
					DBGLOG("BRCMFX", "terminating FakeBrcm with status %d", success);
					
					if (provider->isOpen(service))
					{
						provider->close(service);
						DBGLOG("BRCMFX", "FakeBrcm was closed");
					}
					
					provider->release();
				}
				else
					success = true;
				
				if (success && bundle && ioclass && removeDrivers)
				{
					OSDictionary * dict = OSDictionary::withCapacity(2);
					if (dict) {
						dict->setObject(kCFBundleIdentifierKey, bundle);
						dict->setObject(KIOClass, ioclass);
						if (!removeDrivers(gIOCatalogue, dict, true))
							SYSLOG("BRCMFX", "gIOCatalogue->removeDrivers failed");
						else
							DBGLOG("BRCMFX", "gIOCatalogue->removeDrivers successful");
						OSSafeReleaseNULL(dict);
					}
				}
			}
			else if (startMatching_symbol)
			{
				for (const char* bundle_identifier : idList)
				{
					if (bundle_identifier == idList[i]) continue;
					auto bundle = OSSymbol::withCStringNoCopy(bundle_identifier);
					if (bundle) {
						if (!startMatching_symbol(gIOCatalogue, bundle))
							SYSLOG("BRCMFX", "gIOCatalogue->startMatching(OSSymbol const*) failed");
						else
							DBGLOG("BRCMFX", "gIOCatalogue->startMatching(OSSymbol const*) successful");
						OSSafeReleaseNULL(bundle);
					}
				}
			}
			else if (startMatching_dictionary)
			{
				OSDictionary* dict = OSDictionary::withCapacity(1);
				if (dict) {
					const OSSymbol* pci = OSSymbol::withCStringNoCopy("IOPCIDevice");
					if (pci) {
						dict->setObject("IOProviderClass", pci);
						OSSafeReleaseNULL(pci);
						if (!startMatching_dictionary(gIOCatalogue, dict))
							SYSLOG("BRCMFX", "gIOCatalogue->startMatching(OSDictionary *) failed");
						else
							DBGLOG("BRCMFX", "gIOCatalogue->startMatching(OSDictionary *) successful");
					}
					OSSafeReleaseNULL(dict);
				}
			}
		}
	}

	// Ignore all the errors for other processors
	patcher.clearError();
}
