//
//  kern_brcmfx.hpp
//  HBFX
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#ifndef kern_brcmfx_hpp
#define kern_brcmfx_hpp

#include <Headers/kern_patcher.hpp>
#include <IOKit/pci/IOPCIDevice.h>

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
     *  start func type
     */
    using t_start = bool (*)(IOService *, IOService *);
    
    /**
     *  configRead16 func type
     */
    using t_config_read16 = UInt16 (*)(IOPCIDevice *, IOPCIAddressSpace, UInt8);
    
    /**
     *  getPCIProperty func type
     */
    //using t_get_pci_property = bool (*) (IOService*, char const*, unsigned int&);
    
	/**
	 *  Hooked methods / callbacks
	 */
    static const OSSymbol*  newVendorString(void);
    static bool             checkBoardId(const char *boardID);
    static IOService*       probe(IOService* provider, SInt32* score);
    static bool             start(IOService *service, IOService* provider);
    static UInt16           configRead16(IOPCIDevice *that, IOPCIAddressSpace bits, UInt8 offset);
    //static bool             getPCIProperty(IOService* service, char const* name, unsigned int& value);

    
    /**
     *  Trampolines for original method invocations
     */
    t_start                 orgStart                {nullptr};
    t_config_read16         orgConfigRead16         {nullptr};
    //t_get_pci_property      orgGetPCIProperty       {nullptr};

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
    
    struct VMTOffset {
        enum {
            configRead16  = 0x860/8,
            configWrite16 = 0x868/8,
            configRead32  = 0x850/8,
            configWrite32 = 0x858/8,
            configRead8   = 0x870/8,
            configWrite8  = 0x878/8,
        };
    };
};

#endif /* kern_hbfx_hpp */
