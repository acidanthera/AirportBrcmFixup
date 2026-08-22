#!/usr/bin/env python3
from pathlib import Path
import shutil
import sys

ROOT = Path.cwd()
BASE = ROOT / "AirportBrcmFixup"

files = {
    "start": BASE / "kern_start.cpp",
    "hpp": BASE / "kern_brcmfx.hpp",
    "cpp": BASE / "kern_brcmfx.cpp",
}

missing = [str(p) for p in files.values() if not p.is_file()]
if missing:
    print("ERROR: Run this from the AirportBrcmFixup repository root.")
    print("Expected these files:")
    for p in missing:
        print("  ", p)
    sys.exit(1)

original = {k: p.read_text() for k, p in files.items()}
modified = dict(original)

def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{label}: expected exactly 1 matching source block, found {count}. "
            "No files have been changed."
        )
    return text.replace(old, new, 1)

# ---------------------------------------------------------------------------
# 1) Automatic ASPM policy in kern_start.cpp
# ---------------------------------------------------------------------------
if "auto ASPM policy = preserve firmware" not in modified["start"]:
    old = '''\
\t\tif (PE_parse_boot_argn(bootargBrcmAspm, &brcmfx_aspm, sizeof(brcmfx_aspm))) {
\t\t\tDBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargBrcmAspm, brcmfx_aspm);
\t\t\toverride_aspm = true;
\t\t} else if (WIOKit::getOSDataValue(provider, bootargBrcmAspm, brcmfx_aspm)) {
\t\t\tDBGLOG("BRCMFX", "%s in ioreg is set to %d", bootargBrcmAspm, brcmfx_aspm);
\t\t\toverride_aspm = true;
\t\t}
'''
    new = '''\
\t\tbool userAspmOverride = false;

\t\tif (PE_parse_boot_argn(bootargBrcmAspm, &brcmfx_aspm, sizeof(brcmfx_aspm))) {
\t\t\tDBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargBrcmAspm, brcmfx_aspm);
\t\t\toverride_aspm = true;
\t\t\tuserAspmOverride = true;
\t\t} else if (WIOKit::getOSDataValue(provider, bootargBrcmAspm, brcmfx_aspm)) {
\t\t\tDBGLOG("BRCMFX", "%s in ioreg is set to %d", bootargBrcmAspm, brcmfx_aspm);
\t\t\toverride_aspm = true;
\t\t\tuserAspmOverride = true;
\t\t}
'''
    modified["start"] = replace_once(
        modified["start"], old, new, "ASPM boot-argument block"
    )

    old = '''\
\t\tif (PE_parse_boot_argn(bootargDelay, &start_delay, sizeof(start_delay))) {
\t\t\tDBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargDelay, start_delay);
\t\t} else if (WIOKit::getOSDataValue(provider, bootargDelay, start_delay)) {
\t\t\tDBGLOG("BRCMFX", "%s in ioreg is set to %d", bootargDelay, start_delay);
\t\t}
\t\tif (brcmfx_aspm != 0xFF)
\t\t{
\t\t\tuint16_t vendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigVendorID);
\t\t\tuint16_t deviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigDeviceID);
\t\t\tuint16_t subSystemVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemVendorID);
\t\t\tbool     bcm4350  = (vendorID == 0x14e4 && deviceID == 0x43a3 && subSystemVendorID != 0x106b);
'''
    new = '''\
\t\tif (PE_parse_boot_argn(bootargDelay, &start_delay, sizeof(start_delay))) {
\t\t\tDBGLOG("BRCMFX", "%s in boot-arg is set to %d", bootargDelay, start_delay);
\t\t} else if (WIOKit::getOSDataValue(provider, bootargDelay, start_delay)) {
\t\t\tDBGLOG("BRCMFX", "%s in ioreg is set to %d", bootargDelay, start_delay);
\t\t}

\t\tuint16_t vendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigVendorID);
\t\tuint16_t deviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigDeviceID);
\t\tuint16_t subSystemVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemVendorID);
\t\tuint16_t subSystemID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemID);

\t\t/*
\t\t * Automatic BCM4360/BCM94360 ASPM policy.
\t\t *
\t\t * Explicit brcmfx-aspm (boot-arg or IORegistry property) always wins.
\t\t *
\t\t * 0xFF is AirportBrcmFixup's existing sentinel meaning:
\t\t * preserve the platform/firmware ASPM configuration.
\t\t *
\t\t * 0x102 (258) = L1 (0x2) + CLKREQ (0x100).
\t\t */
\t\tif (!userAspmOverride && vendorID == 0x14e4 && deviceID == 0x43a0) {
\t\t\tif (subSystemVendorID == 0x106b &&
\t\t\t\t(subSystemID == 0x0117 ||   // BCM94360CS2 / Fenvi BCM94360NG identity
\t\t\t\t subSystemID == 0x0111 ||   // BCM94360CD identity
\t\t\t\t subSystemID == 0x0134)) {  // BCM94360CS identity

\t\t\t\tbrcmfx_aspm = 0xFF;
\t\t\t\toverride_aspm = false;
\t\t\t\tDBGLOG("BRCMFX",
\t\t\t\t\t"Configuration::readArguments: auto ASPM policy = preserve firmware, "
\t\t\t\t\t"device = %04x:%04x subsystem = %04x:%04x",
\t\t\t\t\tvendorID, deviceID, subSystemVendorID, subSystemID);

\t\t\t} else if (subSystemVendorID == 0x1043 && subSystemID == 0x8659) {
\t\t\t\tbrcmfx_aspm = 0x102; // 258 = L1 + CLKREQ
\t\t\t\toverride_aspm = true;
\t\t\t\tDBGLOG("BRCMFX",
\t\t\t\t\t"Configuration::readArguments: auto ASPM policy = L1+CLKREQ (258), "
\t\t\t\t\t"device = %04x:%04x subsystem = %04x:%04x",
\t\t\t\t\tvendorID, deviceID, subSystemVendorID, subSystemID);
\t\t\t}
\t\t}

\t\tif (brcmfx_aspm != 0xFF)
\t\t{
\t\t\tbool     bcm4350  = (vendorID == 0x14e4 && deviceID == 0x43a3 && subSystemVendorID != 0x106b);
'''
    modified["start"] = replace_once(
        modified["start"], old, new, "ASPM policy insertion block"
    )

    old = '''\
\t\t\t\tDBGLOG("BRCMFX", "Configuration::readArguments: override aspm, subsystem-vendor-id = 0x%04x, subsystem-id = 0x%04x",
\t\t\t\t\t   subSystemVendorID, pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemID));
'''
    new = '''\
\t\t\t\tDBGLOG("BRCMFX", "Configuration::readArguments: override aspm, subsystem-vendor-id = 0x%04x, subsystem-id = 0x%04x",
\t\t\t\t\t   subSystemVendorID, subSystemID);
'''
    modified["start"] = replace_once(
        modified["start"], old, new, "ASPM subsystem logging block"
    )

