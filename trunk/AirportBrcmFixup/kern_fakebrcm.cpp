#include <Library/LegacyIOService.h>
#include <Headers/kern_api.hpp>

#include "kern_config.hpp"
#include "kern_fakebrcm.hpp"
#include "kern_misc.hpp"

OSDefineMetaClassAndStructors(FakeBrcm, IOService);

IOService *FakeBrcm::service_provider {nullptr};
OSDictionary *FakeBrcm::service_dict {nullptr};
OSDictionary *FakeBrcm::prop_table {nullptr};
FakeBrcm::t_config_read16  FakeBrcm::orgConfigRead16 {nullptr};
FakeBrcm::t_config_read32  FakeBrcm::orgConfigRead32 {nullptr};

//==============================================================================

int getIntegerProperty(IORegistryEntry* entry, const char *aKey)
{
    OSData* data = OSDynamicCast(OSData, entry->getProperty(aKey));
    if (!data || sizeof(UInt32) != data->getLength())
    {
        return -1;
    }
    UInt32 result = *static_cast<const UInt32*>(data->getBytesNoCopy());
    return result;
}

//==============================================================================

OSString* getStringProperty(IORegistryEntry* entry, const char *aKey)
{
    OSData* data = OSDynamicCast(OSData, entry->getProperty(aKey));
    if (!data)
    {
        return nullptr;
    }
    return OSString::withCString((const char *)data->getBytesNoCopy());
}

//==============================================================================

bool isServiceSupported(IOService* service)
{
    for (int i=0; i<kextListSize; i++)
    {
        if (strcmp(serviceNameList[i], service->getName()) == 0)
            return true;
    }
    return false;
}

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
    
    service_dict = OSDictionary::withCapacity(1);
    
    if (propTable != nullptr)
        prop_table = OSDictionary::withDictionary(propTable);
    
    return true;
}

//==============================================================================

bool FakeBrcm::attach(IOService *provider)
{  
    hookProvider(provider);
    
    return super::attach(provider);
}

//==============================================================================

IOService* FakeBrcm::probe(IOService * provider, SInt32 *score)
{
    DBGLOG("BRCMFX", "FakeBrcm::probe()");
    
    IOService* ret = super::probe(provider, score);
    if (!ret)
    {
        SYSLOG("BRCMFX", "FakeBrcm super::probe returned nullptr\n");
        return nullptr;
    }
    
    if (config.disabled)
    {
        DBGLOG("BRCMFX", "FakeBrcm::probe(): FakeBrcm disabled");
        *score = -2000;
        return ret;
    }
    
    service_provider = provider;
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
                service_dict->setObject(serviceNameList[i], service);
            }
            else
                SYSLOG("BRCMFX", "FakeBrcm: instance of driver %s couldn't be created", serviceNameList[i]);
        }
    }
    
    if (service_dict->getCount() != 0)
    {
        DBGLOG("BRCMFX", "FakeBrcm::probe() will change score from %d to 2000", *score);
        *score = 2000;  // change probe score to be the first in the list
    }
    else
    {
        DBGLOG("BRCMFX", "FakeBrcm::probe(): fallback to original driver");
        config.disabled = true;
        *score = -2000;
    }

    return ret;
}

//==============================================================================

bool FakeBrcm::start(IOService *provider)
{
    if (config.disabled)
    {
        DBGLOG("BRCMFX", "FakeBrcm::start(): FakeBrcm disabled");
        return false;
    }
    
    DBGLOG("BRCMFX", "FakeBrcm::start()");
    
    if (!super::start(provider))
    {
        SYSLOG("BRCMFX", "FakeBrcm super::start returned false\n");
        return false;
    }
    
    if (!service_dict->getCount())
    {
        SYSLOG("BRCMFX", "FakeBrcm::start(): fallback to original driver");
        return false;
    }
    
    hookProvider(provider);

    return true;
}

//==============================================================================

void FakeBrcm::stop(IOService *provider)
{
    DBGLOG("BRCMFX", "FakeBrcm::stop()");
    
    unhookProvider();

    super::stop(provider);
}

//==============================================================================

void FakeBrcm::free()
{
    DBGLOG("BRCMFX", "FakeBrcm::free()");
    
    OSSafeReleaseNULL(service_dict);
    
    unhookProvider();

    super::free();
}

//==============================================================================

IOService* FakeBrcm::getService(const char* service_name)
{
    if (service_dict == nullptr)
    {
        DBGLOG("BRCMFX", "FakeBrcm::getService(): no dictionary");
        return nullptr;
    }
    
    OSObject* object = service_dict->getObject(service_name);
    if (object == nullptr)
    {
        DBGLOG("BRCMFX", "FakeBrcm::getService(): service %s is not found in dictionary", service_name);
        return nullptr;
    }
    
    IOService* service = OSDynamicCast(IOService, object);
    if (service == nullptr)
    {
        DBGLOG("BRCMFX", "FakeBrcm::getService(): object for service %s can't be casted to IOService", service_name);
        return nullptr;
    }
    
    return service;
}

