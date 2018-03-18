#include <Library/LegacyIOService.h>
#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_patcher.hpp>

#include "kern_config.hpp"
#include "kern_fakebrcm.hpp"
#include "kern_misc.hpp"

OSDefineMetaClassAndStructors(FakeBrcm, IOService);


IOService *FakeBrcm::service_provider {nullptr};
bool FakeBrcm::service_found {false};
FakeBrcm::t_config_read16  FakeBrcm::orgConfigRead16 {nullptr};
FakeBrcm::t_config_read32  FakeBrcm::orgConfigRead32 {nullptr};

//==============================================================================

bool FakeBrcm::init(OSDictionary *propTable)
{
    if (config.disabled)
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
	service_provider = provider;
    hookProvider(provider);
    
    return super::attach(provider);
}

//==============================================================================

IOService* FakeBrcm::probe(IOService * provider, SInt32 *score)
{
    DBGLOG("BRCMFX", "FakeBrcm::probe()");

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
	
	*score = -2000;

    if (config.disabled)
    {
        DBGLOG("BRCMFX", "FakeBrcm::probe(): FakeBrcm disabled");
        return ret;
    }
	
    DBGLOG("BRCMFX", "FakeBrcm::probe(): service provider is %s", provider->getName());
    
    for (int i=0; i<kextListSize; i++)
    {
        const OSMetaClass * meta_class = OSMetaClass::getMetaClassWithName(OSSymbol::withCStringNoCopy(serviceNameList[i]));
        if (meta_class != nullptr)
        {
            DBGLOG("BRCMFX", "FakeBrcm::probe(): meta class for %s exists!", serviceNameList[i]);

            IOService *service = (IOService *) OSMetaClass::allocClassWithName(serviceNameList[i]);
            if (service != nullptr)
            {
                service->retain();
                DBGLOG("BRCMFX", "FakeBrcm: retain counter for driver %s is %d", serviceNameList[i], service->getRetainCount());
				service_found = true;
            }
            else
                SYSLOG("BRCMFX", "FakeBrcm: instance of driver %s couldn't be created", serviceNameList[i]);
        }
    }
	
    if (service_found)
    {
        DBGLOG("BRCMFX", "FakeBrcm::probe() will change score from %d to 2000", *score);
        *score = 2000;  // change probe score to be the first in the list
    }
    else
    {
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

    if (config.disabled)
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
	
	service_provider = provider;
    hookProvider(provider);

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


//==============================================================================

bool FakeBrcm::isServiceSupported(IOService* service)
{
	if (service == service_provider)
		return true;
	
	for (int i=0; i<kextListSize; i++)
	{
		if (strcmp(serviceNameList[i], service->getName()) == 0)
			return true;
	}
	return false;
}

//==============================================================================

UInt16 FakeBrcm::configRead16(IOService *that, UInt32 space, UInt8 offset)
{
    UInt16 result = orgConfigRead16(that, space, offset);
    UInt16 newResult = result;
    
    if ((offset == WIOKit::PCIRegister::kIOPCIConfigVendorID || offset == WIOKit::PCIRegister::kIOPCIConfigDeviceID) && isServiceSupported(that))
    switch (offset)
    {
        case WIOKit::PCIRegister::kIOPCIConfigVendorID:
        {
            UInt32 vendor;
            if (WIOKit::getOSDataValue(that, "vendor-id", vendor))
                newResult = vendor;
            break;
        }
        case WIOKit::PCIRegister::kIOPCIConfigDeviceID:
        {
            UInt32 device;
            if (WIOKit::getOSDataValue(that, "device-id", device))
                newResult = device;
            break;
        }
    }
    
    if (newResult != result)
        DBGLOG("BRCMFX", "FakeBrcm::configRead16: name = %s, source value = 0x%04x replaced with value = 0x%04x", that->getName(), result, newResult);

    return newResult;
}

//==============================================================================

UInt32 FakeBrcm::configRead32(IOService *that, UInt32 space, UInt8 offset)
{
    UInt32 result = orgConfigRead32(that, space, offset);
    UInt32 newResult = result;
	
	// OS X does a non-aligned read, which still returns full vendor / device ID
    if ((offset == WIOKit::PCIRegister::kIOPCIConfigVendorID || offset == WIOKit::PCIRegister::kIOPCIConfigDeviceID) && isServiceSupported(that))
	{
		UInt32 vendor, device;
		if (WIOKit::getOSDataValue(that, "vendor-id", vendor))
			newResult = (newResult & 0xFFFF0000) | vendor;
		if (WIOKit::getOSDataValue(that, "device-id", device))
			newResult = (device << 16) | (newResult & 0xFFFF);
	}
    
    if (newResult != result)
        DBGLOG("BRCMFX", "FakeBrcm::configRead32: name = %s, source value = 0x%08x replaced with value = 0x%08x", that->getName(), result, newResult);
    
    return newResult;
}

//==============================================================================

void FakeBrcm::hookProvider(IOService *provider)
{
	if (orgConfigRead16 == nullptr)
		if (KernelPatcher::routeVirtual(provider, WIOKit::PCIConfigOffset::ConfigRead16, configRead16, &orgConfigRead16))
			DBGLOG("BRCMFX", "FakeBrcm::hookProvider for configRead16 was successful");
	
	if (orgConfigRead32 == nullptr)
		if (KernelPatcher::routeVirtual(provider, WIOKit::PCIConfigOffset::ConfigRead32, configRead32, &orgConfigRead32))
			DBGLOG("BRCMFX", "FakeBrcm::hookProvider for configRead32 was successful");
}

