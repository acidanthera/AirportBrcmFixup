//
//  kern_brcmfx.cpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>

#include "kern_config.hpp"
#include "kern_brcmfx.hpp"
#include "kern_misc.hpp"

#include <IOKit/IOCatalogue.h>

#define kCFBundleIdentifierKey                  "CFBundleIdentifier"
#define KIOClass                  				"IOClass"

// Only used in apple-driven callbacks
static BRCMFX *callbackBRCMFX {nullptr};

static KernelPatcher::KextInfo kextList[kextListSize] {
	{ idList[0], &binList[0], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
	{ idList[1], &binList[1], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
	{ idList[2], &binList[2], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded }
};

// progress
static bool kext_handled[kextListSize] {};

//==============================================================================

bool BRCMFX::init()
{
	lilu.onKextLoadForce(kextList, kextListSize,
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		callbackBRCMFX = static_cast<BRCMFX *>(user);
		callbackBRCMFX->processKext(patcher, index, address, size);
	}, this);


	return true;
}

//==============================================================================

void BRCMFX::deinit()
{
}

//==============================================================================

bool BRCMFX::checkBoardId(const char *boardID)
{
	DBGLOG("BRCMFX", "checkBoardId is called");
	return true;
}

//==============================================================================

const OSSymbol* BRCMFX::newVendorString(void)
{
	DBGLOG("BRCMFX", "newVendorString is called");
	return OSSymbol::withCString("Apple");
}

//==============================================================================

int32_t BRCMFX::siPmuFvcoPllreg(uint32_t *a1, int64_t a2, int64_t a3)
{
	//_si_pmu_fvco_pllreg (10.11 - 10.13) compares value stored at [rdi+0x3c] with 0xaa52. This value is a chip identificator: 43602, details:
	// https://chromium.googlesource.com/chromiumos/third_party/kernel/+/chromeos-3.18/drivers/net/wireless/bcmdhd/include/bcmdevs.h#396
	// In 10.12 and earlier _si_pmu_fvco_pllreg will fail if the value stored at [rdi+0x3c] is not 0xaa52, driver won't be loaded

	uint32_t original = a1[15];
	a1[15] = 0xaa52;

	auto ret = FunctionCast(siPmuFvcoPllreg, callbackBRCMFX->orgSiPmuFvcoPllreg)(a1, a2, a3);

	a1[15] = original;

	return ret;
}

//==============================================================================
// 23 61 means '#' 'a',  "#a" is universal for all countries
// 55 53 -> US
// 43 4e -> CN

int64_t BRCMFX::wlc_set_countrycode_rev(int64_t a1, const char *country_code, int a3)
{
	DBGLOG("BRCMFX", "wlc_set_countrycode_rev is called, a3 = %d, country_code = %s", a3, country_code);

	const char *new_country_code = ADDPR(brcmfx_config).country_code;
	if (!ADDPR(brcmfx_config).country_code_overrided && strlen(callbackBRCMFX->provider_country_code))
	{
		new_country_code = callbackBRCMFX->provider_country_code;
		DBGLOG("BRCMFX", "country code is overrided in ioreg");
	}

	a3 = -1;
	int64_t result = FunctionCast(wlc_set_countrycode_rev, callbackBRCMFX->orgWlcSetCountryCodeRev)(a1, new_country_code, a3);
	DBGLOG("BRCMFX", "country code is changed from %s to %s, result = %lld", country_code, new_country_code, result);
	IOSleep(100);

	return result;
}

//==============================================================================

bool  BRCMFX::wlc_wowl_enable(int64_t **a1)
{
	DBGLOG("BRCMFX", "wlc_wowl_enable is called, change returned value to false");
	return false;
}

//==============================================================================

bool BRCMFX::start(IOService* service, IOService* provider)
{
	DBGLOG("BRCMFX", "start is called, service name is %s, provider name is %s", service->getName(), provider->getName());

	if (callbackBRCMFX->wl_msg_level && ADDPR(brcmfx_config).wl_msg_level != 0)
		*callbackBRCMFX->wl_msg_level = ADDPR(brcmfx_config).wl_msg_level;

	if (callbackBRCMFX->wl_msg_level2 && ADDPR(brcmfx_config).wl_msg_level2 != 0)
		*callbackBRCMFX->wl_msg_level2 = ADDPR(brcmfx_config).wl_msg_level2;

	auto data = OSDynamicCast(OSData, provider->getProperty(ADDPR(brcmfx_config).bootargBrcmCountry));
	if (data)
	{
		lilu_os_strncpy(callbackBRCMFX->provider_country_code, reinterpret_cast<const char*>(data->getBytesNoCopy()), data->getLength());
		DBGLOG("BRCMFX", "brcmfx-country in ioreg is set to %s", callbackBRCMFX->provider_country_code);
	}

	return FunctionCast(start, callbackBRCMFX->orgStart)(service, provider);
}

//==============================================================================

bool BRCMFX::start_mfg(IOService *service, IOService* provider)
{
	DBGLOG("BRCMFX", "start_mfg is called, service name is %s, provider name is %s", service->getName(), provider->getName());
	return false;
}

//==============================================================================

IOService* BRCMFX::probe_mfg(IOService *service, IOService * provider, SInt32 *score)
{
	DBGLOG("BRCMFX", "probe_mfg is called");
	return nullptr;
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
IOService* findService(const IORegistryPlane* plane, const char *service_name)
{
	IOService            * service = 0;
	IORegistryIterator   * iter = IORegistryIterator::iterateOver(plane, kIORegistryIterateRecursively);
	OSOrderedSet         * all = 0;

	if ( iter)
	{
		do
		{
			if (all)
				all->release();
			all = iter->iterateAll();
		}
		while (!iter->isValid());
		iter->release();

		if (all)
		{
			while ((service = OSDynamicCast(IOService, all->getFirstObject())))
			{
				all->removeObject(service);
				if (strcmp(service->getName(), service_name) == 0)
					break;
			}

			all->release();
		}
	}

	return service;
}

//==============================================================================

void BRCMFX::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
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

				if (i != 0) {
					KernelPatcher::RouteRequest requests[] {
						// Failed PCIe configuration (device-id checking)
						{symbolList[i][0], start, orgStart},
						// Chip identificator checking patch
						{symbolList[i][1], siPmuFvcoPllreg, orgSiPmuFvcoPllreg},
						// Wi-Fi 5 Ghz/Country code patch (required for 10.11)
						{symbolList[i][2], wlc_set_countrycode_rev, orgWlcSetCountryCodeRev},
						// Third party device patch
						{symbolList[i][3], newVendorString},
						// White list restriction patch
						{symbolList[i][4], checkBoardId},
						// Disable "32KHz LPO Clock not running" panic in AirPort_BrcmXXX
						{symbolList[i][6], osl_panic},
					};

					patcher.routeMultiple(index, requests, address, size);

					// Disable WOWL (WoWLAN)
					if (!ADDPR(brcmfx_config).enable_wowl)
					{
						KernelPatcher::RouteRequest request {symbolList[i][5], wlc_wowl_enable};
						patcher.routeMultiple(index, &request, 1, address, size);
					}

					wl_msg_level = patcher.solveSymbol<int32_t *>(index, "_wl_msg_level", address, size);
					wl_msg_level2 = patcher.solveSymbol<int32_t *>(index, "_wl_msg_level2", address, size);
				} else {
					KernelPatcher::RouteRequest requests[] {
						// Failed PCIe configuration (device-id checking)
						{symbolList[i][0], start, orgStart},
						{"__ZN19AirPort_BrcmNIC_MFG5probeEP9IOServicePi", probe_mfg}
					};
					patcher.routeMultiple(index, requests, address, size);
				}

				ADDPR(brcmfx_config).disabled = true;

				IOService *service = findService(gIOServicePlane, "FakeBrcm");
				if (service && service->getProvider() != nullptr)
				{
					auto bundle  = OSDynamicCast(OSString, service->getProperty(kCFBundleIdentifierKey));
					auto ioclass = OSDynamicCast(OSString, service->getProperty(KIOClass));
					bundle  = bundle  ? bundle->withString(bundle)   : nullptr;
					ioclass = ioclass ? ioclass->withString(ioclass) : nullptr;

					IOService *provider = service->getProvider();

					bool success = false;
					if (service->requestTerminate(provider, 0) && service->terminate()) {
						service->stop(provider);
						success = true;
					}

					DBGLOG("BRCMFX", "terminating FakeBrcm with status %d", success);

					if (provider->isOpen(service))
					{
						provider->close(service);
						DBGLOG("BRCMFX", "FakeBrcm was closed");
					}

					if (success && bundle && ioclass)
					{
						OSDictionary * dict = OSDictionary::withCapacity(2);
						dict->setObject(kCFBundleIdentifierKey, bundle);
						dict->setObject(KIOClass, ioclass);
						if (!gIOCatalogue->removeDrivers(dict, true))
							SYSLOG("BRCMFX", "gIOCatalogue->removeDrivers failed");
						else
							DBGLOG("BRCMFX", "gIOCatalogue->removeDrivers successful");
						OSSafeReleaseNULL(dict);
					}
				}
				else
				{
					OSDictionary* dict = OSDictionary::withCapacity(1);
					dict->setObject("IOProviderClass", OSSymbol::withCStringNoCopy("IOPCIDevice"));
					if (!gIOCatalogue->startMatching(dict))
						SYSLOG("BRCMFX", "gIOCatalogue->startMatching failed");
					else
						DBGLOG("BRCMFX", "gIOCatalogue->startMatching successful");
					OSSafeReleaseNULL(dict);
				}

				break;
			}
		}
	}

	// Ignore all the errors for other processors
	patcher.clearError();
}


