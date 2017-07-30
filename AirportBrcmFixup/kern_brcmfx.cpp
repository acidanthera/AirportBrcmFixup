//
//  kern_brcmfx.cpp
//  BRCMFX
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_file.hpp>

#include "kern_config.hpp"
#include "kern_brcmfx.hpp"

#include <IOKit/IORegistryEntry.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOReportTypes.h>


// Only used in apple-driven callbacks
static BRCMFX *callbackBRCMFX {nullptr};
static KernelPatcher *callbackPatcher {nullptr};
static const IORegistryPlane* gIO80211FamilyPlane {nullptr};


static const char *idList[] {
    "com.apple.driver.AirPort.Brcm4360",
    "com.apple.driver.AirPort.BrcmNIC",
    "com.apple.driver.AirPort.BrcmNIC-MFG"
};

static const char *serviceNameList[] {
    "AirPort_Brcm4360",
    "AirPort_BrcmNIC",
    "AirPort_BrcmNIC_MFG"
};

static const int processingStateList[] {
    BRCMFX::ProcessingState::BRCM4360Patched,
    BRCMFX::ProcessingState::BRCMNICPatched,
    BRCMFX::ProcessingState::BRCMNIC_MFGPatched
};

static const char *binList[] {
    "/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcm4360.kext/Contents/MacOS/AirPortBrcm4360",
    "/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AirPortBrcmNIC.kext/Contents/MacOS/AirPortBrcmNIC",
    "/System/Library/Extensions/AirPortBrcmNIC-MFG.kext/Contents/MacOS/AirPortBrcmNIC-MFG"
};

// 4360: __ZN16AirPort_Brcm436014getPCIPropertyEP9IOServicePKcRj
// NIC:  __ZN15AirPort_BrcmNIC14getPCIPropertyEP9IOServicePKcR
// MFG:  __ZN19AirPort_BrcmNIC_MFG14getPCIPropertyEP9IOServicePKcRj

static const char *symbolList[][6] {
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK16AirPort_Brcm436015newVendorStringEv",    "__ZN16AirPort_Brcm436012checkBoardIdEPKc",
        "__ZN16AirPort_Brcm43605startEP9IOService",    "__ZN16AirPort_Brcm43605probeEP9IOServicePi"  },
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK15AirPort_BrcmNIC15newVendorStringEv",     "__ZN15AirPort_BrcmNIC12checkBoardIdEPKc" ,
         "__ZN15AirPort_BrcmNIC5startEP9IOService",    "__ZN15AirPort_BrcmNIC5probeEP9IOServicePi"   },
    {"_si_pmu_fvco_pllreg",  "_wlc_set_countrycode_rev",  "__ZNK19AirPort_BrcmNIC_MFG15newVendorStringEv", "__ZN19AirPort_BrcmNIC_MFG12checkBoardIdEPKc",
        "__ZN19AirPort_BrcmNIC_MFG5startEP9IOService", "__ZN19AirPort_BrcmNIC_MFG5probeEP9IOServicePi" }
};

static KernelPatcher::KextInfo kextList[] {
    { idList[0], &binList[0], 1, true, {}, KernelPatcher::KextInfo::Unloaded },
    { idList[1], &binList[1], 1, true, {}, KernelPatcher::KextInfo::Unloaded },
    { idList[2], &binList[2], 1, true, {}, KernelPatcher::KextInfo::Unloaded }
};

static const size_t kextListSize {3};


//==============================================================================

