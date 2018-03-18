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
	
private:
    /**
     *  configRead16 func type
     */
    using t_config_read16 = UInt16 (*)(IOService *, UInt32, UInt8);
    using t_config_read32 = UInt32 (*)(IOService *, UInt32, UInt8);
	
	static bool 			isServiceSupported(IOService* service);
    static void 			hookProvider(IOService* provider);
    static UInt16           configRead16(IOService *that, UInt32 bits, UInt8 offset);
    static UInt32           configRead32(IOService *that, UInt32 bits, UInt8 offset);
    
private:
    static IOService        *service_provider;
    static bool     		service_found;
    static t_config_read16  orgConfigRead16;
    static t_config_read32  orgConfigRead32;
};


#endif
