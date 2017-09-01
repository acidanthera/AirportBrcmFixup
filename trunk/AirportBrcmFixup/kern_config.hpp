//
//  kern_config.hpp
//  AirportBrcmFixup
//
//  Copyright © 2016-2017 lvs1974. All rights reserved.
//

#ifndef kern_config_private_h
#define kern_config_private_h


class Configuration {
public:
	/**
	 *  Possible boot arguments
	 */
    static const char *bootargOff[];
    static const char *bootargDebug[];
    static const char *bootargBeta[];
    static constexpr const char *bootargBrcmCountry   {"brcmfx-country"};   // 5 Ghz patch - change default country
    static constexpr const char *bootargWlanMsgLevel  {"wl_msg_level"};    // message level for wlan kexts
    static constexpr const char *bootargWlanMsgLevel2 {"wl_msg_level2"};    // message level for wlan kexts
    
public:
	/**
	 *  Retrieve boot arguments
	 *
	 *  @return true if allowed to continue
	 */
	void readArguments();
    
    
    char country_code[4] {"US"};
    bool disabled {false};
    int32_t wl_msg_level {0};
    int32_t wl_msg_level2 {0};
	
#ifdef DEBUG
	static constexpr const char *fullName {xStringify(PRODUCT_NAME) " Kernel Extension " xStringify(MODULE_VERSION) " DEBUG build"};
#else
	static constexpr const char *fullName {xStringify(PRODUCT_NAME) " Kernel Extension " xStringify(MODULE_VERSION)};
#endif
	
    Configuration() = default;
};

extern Configuration config;

#endif /* kern_config_private_h */
