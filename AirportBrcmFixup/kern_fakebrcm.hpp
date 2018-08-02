//
//  kern_fakebrcm.hpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#ifndef FakeBrcm_h
#define FakeBrcm_h

#include <Library/LegacyIOService.h>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_config.hpp>

class EXPORT FakeBrcm : public IOService
{
	OSDeclareDefaultStructors(FakeBrcm);
	typedef IOService super;

	static bool             isServiceSupported(IORegistryEntry* service);
	static void             hookProvider(IOService* provider);
	static UInt16           configRead16(IORegistryEntry *service, UInt32 bits, UInt8 offset);
	static UInt32           configRead32(IORegistryEntry *service, UInt32 bits, UInt8 offset);

	static IOService                    *service_provider;
	static bool                         service_found;
	static WIOKit::t_PCIConfigRead16    orgConfigRead16;
	static WIOKit::t_PCIConfigRead32    orgConfigRead32;

public:
	virtual bool init(OSDictionary *propTable) override;
	virtual bool attach(IOService *provider) override;
	virtual IOService* probe(IOService * provider, SInt32 *score) override;
	virtual bool start(IOService *provider) override;
	virtual void stop(IOService *provider) override;
	virtual void free() override;
};

#endif