bool BRCMFX::init()
{
    gIO80211FamilyPlane	= IORegistryEntry::makePlane( "IO80211Plane" );
    
    LiluAPI::Error error = lilu.onKextLoad(kextList, kextListSize,
                                           [](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
                                               callbackBRCMFX = static_cast<BRCMFX *>(user);
                                               callbackPatcher = &patcher;
                                               callbackBRCMFX->processKext(patcher, index, address, size);
                                           }, this);
    
    if (error != LiluAPI::Error::NoError) {
        SYSLOG("BRCMFX @ failed to register onKextLoad method %d", error);
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
    return true;
}

//==============================================================================

const OSSymbol* BRCMFX::newVendorString(void)
{
    return OSSymbol::withCString("Apple");
}

//==============================================================================

IOService* BRCMFX::probe(IOService* provider, SInt32* score)
{
    return nullptr;
}

//==============================================================================

UInt16 BRCMFX::configRead16(IOPCIDevice *that, IOPCIAddressSpace space, UInt8 offset)
{
    auto val = callbackBRCMFX->orgConfigRead16(that, space, offset);
    
    if (offset == kIOPCIConfigDeviceID)
    {
        DBGLOG("BRCMFX @ configRead16 is called, offset = %d, device-id = 0x%04x", offset, val);

        auto realdev = OSDynamicCast(OSData, that->getProperty("device-id"));
        auto realven = OSDynamicCast(OSData, that->getProperty("vendor-id"));
        if (realdev && realdev->getLength() >= 2 && realven && realven->getLength() >= 2)
        {
            auto rdevice_id = static_cast<const uint16_t *>(realdev->getBytesNoCopy());
            auto rvendor_id = static_cast<const uint16_t *>(realven->getBytesNoCopy());
            if (rdevice_id && rvendor_id)
            {
                auto vendor_id = callbackBRCMFX->orgConfigRead16(that, space, kIOPCIConfigVendorID);
                if (vendor_id == *rvendor_id && val != *rdevice_id)
                {
                    val = *rdevice_id;
                    DBGLOG("BRCMFX @ configRead16 replaced device-id with value = 0x%04x", val);
                }
            }
        }
    }
    
    return val;
}

//==============================================================================

bool BRCMFX::start(IOService* service, IOService* provider)
{
    bool result = false;
    DBGLOG("BRCMFX @ start is called");

    if (callbackBRCMFX && callbackPatcher && callbackBRCMFX->orgStart)
    {
        void * pcidev = static_cast<void *>(provider);
        uint64_t * vmt = pcidev ? static_cast<uint64_t **>(pcidev)[0] : nullptr;
        if (vmt)
        {
            callbackBRCMFX->orgConfigRead16 = reinterpret_cast<t_config_read16>(vmt[VMTOffset::configRead16]);
            vmt[VMTOffset::configRead16] = reinterpret_cast<uint64_t>(BRCMFX::configRead16);
        }
        
        result = callbackBRCMFX->orgStart(service, provider);
        
        if (vmt)
            vmt[VMTOffset::configRead16] = reinterpret_cast<uint64_t>(callbackBRCMFX->orgConfigRead16);
    }
    
    return result;
}

//==============================================================================

//bool BRCMFX::getPCIProperty(IOService* service, char const* name, unsigned int& value)
//{
//    bool result = false;
//    if (callbackBRCMFX && callbackPatcher && callbackBRCMFX->orgGetPCIProperty)
//    {
//        if (strcmp(name, "device-id") == 0)
//        {
//            IOPCIDevice* pcidev = OSDynamicCast(IOPCIDevice, service);
//            value = pcidev->extendedConfigRead16(kIOPCIConfigDeviceID);
//            result = true;
//        }
//        else if (strcmp(name, "vendor-id") == 0)
//        {
//            IOPCIDevice* pcidev = OSDynamicCast(IOPCIDevice, service);
//            value = pcidev->extendedConfigRead16(kIOPCIConfigVendorID);
//            result = true;
//        }
//        else
//            result = callbackBRCMFX->orgGetPCIProperty(service, name, value);
//    }
//    
//    return result;
//}

//==============================================================================

IOService* BRCMFX::findService(const IORegistryPlane* plane, const char *service_name)
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

bool BRCMFX::startService(IOService* service)
{
    bool result = false;
    IOService *provider = service->getProvider();
    if (provider != nullptr)
    {
        DBGLOG("BRCMFX @ service name = %s, provider name = %s", service->getName(), provider->getName());
        if (service->attach(provider))
        {
            SInt32 score = 0;
            IOService *service_to_start = service->probe(provider, &score);
            service->detach(provider);
            IOSleep(300);
            if (service_to_start != nullptr)
            {
                service_to_start->retain();
                if (service_to_start->attach(provider))
                {
                    result = service_to_start->start(provider);
                    if (!result)
                    {
                        SYSLOG("BRCMFX @ service %s can't be started", service_to_start->getName());
                        service->detach( provider );
                    }
                }
                else
                    SYSLOG("BRCMFX @ service %s can't be attached to provider", service_to_start->getName());
                service_to_start->release();
            }
            else
                SYSLOG("BRCMFX @ service %s probe returned null", service->getName());
        }
        else
            SYSLOG("BRCMFX @ service %s can't be attached to provider %s", service->getName(), provider->getName());
    }
    else
        SYSLOG("BRCMFX @ service %s doesn't have provider", service->getName());
    return result;
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

/*******************************************************************************
Assembler wrapper for calling _wlc_set_countrycode_rev (Wi-Fi 5Ghz patch):
// The idea behind of this patch: write country code value 0x6123/0x5355 to [rsi] - TODO: clarify
00000000000000 51                     push       rcx                       // entry point, when _wlc_channel_init_ccode calls _wlc_set_countrycode_rev, we are here
00000000000001 66C7062361             mov        word [rsi], 0x6123
00000000000006 E800000000             call       $+5                       // calls the next instruction to retrieve rip
0000000000000B 59                     pop        rcx                       // rcx = 0x0B (absolute address in a real life)
0000000000000C 4883C108               add        rcx, 0x8                  // add 0x8 to get an absolute address of instruction "pop rcx"
00000000000010 51                     push       rcx                       // push an absolute address (will be used by instruction ret later on)
00000000000011 EB02                   jmp        $+2                       // jump to Lilu_wrapper, _wlc_set_countrycode_rev will be called
00000000000013 59                     pop        rcx                       // we are here after return from _wlc_set_countrycode_rev performs
00000000000014 C3                     ret                                  // return to _wlc_channel_init_ccode
*/
 
static const uint8_t wlc_set_countrycode_rev_opcodes[] = {
//    0x51, 0x66, 0xC7, 0x06, 0x23, 0x61,
    0x51, 0x66, 0xC7, 0x06, 0x55, 0x53,
    0xE8, 0x00, 0x00, 0x00, 0x00, 0x59,
    0x48, 0x83, 0xC1, 0x08, 0x51, 0xEB,
    0x02, 0x59, 0xC3
};

//==============================================================================

void BRCMFX::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
    if (progressState != ProcessingState::EverythingDone)
    {
        for (size_t i = 0; i < kextListSize; i++)
        {
            if (kextList[i].loadIndex == index)
            {
                while (!(progressState & processingStateList[i]))
                {
                    progressState |= processingStateList[i];
                    
                    DBGLOG("BRCMFX @ found %s", idList[i]);

                    // Chip identificator checking patch
                    const char *method_name = symbolList[i][0];
                    auto method_address = patcher.solveSymbol(index, method_name);
                    if (method_address) {
                        DBGLOG("BRCMFX @ obtained %s", method_name);
                        patcher.routeBlock(method_address, si_pmu_fvco_pllreg_opcodes, sizeof(si_pmu_fvco_pllreg_opcodes), true);
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX @ routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX @ failed to route %s", method_name);
                        }
                    } else {
                        SYSLOG("BRCMFX @ failed to resolve %s", method_name);
                    }
                    
                    // Wi-Fi 5 Ghz/Country code patch (required for 10.11)
                    method_name = symbolList[i][1];
                    method_address = patcher.solveSymbol(index, method_name);
                    if (method_address) {
                        DBGLOG("BRCMFX @ obtained %s", method_name);
                        patcher.routeBlock(method_address, wlc_set_countrycode_rev_opcodes, sizeof(wlc_set_countrycode_rev_opcodes), true);
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX @ routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX @ failed to route %s", method_name);
                        }
                    } else {
                        SYSLOG("BRCMFX @ failed to resolve %s", method_name);
                    }
                    
                    // Third party device patch
                    method_name = symbolList[i][2];
                    method_address = patcher.solveSymbol(index, method_name);
                    if (method_address) {
                        DBGLOG("BRCMFX @ obtained %s", method_name);
                        patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(newVendorString), true);
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX @ routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX @ failed to route %s", method_name);
                            break;
                        }
                    } else {
                        SYSLOG("BRCMFX @ failed to resolve %s", method_name);
                        break;
                    }
                    
                    // White list restriction patch
                    method_name = symbolList[i][3];
                    method_address = patcher.solveSymbol(index, method_name);
                    if (method_address) {
                        DBGLOG("BRCMFX @ obtained %s", method_name);
                        patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(checkBoardId), true);
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX @ routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX @ failed to route %s", method_name);
                            break;
                        }
                    } else {
                        SYSLOG("BRCMFX @ failed to resolve %s", method_name);
                        break;
                    }
                    
                    // Failed PCIe configuration (device-id checking)
                    method_name = symbolList[i][4];
                    method_address = patcher.solveSymbol(index, method_name);
                    if (method_address) {
                        DBGLOG("BRCMFX @ obtained %s", method_name);
                        orgStart = reinterpret_cast<t_start>(patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(start), true));
                        if (patcher.getError() == KernelPatcher::Error::NoError) {
                            DBGLOG("BRCMFX @ routed %s", method_name);
                        } else {
                            SYSLOG("BRCMFX @ failed to route %s", method_name);
                            break;
                        }
                    } else {
                        SYSLOG("BRCMFX @ failed to resolve %s", method_name);
                        break;
                    }

                    // IO80211FamilyPlane should keep a pointer to broadcom driver (even if it's not started/probed)
                    IOService* service = findService(gIO80211FamilyPlane, serviceNameList[i]);
                    // IOServicePlane should keep a pointer to broadcom driver only if it was successfully started
                    IOService* running_service = findService(gIOServicePlane, serviceNameList[i]);
                    if (service != nullptr)
                        DBGLOG("BRCMFX @ driver %s is found in IO80211FamlilyPlane", serviceNameList[i]);
                    if (running_service != nullptr)
                        SYSLOG("BRCMFX @ %s driver is already loaded, too late to do patching", serviceNameList[i]);

                    // In 10.12 (and probably earlier) BRCMFX::processKext is called too late, after failed attempt to start broadcom driver,
                    // so we have to try to start it again after all required patches
                    if (getKernelVersion() < KernelVersion::HighSierra && service != nullptr && running_service == nullptr && startService(service))
                    {
                        SYSLOG("BRCMFX @ service %s successfully started", service->getName());
                        
                        // we have to disable probing in the future, otherwise system can try to run this service again (10.13)
                        method_name = symbolList[i][5];
                        method_address = patcher.solveSymbol(index, method_name);
                        if (method_address) {
                            DBGLOG("BRCMFX @ obtained %s", method_name);
                            patcher.routeFunction(method_address, reinterpret_cast<mach_vm_address_t>(probe), true);
                            if (patcher.getError() == KernelPatcher::Error::NoError) {
                                DBGLOG("BRCMFX @ routed %s", method_name);
                            } else {
                                SYSLOG("BRCMFX @ failed to route %s", method_name);
                            }
                        } else {
                            SYSLOG("BRCMFX @ failed to resolve %s", method_name);
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // Ignore all the errors for other processors
    patcher.clearError();
}


