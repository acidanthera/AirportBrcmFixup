//
//  kern_hbfx.hpp
//  HBFX
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#ifndef kern_brcmfx_hpp
#define kern_brcmfx_hpp

#include <Headers/kern_patcher.hpp>
#include <IOKit/IOService.h>


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
     *  Find service by name in a specified registry plane (gIO80211FamilyPlane or gIOServicePlane)
     */
    IOService* findService(const IORegistryPlane* plane, const char *service_name);
    
    /**
     *  Try to start a service
     */
    bool startService(IOService* service);
    
	/**
	 *  newVendorString callback type
	 */
    using t_new_vendorstring = const OSSymbol* (*)(void);
    
    /**
     *  board-id check func type
     */
    using t_check_board_Id = bool (*)(const char *);

    
	/**
	 *  Hooked methods / callbacks
	 */
    static const OSSymbol*  newVendorString(void);
    static bool             checkBoardId(const char *boardID);

    
    /**
     *  Trampolines for original method invocations
     */
    t_new_vendorstring      orgNewVendorString      {nullptr};
    t_check_board_Id        orgCheckBoardId         {nullptr};

public:
    /**
     *  Current progress mask
     */
    struct ProcessingState {
        enum {
            NothingReady       = 0,
            BRCM4360Patched    = 1,
            BRCMNICPatched     = 2,
            BRCMNIC_MFGPatched = 4,
            EverythingDone      = BRCM4360Patched | BRCMNICPatched | BRCMNIC_MFGPatched
        };
    };
    int progressState {ProcessingState::NothingReady};
};

#endif /* kern_hbfx_hpp */
