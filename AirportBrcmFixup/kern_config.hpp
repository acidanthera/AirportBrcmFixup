//
//  kern_config.hpp
//  AirportBrcmFixup
//
//  Copyright © 2016-2017 lvs1974. All rights reserved.
//

#ifndef kern_config_private_h
#define kern_config_private_h

#include <Headers/kern_util.hpp>
#include <IOKit/IOService.h>


class Configuration {
public:
	/**
	 *  Possible boot arguments
	 */
	static constexpr const char *bootargOff            {"-brcmfxoff"};
	static constexpr const char *bootargDebug          {"-brcmfxdbg"};
	static constexpr const char *bootargBeta           {"-brcmfxbeta"};
	static constexpr const char *bootargBrcmEnableWowl {"-brcmfx-wowl"};        // enable WOWL
	static constexpr const char *bootargBrcmAllDrv     {"-brcmfx-alldrv"};	   // skip driver availability check (which drivers available in the current system)
	static constexpr const char *bootargBrcmCountry    {"brcmfx-country"};     // 5 Ghz patch - change default country
	static constexpr const char *bootargBrcmDriver     {"brcmfx-driver"};
	static constexpr const char *bootargBrcmAspm       {"brcmfx-aspm"};

	/**
	 *  Retrieve boot arguments
	 *
	 *  @return true if allowed to continue
	 */
	void readArguments(IOService* provider = nullptr);
	bool awaitPublishing(IORegistryEntry *obj);

	char country_code[5]        {"US"};
	
	bool disabled               {false};
	bool enable_wowl            {false};

	int32_t brcmfx_driver       {-1};

	IOOptionBits brcmfx_aspm    {0};
	bool override_aspm          {false};
	
	bool enable_all_drv         {false};
	
	bool config_is_ready        {false};

	Configuration() = default;
};

extern Configuration ADDPR(brcmfx_config);

#endif /* kern_config_private_h */