# ---------------------------------------------------------------------------
# 2) NSS:2 declaration/trampoline in kern_brcmfx.hpp
# ---------------------------------------------------------------------------
if "wlc_stf_txchain_set(void *wlc" not in modified["hpp"]:
    old = '''\
\ttemplate <size_t index>
\tstatic int64_t          siPmuFvcoPllreg(uint32_t *a1, int64_t a2, int64_t a3);

#ifdef DEBUG
'''
    new = '''\
\ttemplate <size_t index>
\tstatic int64_t          siPmuFvcoPllreg(uint32_t *a1, int64_t a2, int64_t a3);

\t// Correct the cold-boot NSS:1 txchain constraint without faking NSS reporting.
\tstatic uint64_t         wlc_stf_txchain_set(void *wlc, uint64_t chain, uint64_t arg3, uint64_t reason);

#ifdef DEBUG
'''
    modified["hpp"] = replace_once(
        modified["hpp"], old, new, "NSS2 declaration block"
    )

if "orgWlcStfTxchainSet" not in modified["hpp"]:
    old = '''\
\tmach_vm_address_t orgProbe[MaxServices] {};
\tmach_vm_address_t orgWlcSetCountryCodeRev[MaxServices] {};
\tmach_vm_address_t orgSiPmuFvcoPllreg[MaxServices] {};

#ifdef DEBUG
'''
    new = '''\
\tmach_vm_address_t orgProbe[MaxServices] {};
\tmach_vm_address_t orgWlcSetCountryCodeRev[MaxServices] {};
\tmach_vm_address_t orgSiPmuFvcoPllreg[MaxServices] {};
\tmach_vm_address_t orgWlcStfTxchainSet {};

#ifdef DEBUG
'''
    modified["hpp"] = replace_once(
        modified["hpp"], old, new, "NSS2 trampoline block"
    )

