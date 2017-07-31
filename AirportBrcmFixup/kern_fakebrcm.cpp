#include <IOKit/IOLib.h>
#include <Headers/kern_api.hpp>
#include "kern_fakebrcm.hpp"

OSDefineMetaClassAndStructors(FakeBrcm, IOService);

bool FakeBrcm::disable_driver = false;

bool FakeBrcm::init(OSDictionary *propTable)
{
    DBGLOG("BRCMFX @ FakeBrcm::init() %p\n", this);
    
    if (disable_driver)
        return false;

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
    if (disable_driver)
        return false;
    
    DBGLOG("BRCMFX @ FakeBrcm::start() %p\n", this);
    
    if (!super::start(provider))
    {
        SYSLOG("super::start returned false\n");
        return false;
    }

    return true;
}

void FakeBrcm::stop(IOService *provider)
{
    DBGLOG("BRCMFX @ FakeBrcm::stop() %p\n", this);

    super::stop(provider);
}

void FakeBrcm::free()
{
    DBGLOG("BRCMFX @ FakeBrcm::free() %p\n", this);

    super::free();
}

void FakeBrcm::set_disable_driver()
{
    disable_driver = true;
}


