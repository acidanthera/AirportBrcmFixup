//
//  kern_brcmfx.hpp
//  AirportBrcmFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#ifndef kern_brcmfx_hpp
#define kern_brcmfx_hpp

#include "kern_misc.hpp"

#include <Headers/kern_patcher.hpp>
#include <stdatomic.h>

#include <IOKit/IOService.h>
#include <IOKit/IOWorkLoop.h>


class BRCMFX {
public:
	bool init();
	void deinit();

private:
	/**
	 *  Patch kernel
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processKernel(KernelPatcher &patcher);
	
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
	 * Execute postponed matching
	 */
	void startMatching();

	
	/**
	 *  Symbol types
	 */
	using IOCatalogue_startMatching_symbol = bool (*)(void *that, OSSymbol const* bundle_identifier);
	using IOCatalogue_startMatching_dictionary = bool (*)(void *that, OSDictionary *matching);
	using IOCatalogue_removeDrivers = bool (*)(void *that, OSDictionary *matching, bool doNubMatching);
	using IOPCIDevice_setASPMState = IOReturn (*)(void *pciDevice, IOService *service, IOOptionBits state);

	/**
	 *  Hooked methods / callbacks
	 */
	static bool             start(IOService *service, IOService* provider);
	static IOService*       probe(IOService *service, IOService * provider, SInt32 *score);
	static void             osl_panic(const char *format, ...);
	static const OSSymbol*  newVendorString(void);
	
	template <size_t index>
	static bool             checkBoardId(void *that, const char *boardID);
	static bool             wlc_wowl_enable(int64_t **a1);
	static bool             wowCapablePlatform(void *that);
	
	template <size_t index>
	static int64_t          wlc_set_countrycode_rev(int64_t a1, const char *country_code, int a3);
	static int64_t          wlc_set_countrycode_rev_4331(int64_t a1, int64_t a2, const char *country_code, int a4);
	
	template <size_t index>
	static int64_t          siPmuFvcoPllreg(uint32_t *a1, int64_t a2, int64_t a3);
	
#ifdef DEBUG
	template <size_t index>
	static IOReturn         AirPort_BrcmNIC_setTX_NSS(void *that, OSObject*, apple80211_tx_nss_data*);
	template <size_t index>
	static IOReturn         AirPort_BrcmNIC_getTX_NSS(void *that, OSObject*, apple80211_tx_nss_data*);
	template <size_t index>
	static IOReturn         AirPort_BrcmNIC_getNSS(void *that, OSObject*, apple80211_nss_data*);
	template <size_t index>
	static int64_t          wlc_ratespec_nss(int a1);
#endif

	/**
	 *  Trampolines for original method invocations
	 */
	mach_vm_address_t orgStart[MaxServices] {};
	mach_vm_address_t orgProbe[MaxServices] {};
	mach_vm_address_t orgWlcSetCountryCodeRev[MaxServices] {};
	mach_vm_address_t orgSiPmuFvcoPllreg[MaxServices] {};
	
#ifdef DEBUG
	mach_vm_address_t orgAirPort_BrcmNIC_setTX_NSS[MaxServices] {};
	mach_vm_address_t orgAirPort_BrcmNIC_getTX_NSS[MaxServices] {};
	mach_vm_address_t orgAirPort_BrcmNIC_getNSS[MaxServices] {};
	mach_vm_address_t orgWlcRatespecNss[MaxServices] {};
#endif
	
	// access to IOCatalogue methods
	IOCatalogue_startMatching_symbol startMatching_symbol {};
	IOCatalogue_startMatching_dictionary startMatching_dictionary {};
	IOCatalogue_removeDrivers removeDrivers {};
	IOPCIDevice_setASPMState  setASPMState {};
	
	const char** cpmChanSwitchWhitelist {};
	_Atomic(bool) atLeastOneServiceStarted = false;
	IOWorkLoop *workLoop {};
	int matchingLeftAttemptCounter {9};
	IOTimerEventSource *matchingTimer {};
};

#endif /* kern_brcmfx_hpp */