# ---------------------------------------------------------------------------
# 3) NSS:2 hook and routing in kern_brcmfx.cpp
# ---------------------------------------------------------------------------
if "uint64_t BRCMFX::wlc_stf_txchain_set" not in modified["cpp"]:
    old = '''\
\treturn ret;
}
#ifdef DEBUG
//==============================================================================
'''
    new = '''\
\treturn ret;
}

//==============================================================================
// Fix the specific cold-boot path that disables the second TX chain.
// reason == 2, chain low byte == 1 -> preserve upper bits and use chain mask 3.
uint64_t BRCMFX::wlc_stf_txchain_set(void *wlc, uint64_t chain, uint64_t arg3, uint64_t reason)
{
\tif (reason == 2 && (chain & 0xFF) == 1)
\t\tchain = (chain & ~0xFFULL) | 0x3;

\treturn FunctionCast(
\t\twlc_stf_txchain_set,
\t\tcallbackBRCMFX->orgWlcStfTxchainSet
\t)(wlc, chain, arg3, reason);
}

#ifdef DEBUG
//==============================================================================
'''
    modified["cpp"] = replace_once(
        modified["cpp"], old, new, "NSS2 hook implementation block"
    )

if "NSS2 txchain fix failed" not in modified["cpp"]:
    old = '''\
\t\t\t\tif (!patcher.routeMultiple(index, requests, address, size))
\t\t\t\t\tSYSLOG("BRCMFX", "at least one basic patch is failed, error = %d", patcher.getError());
\t\t\t\telse
\t\t\t\t\tDBGLOG("BRCMFX", "all patches are successfuly applied to %s", idList[i]);

\t\t\t\tif ((ADDPR(brcmfx_config).brcmfx_driver == -1 && i == AirPort_BrcmNIC_MFG) ||
'''
    new = '''\
\t\t\t\tif (!patcher.routeMultiple(index, requests, address, size))
\t\t\t\t\tSYSLOG("BRCMFX", "at least one basic patch is failed, error = %d", patcher.getError());
\t\t\t\telse
\t\t\t\t\tDBGLOG("BRCMFX", "all patches are successfuly applied to %s", idList[i]);

\t\t\t\t// Production NSS:2 fix for AirPort_BrcmNIC.
\t\t\t\t// Route directly so this remains active in RELEASE builds.
\t\t\t\tif (i == AirPort_BrcmNIC) {
\t\t\t\t\tpatcher.clearError();

\t\t\t\t\tKernelPatcher::RouteRequest nss2Fix[] {
\t\t\t\t\t\t{
\t\t\t\t\t\t\t"_wlc_stf_txchain_set",
\t\t\t\t\t\t\treinterpret_cast<mach_vm_address_t>(BRCMFX::wlc_stf_txchain_set),
\t\t\t\t\t\t\torgWlcStfTxchainSet
\t\t\t\t\t\t}
\t\t\t\t\t};

\t\t\t\t\tif (!patcher.routeMultiple(index, nss2Fix, address, size))
\t\t\t\t\t\tSYSLOG("BRCMFX", "NSS2 txchain fix failed, error = %d", patcher.getError());

\t\t\t\t\tpatcher.clearError();
\t\t\t\t}

\t\t\t\tif ((ADDPR(brcmfx_config).brcmfx_driver == -1 && i == AirPort_BrcmNIC_MFG) ||
'''
    modified["cpp"] = replace_once(
        modified["cpp"], old, new, "NSS2 route block"
    )

# ---------------------------------------------------------------------------
# Final validation before writing anything
# ---------------------------------------------------------------------------
checks = [
    ("start", "auto ASPM policy = preserve firmware"),
    ("start", "auto ASPM policy = L1+CLKREQ (258)"),
    ("hpp", "wlc_stf_txchain_set(void *wlc"),
    ("hpp", "orgWlcStfTxchainSet"),
    ("cpp", "uint64_t BRCMFX::wlc_stf_txchain_set"),
    ("cpp", "reason == 2 && (chain & 0xFF) == 1"),
    ("cpp", '"_wlc_stf_txchain_set"'),
    ("cpp", "NSS2 txchain fix failed"),
]
for key, needle in checks:
    if needle not in modified[key]:
        raise RuntimeError(f"Internal validation failed: {needle}")

if all(modified[k] == original[k] for k in files):
    print("Nothing to do: both the NSS:2 and automatic ASPM modifications already appear to be present.")
    sys.exit(0)

# Only now make backups and write.
for key, path in files.items():
    if modified[key] != original[key]:
        backup = path.with_suffix(path.suffix + ".pre-nss2-aspm.bak")
        if not backup.exists():
            shutil.copy2(path, backup)
        path.write_text(modified[key])

print("SUCCESS: combined NSS:2 + automatic ASPM source modifications applied.")
print()
print("Changed files:")
for key, path in files.items():
    if modified[key] != original[key]:
        print("  ", path)
print()
print("Verify with:")
print('  grep -n "auto ASPM policy" AirportBrcmFixup/kern_start.cpp')
print('  grep -n "wlc_stf_txchain_set\\|NSS2 txchain" AirportBrcmFixup/kern_brcmfx.cpp AirportBrcmFixup/kern_brcmfx.hpp')
