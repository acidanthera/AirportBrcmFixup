#!/usr/bin/env python3
from pathlib import Path
import shutil
import sys

# v5: broaden AutoASPM for BCM4360/14e4:43a0 so any Apple subsystem vendor
# (106b:*) preserves platform ASPM. AutoNSS2 remains deliberately limited
# to the validated 106b:0117 2x2 identity.

ROOT = Path.cwd()
SRC = ROOT / "AirportBrcmFixup"

FILES = {
    "config": SRC / "kern_config.hpp",
    "start": SRC / "kern_start.cpp",
    "hpp": SRC / "kern_brcmfx.hpp",
    "cpp": SRC / "kern_brcmfx.cpp",
}

missing = [str(p) for p in FILES.values() if not p.is_file()]
if missing:
    print("ERROR: Run this from the OUTER AirportBrcmFixup 2.1.9 repository folder.")
    for p in missing:
        print("  missing:", p)
    sys.exit(1)

original = {k: p.read_text().splitlines(keepends=True) for k, p in FILES.items()}
work = {k: list(v) for k, v in original.items()}

def exact_index(lines, needle, label, start=0, end=None):
    if end is None:
        end = len(lines)
    hits = [i for i in range(start, end) if lines[i] == needle]
    if len(hits) != 1:
        raise RuntimeError(
            f"{label}: expected exactly one exact line, found {len(hits)}. Nothing written."
        )
    return hits[0]

def contains_index(lines, needle, label, start=0, end=None):
    if end is None:
        end = len(lines)
    hits = [i for i in range(start, end) if needle in lines[i]]
    if len(hits) != 1:
        raise RuntimeError(
            f"{label}: expected exactly one matching line, found {len(hits)}. Nothing written."
        )
    return hits[0]

# --------------------------------------------------------------------------
# kern_config.hpp
# --------------------------------------------------------------------------
if not any("bootargBrcmNss1" in line for line in work["config"]):
    i = exact_index(
        work["config"],
        '\tstatic constexpr const char *bootargDelay          {"brcmfx-delay"};\n',
        "kern_config.hpp boot-arg insertion"
    )
    work["config"][i+1:i+1] = [
        '\tstatic constexpr const char *bootargBrcmNss1       {"brcmfx-nss1"};      // =1 disables automatic NSS:2 correction\n'
    ]

if not any("enable_nss2" in line for line in work["config"]):
    i = exact_index(
        work["config"],
        '\tbool override_aspm          {false};\n',
        "kern_config.hpp state insertion"
    )
    work["config"][i+1:i+1] = [
        '\tbool force_nss1             {false};\n',
        '\tbool enable_nss2            {false};\n',
    ]

# --------------------------------------------------------------------------
# kern_start.cpp
# --------------------------------------------------------------------------
if not any("bootargBrcmNss1" in line for line in work["start"]):
    i = exact_index(
        work["start"],
        '\t\tenable_all_drv = checkKernelArgument(bootargBrcmAllDrv);\n',
        "kern_start.cpp brcmfx-nss1 insertion"
    )
    work["start"][i+1:i+1] = [
        '\n',
        '\t\tuint32_t nss1_value = 0;\n',
        '\t\tif (PE_parse_boot_argn(bootargBrcmNss1, &nss1_value, sizeof(nss1_value)))\n',
        '\t\t\tforce_nss1 = (nss1_value != 0);\n',
    ]

# Capture whether the user explicitly supplied ASPM after upstream has parsed
# both the boot argument and the provider property.
if not any("userAspmOverride" in line for line in work["start"]):
    i = exact_index(
        work["start"],
        '\t\tif (PE_parse_boot_argn(bootargDelay, &start_delay, sizeof(start_delay))) {\n',
        "kern_start.cpp provider delay parser"
    )
    work["start"][i:i] = [
        '\t\tbool userAspmOverride = override_aspm;\n',
        '\n',
    ]

# Upgrade the v4 AutoASPM whitelist in-place when this script is run on an
# already-patched tree. AutoNSS2 intentionally remains restricted to 106b:0117.
v4_aspm = [
    '\t\t\tif (autoSubVendorID == 0x106b &&\n',
    '\t\t\t\t(autoSubDeviceID == 0x0111 || autoSubDeviceID == 0x0117 || autoSubDeviceID == 0x0134)) {\n',
    '\t\t\t\t// Native Apple BCM94360 identities: preserve platform ASPM (0xFF sentinel).\n',
]
v5_aspm = [
    '\t\t\tif (autoSubVendorID == 0x106b) {\n',
    '\t\t\t\t// Apple-subsystem BCM4360 identity: preserve platform ASPM (0xFF sentinel).\n',
]
joined_start = ''.join(work["start"])
old_v4_aspm = ''.join(v4_aspm)
if old_v4_aspm in joined_start:
    hits = joined_start.count(old_v4_aspm)
    if hits != 1:
        raise RuntimeError(
            f"kern_start.cpp v4 AutoASPM upgrade: expected exactly one old policy, found {hits}. Nothing written."
        )
    joined_start = joined_start.replace(old_v4_aspm, ''.join(v5_aspm), 1)
    work["start"] = joined_start.splitlines(keepends=True)

