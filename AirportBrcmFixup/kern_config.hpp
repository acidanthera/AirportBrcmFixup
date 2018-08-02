//
//  kern_config.hpp
//  AirportBrcmFixup
//
//  Copyright © 2016-2017 lvs1974. All rights reserved.
//

#ifndef kern_config_private_h
#define kern_config_private_h

#include <Headers/kern_util.hpp>


class Configuration {
public:
	/**
	 *  Possible boot arguments
	 */
	static const char *bootargOff[];
	static const char *bootargDebug[];
	static const char *bootargBeta[];
	static constexpr const char *bootargBrcmCountry {"brcmfx-country"};     // 5 Ghz patch - change default country
	static constexpr const char *bootargWlanMsgLevel {"wl_msg_level"};      // message level for wlan kexts
	static constexpr const char *bootargWlanMsgLevel2 {"wl_msg_level2"};    // message level for wlan kexts
	static constexpr const char *bootargBrcmEnableWowl {"-brcmfxwowl"};     // enable WOWL
	
	/**
	 *  Retrieve boot arguments
	 *
	 *  @return true if allowed to continue
	 */
	void readArguments();

	char country_code[4] {"US"};
	bool country_code_overrided {false};
	
	bool disabled           {false};
	bool enable_wowl        {false};
	
	int32_t wl_msg_level    {0};
	int32_t wl_msg_level2   {0};

	Configuration() = default;
};

extern Configuration ADDPR(brcmfx_config);

#endif /* kern_config_private_h */
