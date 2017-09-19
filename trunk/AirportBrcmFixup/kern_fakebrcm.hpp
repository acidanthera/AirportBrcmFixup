#ifndef FakeBrcm_h
#define FakeBrcm_h

#include <Library/LegacyIOService.h>


class EXPORT FakeBrcm : public IOService
{
    OSDeclareDefaultStructors(FakeBrcm);
    typedef IOService super;


public:
    virtual bool init(OSDictionary *propTable) override;
    virtual bool attach(IOService *provider) override;
    virtual IOService* probe(IOService * provider, SInt32 *score) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual void free() override;
    
    static IOService*    getServiceProvider() { return service_provider; }
    static IOService*    getService(const char* service_name);
    static OSDictionary* getPropTable() { return prop_table; }
    
    static void hookProvider(IOService* provider);
    static void unhookProvider();
    

private:
    /**
     *  configRead16 func type
     */
    using t_config_read16 = UInt16 (*)(IOService *, UInt32, UInt8);
    using t_config_read32 = UInt32 (*)(IOService *, UInt32, UInt8);
    
    static UInt16           configRead16(IOService *that, UInt32 bits, UInt8 offset);
    static UInt32           configRead32(IOService *that, UInt32 bits, UInt8 offset);
    
private:
    static IOService        *service_provider;
    static OSDictionary     *service_dict;
    static OSDictionary     *prop_table;
    static t_config_read16  orgConfigRead16;
    static t_config_read32  orgConfigRead32;
};


#endif