# Insert automatic card policy immediately before upstream's 0xFF handling.
if not any("AutoNSS2/AutoASPM policy" in line for line in work["start"]):
    i = exact_index(
        work["start"],
        '\t\tif (brcmfx_aspm != 0xFF)\n',
        "kern_start.cpp automatic policy insertion point"
    )
    policy = [
        '\n',
        '\t\t// AutoNSS2/AutoASPM policy.\n',
        '\t\tuint16_t autoVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigVendorID);\n',
        '\t\tuint16_t autoDeviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigDeviceID);\n',
        '\t\tuint16_t autoSubVendorID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemVendorID);\n',
        '\t\tuint16_t autoSubDeviceID = pciDevice->configRead16(WIOKit::PCIRegister::kIOPCIConfigSubSystemID);\n',
        '\n',
        '\t\t// Apple BCM94360CS2 / common BCM94360NG identity is 2x2:2.\n',
        '\t\tenable_nss2 = !force_nss1 &&\n',
        '\t\t\tautoVendorID == 0x14e4 && autoDeviceID == 0x43a0 &&\n',
        '\t\t\tautoSubVendorID == 0x106b && autoSubDeviceID == 0x0117;\n',
        '\n',
        '\t\tif (enable_nss2)\n',
        '\t\t\tDBGLOG("BRCMFX", "AutoNSS2: enabled for %04x:%04x subsystem %04x:%04x",\n',
        '\t\t\t\tautoVendorID, autoDeviceID, autoSubVendorID, autoSubDeviceID);\n',
        '\t\telse if (force_nss1)\n',
        '\t\t\tDBGLOG("BRCMFX", "AutoNSS2: disabled by brcmfx-nss1=1");\n',
        '\n',
        '\t\t// Explicit brcmfx-aspm always wins over automatic selection.\n',
        '\t\tif (!userAspmOverride && autoVendorID == 0x14e4 && autoDeviceID == 0x43a0) {\n',
        '\t\t\tif (autoSubVendorID == 0x106b) {\n',
        '\t\t\t\t// Apple-subsystem BCM4360 identity: preserve platform ASPM (0xFF sentinel).\n',
        '\t\t\t\tbrcmfx_aspm = 0xFF;\n',
        '\t\t\t\toverride_aspm = false;\n',
        '\t\t\t\tDBGLOG("BRCMFX", "AutoASPM: preserving platform ASPM for subsystem %04x:%04x",\n',
        '\t\t\t\t\tautoSubVendorID, autoSubDeviceID);\n',
        '\t\t\t} else {\n',
        '\t\t\t\t// Non-Apple BCM4360 identity: L1 + CLKREQ (2 + 256 = 258).\n',
        '\t\t\t\tbrcmfx_aspm = 0x102;\n',
        '\t\t\t\toverride_aspm = true;\n',
        '\t\t\t\tDBGLOG("BRCMFX", "AutoASPM: using 258 (L1+CLKREQ) for subsystem %04x:%04x",\n',
        '\t\t\t\t\tautoSubVendorID, autoSubDeviceID);\n',
        '\t\t\t}\n',
        '\t\t}\n',
        '\n',
    ]
    work["start"][i:i] = policy

# --------------------------------------------------------------------------
# kern_brcmfx.hpp
# --------------------------------------------------------------------------
if not any("wlc_stf_txchain_set(void *wlc" in line for line in work["hpp"]):
    i = exact_index(
        work["hpp"],
        '\tstatic int64_t          siPmuFvcoPllreg(uint32_t *a1, int64_t a2, int64_t a3);\n',
        "kern_brcmfx.hpp NSS callback declaration"
    )
    work["hpp"][i+1:i+1] = [
        '\tstatic uint64_t         wlc_stf_txchain_set(void *wlc, uint64_t chain, uint64_t arg3, uint64_t reason);\n'
    ]

if not any("orgWlcStfTxchainSet" in line for line in work["hpp"]):
    i = exact_index(
        work["hpp"],
        '\tmach_vm_address_t orgSiPmuFvcoPllreg[MaxServices] {};\n',
        "kern_brcmfx.hpp NSS trampoline"
    )
    work["hpp"][i+1:i+1] = [
        '\tmach_vm_address_t orgWlcStfTxchainSet {};\n'
    ]

