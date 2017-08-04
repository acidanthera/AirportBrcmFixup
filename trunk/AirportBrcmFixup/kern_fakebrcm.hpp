#ifndef FakeBrcm_h
#define FakeBrcm_h

#include <Library/LegacyIOService.h>

class FakeBrcm : public IOService
{
    OSDeclareDefaultStructors(FakeBrcm);
    typedef IOService super;


public:
    virtual bool init(OSDictionary *propTable) override;
    virtual IOService* probe(IOService * provider, SInt32 *score) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual void free() override;
    
    static IOService* getServiceProvider() { return service_provider; }
    static IOService* getService(const char* service_name);
    
protected:
    static IOService    *service_provider;
    static OSDictionary *service_dict;
};


#endif
