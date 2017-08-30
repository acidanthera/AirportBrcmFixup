#include <Library/LegacyIOService.h>
#include <Headers/kern_api.hpp>

#include "kern_config.hpp"
#include "kern_fakebrcm.hpp"
#include "kern_misc.hpp"

OSDefineMetaClassAndStructors(FakeBrcm, IOService);

IOService *FakeBrcm::service_provider {nullptr};
OSDictionary *FakeBrcm::service_dict {nullptr};
OSDictionary *FakeBrcm::prop_table {nullptr};

//==============================================================================

bool FakeBrcm::init(OSDictionary *propTable)
{
    if (config.disabled)
    {
        DBGLOG("BRCMFX @ FakeBrcm::init(): FakeBrcm disabled");
        return false;
    }
    
    DBGLOG("BRCMFX @ FakeBrcm::init()");

    bool ret = super::init(propTable);
    if (!ret)
    {
        SYSLOG("BRCMFX @ FakeBrcm super::init returned false\n");
        return false;
    }
    
    service_dict = OSDictionary::withCapacity(1);
    
    if (propTable != nullptr)
        prop_table = OSDictionary::withDictionary(propTable);
    
    return true;
}

//==============================================================================

IOService* FakeBrcm::probe(IOService * provider, SInt32 *score)
{
    DBGLOG("BRCMFX @ FakeBrcm::probe()");
    
    IOService* ret = super::probe(provider, score);
    if (!ret)
    {
        SYSLOG("BRCMFX @ FakeBrcm super::probe returned nullptr\n");
        return nullptr;
    }
    
    if (config.disabled)
    {
        DBGLOG("BRCMFX @ FakeBrcm::probe(): FakeBrcm disabled");
        *score = -2000;
        return ret;
    }
    
    service_provider = provider;
    
    for (int i=0; i<kextListSize; i++)
    {
        const OSMetaClass * meta_class = OSMetaClass::getMetaClassWithName(OSSymbol::withCStringNoCopy(serviceNameList[i]));
        if (meta_class != nullptr)
        {
            DBGLOG("BRCMFX @ FakeBrcm::probe(): meta class for %s exists!", serviceNameList[i]);
            
            IOService *service = (IOService *) OSMetaClass::allocClassWithName(serviceNameList[i]);
            if (service != nullptr)
            {
                service->retain();
                DBGLOG("BRCMFX @ FakeBrcm: retain counter for driver %s is %d", serviceNameList[i], service->getRetainCount());
                service_dict->setObject(serviceNameList[i], service);
            }
            else
                SYSLOG("BRCMFX @ FakeBrcm: instance of driver %s couldn't be created", serviceNameList[i]);
        }
    }
    
    if (service_dict->getCount() != 0)
    {
        DBGLOG("BRCMFX @ FakeBrcm::probe() will change score from %d to 1300", *score);
        *score = 2000;  // change probe score to be the first in the list
    }
    else
    {
        DBGLOG("BRCMFX @ FakeBrcm::probe(): fallback to original driver");
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
        DBGLOG("BRCMFX @ FakeBrcm::start(): FakeBrcm disabled");
        return false;
    }
    
    DBGLOG("BRCMFX @ FakeBrcm::start()");
    
    if (!super::start(provider))
    {
        SYSLOG("BRCMFX @ FakeBrcm super::start returned false\n");
        return false;
    }
    
    if (!service_dict->getCount())
    {
        SYSLOG("BRCMFX @ FakeBrcm::start(): fallback to original driver");
        return false;
    }

    return true;
}

//==============================================================================

void FakeBrcm::stop(IOService *provider)
{
    DBGLOG("BRCMFX @ FakeBrcm::stop()");

    super::stop(provider);
}

//==============================================================================

void FakeBrcm::free()
{
    DBGLOG("BRCMFX @ FakeBrcm::free()");
    
    OSSafeReleaseNULL(service_dict);

    super::free();
}

//==============================================================================

IOService* FakeBrcm::getService(const char* service_name)
{
    if (service_dict == nullptr)
    {
        DBGLOG("BRCMFX @ FakeBrcm::getService(): no dictionary");
        return nullptr;
    }
    
    OSObject* object = service_dict->getObject(service_name);
    if (object == nullptr)
    {
        DBGLOG("BRCMFX @ FakeBrcm::getService(): service %s is not found in dictionary", service_name);
        return nullptr;
    }
    
    IOService* service = OSDynamicCast(IOService, object);
    if (service == nullptr)
    {
        DBGLOG("BRCMFX @ FakeBrcm::getService(): object for service %s can't be casted to IOService", service_name);
        return nullptr;
    }
    
    return service;
}