# --------------------------------------------------------------------------
# kern_brcmfx.cpp
# --------------------------------------------------------------------------
if not any("uint64_t BRCMFX::wlc_stf_txchain_set" in line for line in work["cpp"]):
    fn = contains_index(
        work["cpp"],
        "int64_t BRCMFX::siPmuFvcoPllreg(",
        "kern_brcmfx.cpp siPmuFvcoPllreg function"
    )

    debug_candidates = [
        i for i in range(fn + 1, len(work["cpp"]))
        if work["cpp"][i] == '#ifdef DEBUG\n'
    ]
    if not debug_candidates:
        raise RuntimeError(
            "kern_brcmfx.cpp: could not find #ifdef DEBUG after siPmuFvcoPllreg. Nothing written."
        )
    debug_after = debug_candidates[0]

    body = [
        '\n',
        '//==============================================================================\n',
        '// Correct only the cold-boot call that constrains the known 2x2 card to TX chain 1.\n',
        'uint64_t BRCMFX::wlc_stf_txchain_set(void *wlc, uint64_t chain, uint64_t arg3, uint64_t reason)\n',
        '{\n',
        '\tif (ADDPR(brcmfx_config).enable_nss2 && reason == 2 && (chain & 0xFF) == 1) {\n',
        '\t\tDBGLOG("BRCMFX", "AutoNSS2: wlc_stf_txchain_set reason=2 chain 1 -> 3");\n',
        '\t\tchain = (chain & ~0xFFULL) | 0x3;\n',
        '\t}\n',
        '\n',
        '\treturn FunctionCast(wlc_stf_txchain_set, callbackBRCMFX->orgWlcStfTxchainSet)(wlc, chain, arg3, reason);\n',
        '}\n',
    ]
    work["cpp"][debug_after:debug_after] = body

if not any("AutoNSS2 route installed" in line for line in work["cpp"]):
    i = exact_index(
        work["cpp"],
        '\t\t\t\t\tDBGLOG("BRCMFX", "all patches are successfuly applied to %s", idList[i]);\n',
        "kern_brcmfx.cpp route insertion point"
    )
    route = [
        '\n',
        '\t\t\t\tif (i == AirPort_BrcmNIC) {\n',
        '\t\t\t\t\tpatcher.clearError();\n',
        '\t\t\t\t\tKernelPatcher::RouteRequest nss2Request[] {\n',
        '\t\t\t\t\t\t{"_wlc_stf_txchain_set", reinterpret_cast<mach_vm_address_t>(BRCMFX::wlc_stf_txchain_set), orgWlcStfTxchainSet}\n',
        '\t\t\t\t\t};\n',
        '\t\t\t\t\tif (!patcher.routeMultiple(index, nss2Request, address, size))\n',
        '\t\t\t\t\t\tSYSLOG("BRCMFX", "AutoNSS2 route failed, error = %d", patcher.getError());\n',
        '\t\t\t\t\telse\n',
        '\t\t\t\t\t\tDBGLOG("BRCMFX", "AutoNSS2 route installed");\n',
        '\t\t\t\t\tpatcher.clearError();\n',
        '\t\t\t\t}\n',
    ]
    work["cpp"][i+1:i+1] = route

# --------------------------------------------------------------------------
# Validate prospective result before writing.
# --------------------------------------------------------------------------
markers = {
    "config": ["bootargBrcmNss1", "force_nss1", "enable_nss2"],
    "start": [
        "AutoNSS2/AutoASPM policy",
        "autoSubDeviceID == 0x0117",
        "if (autoSubVendorID == 0x106b)",
        "brcmfx_aspm = 0xFF",
        "brcmfx_aspm = 0x102",
        "userAspmOverride",
    ],
    "hpp": ["wlc_stf_txchain_set(void *wlc", "orgWlcStfTxchainSet"],
    "cpp": [
        "uint64_t BRCMFX::wlc_stf_txchain_set",
        "reason == 2 && (chain & 0xFF) == 1",
        "AutoNSS2 route installed",
    ],
}
for key, required in markers.items():
    joined = ''.join(work[key])
    for marker in required:
        if marker not in joined:
            raise RuntimeError(
                f"Internal validation failed: {key} missing {marker!r}. Nothing written."
            )

# v5 must not retain the old three-subdevice AutoASPM whitelist.
if "autoSubDeviceID == 0x0111 || autoSubDeviceID == 0x0117 || autoSubDeviceID == 0x0134" in ''.join(work["start"]):
    raise RuntimeError("Internal validation failed: old v4 AutoASPM whitelist is still present. Nothing written.")

changed = [key for key in FILES if work[key] != original[key]]
if not changed:
    print("Nothing to do: requested modifications already appear to be present.")
    sys.exit(0)

if "--check" in sys.argv[1:]:
    print("CHECK OK: every required AirportBrcmFixup 2.1.9 source location was found.")
    print("No files were changed.")
    print("Ready to apply.")
    sys.exit(0)

for key in changed:
    path = FILES[key]
    backup = path.with_name(path.name + ".pre-autoaspm-v5.bak")
    if not backup.exists():
        shutil.copy2(path, backup)
    path.write_text(''.join(work[key]))

print("SUCCESS: AutoNSS2 + AutoASPM v5 modifications applied.")
print()
print("Default policy:")
print("  14e4:43a0 / 106b:0117 -> NSS:2 correction ON")
print("  14e4:43a0 / any 106b:* subsystem -> preserve platform ASPM")
print("  14e4:43a0 / non-106b subsystem -> ASPM 258 (L1 + CLKREQ)")
print("  other device IDs -> stock AirportBrcmFixup ASPM behavior")
print()
print("Optional opt-out:")
print("  brcmfx-nss1=1 -> disables automatic NSS:2 correction")
print()
print("Explicit brcmfx-aspm=... overrides automatic ASPM selection.")
print()
print("Next:")
print("  git diff --check")
print("  git diff")
