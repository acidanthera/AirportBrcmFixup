//
//  kern_brcmfx.hpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#ifndef kern_brcmfx_hpp
#define kern_brcmfx_hpp

#include <Headers/kern_patcher.hpp>
#include <Library/LegacyIOService.h>

class BRCMFX {
public:
	bool init();
	void deinit();

private:

	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  start func type
	 */
	using t_start = bool (*)(IOService *, IOService *);

	/**
	 *  wlc_set_countrycode_rev func type
	 */
	using t_wlc_set_countrycode_rev = int64_t (*) (int64_t a1, const char *country_code, int a3);


	/**
	 *  Hooked methods / callbacks
	 */
	static bool             start(IOService *service, IOService* provider);
	static bool             start_mfg(IOService *service, IOService* provider);
	static IOService*       probe_mfg(IOService *service, IOService * provider, SInt32 *score);
	static void             osl_panic(const char *format, ...);
	static const OSSymbol*  newVendorString(void);
	static int32_t          siPmuFvcoPllreg(uint32_t *a1, int64_t a2, int64_t a3);
	static bool             checkBoardId(const char *boardID);
	static int64_t          wlc_set_countrycode_rev(int64_t a1, const char *country_code, int a3);
	static bool             wlc_wowl_enable(int64_t **a1);


	/**
	 *  Trampolines for original method invocations
	 */
	mach_vm_address_t orgStart {};
	mach_vm_address_t orgWlcSetCountryCodeRev {};
	mach_vm_address_t orgSiPmuFvcoPllreg {};
	int32_t *wl_msg_level {nullptr};
	int32_t *wl_msg_level2 {nullptr};
	char provider_country_code[5] {""};
};

#endif /* kern_brcmfx_hpp */
