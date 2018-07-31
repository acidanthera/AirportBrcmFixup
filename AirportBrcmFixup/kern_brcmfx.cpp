//
//  kern_brcmfx.cpp
//  BRCMFX
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
static KernelPatcher *callbackPatcher {nullptr};

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
    LiluAPI::Error error = lilu.onKextLoad(kextList, kextListSize,
                                           [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
                                               callbackBRCMFX = static_cast<BRCMFX *>(user);
                                               callbackPatcher = &patcher;
                                               callbackBRCMFX->processKext(patcher, index, address, size);
                                           }, this);

    if (error != LiluAPI::Error::NoError) {
        SYSLOG("BRCMFX", "failed to register onKextLoad method %d", error);
        return false;
    }

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
// 23 61 means '#' 'a',  "#a" is universal for all countries
// 55 53 -> US
// 43 4e -> CN

int64_t BRCMFX::wlc_set_countrycode_rev(int64_t a1, const char *country_code, int a3)
{
    int64_t result = 0;
    DBGLOG("BRCMFX", "wlc_set_countrycode_rev is called, a3 = %d, country_code = %s", a3, country_code);
    if (callbackBRCMFX && callbackPatcher && callbackBRCMFX->orgWlcSetCountryCodeRev)
    {
        const char *new_country_code = ADDPR(brcmfx_config).country_code;
        if (!ADDPR(brcmfx_config).country_code_overrided && strlen(callbackBRCMFX->provider_country_code))
        {
			new_country_code = callbackBRCMFX->provider_country_code;
			DBGLOG("BRCMFX", "country code is overrided in ioreg");
        }

        a3 = -1;
        result = callbackBRCMFX->orgWlcSetCountryCodeRev(a1, new_country_code, a3);
        DBGLOG("BRCMFX", "country code is changed from %s to %s, result = %lld", country_code, new_country_code, result);
        IOSleep(100);
    }

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
    bool result = false;
    DBGLOG("BRCMFX", "start is called, service name is %s, provider name is %s", service->getName(), provider->getName());

	if (callbackBRCMFX && callbackPatcher)
	{
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

		if (callbackPatcher && callbackBRCMFX->orgStart)
			result = callbackBRCMFX->orgStart(service, provider);
	}

    return result;
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

/*******************************************************************************
// This assembler function is needed since method _si_pmu_fvco_pllreg uses user calling convention (rdi, rsi, rdx, rax)
// (more details on https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI)
// _si_pmu_fvco_pllreg (10.11 - 10.13) compares value stored at [rdi+0x3c] with 0xaa52. This value is a chip identificator: 43602, details:
// https://chromium.googlesource.com/chromiumos/third_party/kernel/+/chromeos-3.18/drivers/net/wireless/bcmdhd/include/bcmdevs.h#396
// In 10.12 and earlier _si_pmu_fvco_pllreg will fail if the value stored at [rdi+0x3c] is not 0xaa52, driver won't be loaded
// The idea behind of this assembler function (credits to vit9696):
// - save rdi on stack
// - save current value at [rdi+0x3c] on stack (it may be something different from 0xaa52, we will restore it later)
// - set a field of data structure to the expected value 0xaa52
// - call the next instruction after call (by address 11) -> it's a common way to retrieve an absolute execution pointer (rip) (0x11 in this example)
// - add 8 bytes to the rip (skip opcodes 59 48 83 C1 08 51 EB 03) in order to calculate an address of the instruction "pop rcx" (0x19)
// - save the calculated value on stack (it will be used as a return address by instruction ret in _si_pmu_fvco_pllreg)
// - jump to block of opcodes, generated by Lilu (to address 1F), finally the original method _si_pmu_fvco_pllreg will be called
// _si_pmu_fvco_pllreg is executed and afterwards returns to address 19.
// - restore rcx and rdi
// - set the stored value at [rdi+0x3c]
// - return to the caller (_wlc_proxd_attach or _pdburst_collection)

 Assembler wrapper for calling _si_pmu_fvco_pllreg:
 00000000000000 57                     push       rdi                       // entry point, when _wlc_proxd_attach calls _si_pmu_fvco_pllreg, we are here
 00000000000001 8B4F3C                 mov        ecx, dword [rdi+0x3c]
 00000000000004 51                     push       rcx
 00000000000005 C7473C52AA0000         mov        dword [rdi+0x3c], 0xaa52
 0000000000000C E800000000             call       $+5                       // calls the next instruction to retrieve rip
 00000000000011 59                     pop        rcx                       // rcx = 0x11 (absolute address in a real life)
 00000000000012 4883C108               add        rcx, 0x8                  // add 0x8 to get an absolute address of instruction "pop rcx"
 00000000000016 51                     push       rcx                       // push an absolute address (will be used by instruction ret later on)
 00000000000017 EB06                   jmp        $+6                       // jump to Lilu_wrapper, _si_pmu_fvco_pllreg will be called
 00000000000019 59                     pop        rcx                       // we are here after return from _si_pmu_fvco_pllreg performs
 0000000000001A 5F                     pop        rdi
 0000000000001B 894F3C                 mov        dword [rdi+0x3c], ecx     // restore saved value
 0000000000001E C3                     ret                                  // return to _wlc_proxd_attach
 ******************************************************************************
 These opcodes were written and generated by Lilu automatically (by routeBlock)
 Lilu_wrapper:
 0000000000001F 55 48 89 e5 41 57 41 56 53 50 49 89 d6 48 89 f3             // this is the copy of _si_pmu_fvco_pllreg (the beginning)
 0000000000002E e9 20 95 cb fe                                              // jump to the rest of original part _si_pmu_fvco_pllreg's body
 */

static const uint8_t si_pmu_fvco_pllreg_opcodes[] = {
    0x57, 0x8B, 0x4F, 0x3C, 0x51, 0xC7,
    0x47, 0x3C, 0x52, 0xAA, 0x00, 0x00,
    0xE8, 0x00, 0x00, 0x00, 0x00, 0x59,
    0x48, 0x83, 0xC1, 0x08, 0x51, 0xEB,
    0x06, 0x59, 0x5F, 0x89, 0x4F, 0x3C,
    0xC3
};


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

                // Failed PCIe configuration (device-id checking)
                const char *method_name = symbolList[i][0];
                auto method_address = patcher.solveSymbol(index, method_name, address, size);
                if (method_address) {
                    DBGLOG("BRCMFX", "obtained %s", method_name);
                    patcher.clearError();
                    if (i != 0)
                        orgStart = reinterpret_cast<t_start>(patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(start), true));
                    else
                    {
                        patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(start_mfg), true);
                        auto probe_method_address = patcher.solveSymbol(index, "__ZN19AirPort_BrcmNIC_MFG5probeEP9IOServicePi", address, size);
                        if (probe_method_address)
                            patcher.routeFunction(probe_method_address, reinterpret_cast<mach_vm_address_t>(probe_mfg));
                    }

                    if (patcher.getError() == KernelPatcher::Error::NoError) {
                        DBGLOG("BRCMFX", "routed %s", method_name);
                    } else {
                        SYSLOG("BRCMFX", "failed to route %s", method_name);
                    }
                } else {
                    SYSLOG("BRCMFX", "failed to resolve %s", method_name);
                }

                if (i != 0)
                {
                    // Chip identificator checking patch
                    method_name = symbolList[i][1];
                    method_address = patcher.solveSymbol(index, method_name, address, size);
                    if (method_address) {
                        DBGLOG("BRCMFX", "obtained %s", method_name);
                        patcher.clearError();
                        patcher.routeBlock(method_address, si_pmu_fvco_pllreg_opcodes, sizeof(si_pmu_fvco_pllreg_opcodes), true);
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX", "routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX", "failed to route %s", method_name);
                        }
                    } else {
                        SYSLOG("BRCMFX", "failed to resolve %s", method_name);
                    }

                    // Wi-Fi 5 Ghz/Country code patch (required for 10.11)
                    method_name = symbolList[i][2];
                    method_address = patcher.solveSymbol(index, method_name, address, size);
                    if (method_address) {
                        DBGLOG("BRCMFX", "obtained %s", method_name);
                        patcher.clearError();
                        orgWlcSetCountryCodeRev = reinterpret_cast<t_wlc_set_countrycode_rev>(patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(wlc_set_countrycode_rev), true));
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX", "routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX", "failed to route %s", method_name);
                        }
                    } else {
                        SYSLOG("BRCMFX", "failed to resolve %s", method_name);
                    }

                    // Third party device patch
                    method_name = symbolList[i][3];
                    method_address = patcher.solveSymbol(index, method_name, address, size);
                    if (method_address) {
                        DBGLOG("BRCMFX", "obtained %s", method_name);
                        patcher.clearError();
                        patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(newVendorString), true);
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX", "routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX", "failed to route %s", method_name);
                        }
                    } else {
                        SYSLOG("BRCMFX", "failed to resolve %s", method_name);
                    }

                    // White list restriction patch
                    method_name = symbolList[i][4];
                    method_address = patcher.solveSymbol(index, method_name, address, size);
                    if (method_address) {
                        DBGLOG("BRCMFX", "obtained %s", method_name);
                        patcher.clearError();
                        patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(checkBoardId), true);
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX", "routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX", "failed to route %s", method_name);
                        }
                    } else {
                        SYSLOG("BRCMFX", "failed to resolve %s", method_name);
                    }

                    // Disable WOWL (WoWLAN)
                    if (!ADDPR(brcmfx_config).enable_wowl)
                    {
                        method_name = symbolList[i][5];
                        method_address = patcher.solveSymbol(index, method_name, address, size);
                        if (method_address) {
                            DBGLOG("BRCMFX", "obtained %s", method_name);
                            patcher.clearError();
                            patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(wlc_wowl_enable), true);
                            if (patcher.getError() == KernelPatcher::Error::NoError) {
                                DBGLOG("BRCMFX", "routed %s", method_name);
                            } else {
                                SYSLOG("BRCMFX", "failed to route %s", method_name);
                            }
                        } else {
                            SYSLOG("BRCMFX", "failed to resolve %s", method_name);
                        }
                    }

                    wl_msg_level = reinterpret_cast<int32_t *>(patcher.solveSymbol(index, "_wl_msg_level", address, size));
                    wl_msg_level2 = reinterpret_cast<int32_t *>(patcher.solveSymbol(index, "_wl_msg_level2", address, size));
                }

                // Disable "32KHz LPO Clock not running" panic in AirPortBrcmNIC
                if (i == 2)
                {
                    uint8_t find[]    = {0x25, 0x00, 0x31, 0xC0, 0xE8};
                    uint8_t replace[] = {0x25, 0x00, 0xEB, 0x05, 0xE8};
                    KernelPatcher::LookupPatch patch {
                        &kextList[2], find, replace, sizeof(find), 1
                    };

                    patcher.applyLookupPatch(&patch);
                    if (patcher.getError() != KernelPatcher::Error::NoError) {
                        SYSLOG("BRCMFX", "failed to apply LPO panic patch %d", patcher.getError());
                        patcher.clearError();
                    }
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
                    service->stop(provider);
                    bool success = service->terminate();
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


