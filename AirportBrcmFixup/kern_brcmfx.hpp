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

#include "kern_misc.hpp"


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
	 *  Hooked methods / callbacks
	 */
	static bool             start(IOService *service, IOService* provider);
	static IOService*       probe(IOService *service, IOService * provider, SInt32 *score);
	static void             osl_panic(const char *format, ...);
	static const OSSymbol*  newVendorString(void);
	static bool             checkBoardId(const char *boardID);
	static bool             wlc_wowl_enable(int64_t **a1);
	
	static int64_t          wlc_set_countrycode_rev0(int64_t a1, const char *country_code, int a3);
	static int64_t          wlc_set_countrycode_rev1(int64_t a1, const char *country_code, int a3);
	static int64_t          wlc_set_countrycode_rev2(int64_t a1, const char *country_code, int a3);
	
	static int32_t 			siPmuFvcoPllreg0(uint32_t *a1, int64_t a2, int64_t a3);
	static int32_t 			siPmuFvcoPllreg1(uint32_t *a1, int64_t a2, int64_t a3);
	static int32_t 			siPmuFvcoPllreg2(uint32_t *a1, int64_t a2, int64_t a3);


	/**
	 *  Trampolines for original method invocations
	 */
	mach_vm_address_t orgStart[kextListSize] {};
	mach_vm_address_t orgProbe[kextListSize] {};
	mach_vm_address_t orgWlcSetCountryCodeRev[kextListSize] {};
	mach_vm_address_t orgSiPmuFvcoPllreg[kextListSize] {};
	char provider_country_code[5] {""};
};

#endif /* kern_brcmfx_hpp */