//==============================================================================

UInt16 FakeBrcm::configRead16(IOService *that, UInt32 space, UInt8 offset)
{
    UInt16 result = orgConfigRead16(that, space, offset);
    UInt16 newResult = result;
    
    if (that == service_provider || isServiceSupported(that))
    switch (offset)
    {
        case kIOPCIConfigVendorID:
        {
            int vendor = getIntegerProperty(that, "vendor-id");
            if (-1 != vendor)
                newResult = vendor;
            break;
        }
        case kIOPCIConfigDeviceID:
        {
            int device = getIntegerProperty(that, "device-id");
            if (-1 != device)
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
    
    if (that == service_provider || isServiceSupported(that))
    switch (offset)
    {
        case kIOPCIConfigVendorID:
        case kIOPCIConfigDeviceID: // OS X does a non-aligned read, which still returns full vendor / device ID
        {
            int vendor = getIntegerProperty(that, "vendor-id");
            if (-1 != vendor)
                newResult = (newResult & 0xFFFF0000) | vendor;
            
            int device = getIntegerProperty(that, "device-id");
            if (-1 != device)
                newResult = (device << 16) | (newResult & 0xFFFF);
            break;
        }
    }
    
    if (newResult != result)
        DBGLOG("BRCMFX", "FakeBrcm::configRead32: name = %s, source value = 0x%08x replaced with value = 0x%08x", that->getName(), result, newResult);
    
    return newResult;
}

//==============================================================================

void FakeBrcm::hookProvider(IOService *provider)
{
    if (getIntegerProperty(provider, "RM,subsystem-id") != -1 ||
        getIntegerProperty(provider, "RM,subsystem-vendor-id") != -1 ||
        getIntegerProperty(provider, "RM,device-id") != -1 ||
        getIntegerProperty(provider, "RM,vendor-id") != -1)
    {
        DBGLOG("BRCMFX", "FakeBrcm::hookProvider - FakePCIID DETECTED!");
    }
    
    void * pcidev = static_cast<void *>(provider);
    uint64_t * vmt = pcidev ? static_cast<uint64_t **>(pcidev)[0] : nullptr;
    if (vmt && vmt[VMTOffset::configRead16] != reinterpret_cast<uint64_t>(FakeBrcm::configRead16) && orgConfigRead16 == nullptr)
    {
        orgConfigRead16 = reinterpret_cast<t_config_read16>(vmt[VMTOffset::configRead16]);
        vmt[VMTOffset::configRead16] = reinterpret_cast<uint64_t>(FakeBrcm::configRead16);
        DBGLOG("BRCMFX", "FakeBrcm::hookProvider for configRead16 was successful");
    }
    
    if (vmt && vmt[VMTOffset::configRead32] != reinterpret_cast<uint64_t>(FakeBrcm::configRead32) && orgConfigRead32 == nullptr)
    {
        orgConfigRead32 = reinterpret_cast<t_config_read32>(vmt[VMTOffset::configRead32]);
        vmt[VMTOffset::configRead32] = reinterpret_cast<uint64_t>(FakeBrcm::configRead32);
        DBGLOG("BRCMFX", "FakeBrcm::hookProvider for configRead32 was successful");
    }
}

//==============================================================================

void FakeBrcm::unhookProvider()
{
    if (service_provider != nullptr)
    {
        void * pcidev = static_cast<void *>(service_provider);
        uint64_t * vmt = pcidev ? static_cast<uint64_t **>(pcidev)[0] : nullptr;
        if (vmt && vmt[VMTOffset::configRead16] != reinterpret_cast<uint64_t>(orgConfigRead16) && orgConfigRead16 != nullptr)
        {
            vmt[VMTOffset::configRead16] = reinterpret_cast<uint64_t>(orgConfigRead16);
            DBGLOG("BRCMFX", "FakeBrcm::unhookProvider for configRead16 was successful");
            orgConfigRead16 = nullptr;
        }
        
        if (vmt && vmt[VMTOffset::configRead32] != reinterpret_cast<uint64_t>(orgConfigRead32) && orgConfigRead32 != nullptr)
        {
            vmt[VMTOffset::configRead32] = reinterpret_cast<uint64_t>(orgConfigRead32);
            DBGLOG("BRCMFX", "FakeBrcm::unhookProvider for configRead32 was successful");
            orgConfigRead32 = nullptr;
        }
    }
}
