#ifndef FakeBrcm_h
#define FakeBrcm_h

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>

class FakeBrcm : public IOService
{
    OSDeclareDefaultStructors(FakeBrcm);
    typedef IOService super;

protected:
    IOPCIDevice* mProvider;
    static bool disable_driver;

public:
    virtual bool init(OSDictionary *propTable) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual void free() override;
    
public:
    static void set_disable_driver();
};


#endif
