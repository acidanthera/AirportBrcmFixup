#include <IOKit/IOLib.h>
#include <Headers/kern_api.hpp>
#include "kern_config.hpp"
#include "kern_fakebrcm.hpp"

OSDefineMetaClassAndStructors(FakeBrcm, IOService);

bool FakeBrcm::init(OSDictionary *propTable)
{
    if (config.disabled)
        return false;
    
    DBGLOG("BRCMFX @ FakeBrcm::init()");

    bool ret = super::init(propTable);
    if (!ret)
    {
        SYSLOG("super::init returned false\n");
        return false;
    }
    
    return true;
}

bool FakeBrcm::start(IOService *provider)
{
    if (config.disabled)
        return false;
    
    DBGLOG("BRCMFX @ FakeBrcm::start()");
    
    if (!super::start(provider))
    {
        SYSLOG("super::start returned false\n");
        return false;
    }

    return true;
}

void FakeBrcm::stop(IOService *provider)
{
    DBGLOG("BRCMFX @ FakeBrcm::stop()");

    super::stop(provider);
}

void FakeBrcm::free()
{
    DBGLOG("BRCMFX @ FakeBrcm::free()");

    super::free();
}



