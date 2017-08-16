#include <Library/LegacyIOService.h>
#include <Headers/kern_api.hpp>

#include "kern_config.hpp"
#include "kern_fakebrcm.hpp"
#include "kern_misc.hpp"

OSDefineMetaClassAndStructors(FakeBrcm, IOService);

IOService *FakeBrcm::service_provider {nullptr};
OSDictionary *FakeBrcm::service_dict {nullptr};

//==============================================================================

bool FakeBrcm::init(OSDictionary *propTable)
{
    if (config.disabled)
        return false;
    
    DBGLOG("BRCMFX @ FakeBrcm::init()");

    bool ret = super::init(propTable);
    if (!ret)
    {
        SYSLOG("BRCMFX @ FakeBrcm super::init returned false\n");
        return false;
    }
    
    service_dict = OSDictionary::withCapacity(1);
    
    return true;
}

//==============================================================================

IOService* FakeBrcm::probe(IOService * provider, SInt32 *score)
{
    if (config.disabled)
        return nullptr;
    
    DBGLOG("BRCMFX @ FakeBrcm::probe()");
    
    IOService* ret = super::probe(provider, score);
    if (!ret)
    {
        SYSLOG("BRCMFX @ FakeBrcm super::probe returned nullptr\n");
        return nullptr;
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
#ifdef DEBUG
        else
        {
            DBGLOG("BRCMFX @ FakeBrcm::probe(): meta class for %s is null", serviceNameList[i]);
            IOService *service = (IOService *) OSMetaClass::allocClassWithName(serviceNameList[i]);
            if (service == nullptr)
            {
                DBGLOG("BRCMFX @ FakeBrcm::probe(): instance for driver %s can't be created", serviceNameList[i]);
            }
        }
#endif
    }
    
    if (!service_dict->getCount())
    {
        SYSLOG("BRCMFX @ FakeBrcm::probe() will return nullptr to fallback to original driver");
        return nullptr;
    }

    DBGLOG("BRCMFX @ FakeBrcm::probe() will change score from %d to 1300", *score);
    *score = 1300;  // change probe score to be the first in the list

    return ret;
}

//==============================================================================

bool FakeBrcm::start(IOService *provider)
{
    if (config.disabled)
        return false;
    
    DBGLOG("BRCMFX @ FakeBrcm::start()");
    
    if (!super::start(provider))
    {
        SYSLOG("BRCMFX @ FakeBrcm super::start returned false\n");
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


