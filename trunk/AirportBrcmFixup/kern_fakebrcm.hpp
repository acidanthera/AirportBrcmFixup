#ifndef FakeBrcm_h
#define FakeBrcm_h

#include <Library/LegacyIOService.h>

class FakeBrcm : public IOService
{
    OSDeclareDefaultStructors(FakeBrcm);
    typedef IOService super;


public:
    virtual bool init(OSDictionary *propTable) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual void free() override;
};


#endif
