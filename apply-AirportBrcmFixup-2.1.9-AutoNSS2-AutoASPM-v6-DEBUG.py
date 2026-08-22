#!/usr/bin/env python3
from pathlib import Path
import shutil
import subprocess
import sys
import plistlib

# v6 DEBUG builder:
# - preserves the v5 AutoNSS2/AutoASPM source modifications unchanged
# - refuses to operate on a project that is not AirportBrcmFixup 2.1.9
# - defaults to patching and then building the x86_64 DEBUG configuration
# - verifies the built kext reports 2.1.9 and contains the custom DEBUG log strings
#
# v5 policy: broaden AutoASPM for BCM4360/14e4:43a0 so any Apple subsystem vendor
# (106b:*) preserves platform ASPM. AutoNSS2 remains deliberately limited
# to the validated 106b:0117 2x2 identity.

ROOT = Path.cwd()
SRC = ROOT / "AirportBrcmFixup"
PROJECT = ROOT / "AirportBrcmFixup.xcodeproj"
PBXPROJ = PROJECT / "project.pbxproj"
BUILD_ROOT = ROOT / "build"
EXPECTED_VERSION = "2.1.9"
CUSTOM_ARCHIVE = ROOT / "AirportBrcmFixup-2.1.9-AutoNSS2-AutoASPM-v5-DEBUG.zip"

CHECK_ONLY = "--check" in sys.argv[1:]
PATCH_ONLY = "--patch-only" in sys.argv[1:]

unknown_args = [arg for arg in sys.argv[1:] if arg not in {"--check", "--patch-only"}]
if unknown_args:
    print("ERROR: Unknown argument(s):", " ".join(unknown_args))
    print("Usage:")
    print(f"  python3 {Path(sys.argv[0]).name}                # patch + build DEBUG")
    print(f"  python3 {Path(sys.argv[0]).name} --check        # validate only; write/build nothing")
    print(f"  python3 {Path(sys.argv[0]).name} --patch-only   # patch source only; do not build")
    sys.exit(2)

if CHECK_ONLY and PATCH_ONLY:
    print("ERROR: --check and --patch-only cannot be used together.")
    sys.exit(2)

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


def validate_project_version():
    if not PBXPROJ.is_file():
        raise RuntimeError(
            "AirportBrcmFixup.xcodeproj/project.pbxproj is missing. "
            "Run this from the OUTER AirportBrcmFixup 2.1.9 repository folder."
        )

    pbx = PBXPROJ.read_text(errors="replace")
    version_marker = f"MODULE_VERSION = {EXPECTED_VERSION};"
    if version_marker not in pbx:
        raise RuntimeError(
            f"Expected AirportBrcmFixup {EXPECTED_VERSION} project marker "
            f"{version_marker!r}, but it was not found. Refusing to patch/build a different version."
        )

    if "DEBUG=1" not in pbx or "name = Debug;" not in pbx:
        raise RuntimeError(
            "The expected Xcode Debug configuration was not found in the 2.1.9 project."
        )


def validate_build_prerequisites():
    if shutil.which("xcodebuild") is None:
        raise RuntimeError(
            "xcodebuild was not found. Install/select full Xcode before building."
        )

    required = [
        ROOT / "MacKernelSDK" / "Headers",
        ROOT / "MacKernelSDK" / "Library" / "x86_64",
        ROOT / "Lilu.kext" / "Contents" / "Resources" / "Headers",
        ROOT / "Lilu.kext" / "Contents" / "Resources" / "Library",
    ]
    missing_build = [str(p) for p in required if not p.exists()]
    if missing_build:
        lines = "\n".join(f"  missing: {p}" for p in missing_build)
        raise RuntimeError(
            "DEBUG build prerequisites are incomplete.\n"
            "AirportBrcmFixup 2.1.9 expects MacKernelSDK and Lilu.kext in the repository root.\n"
            f"{lines}\n"
            "Use a DEBUG build of Lilu.kext when building/using a DEBUG AirportBrcmFixup."
        )


def locate_debug_product():
    preferred_kext = BUILD_ROOT / "Debug" / "AirportBrcmFixup.kext"
    preferred_zip = BUILD_ROOT / "Debug" / f"AirportBrcmFixup-{EXPECTED_VERSION}-DEBUG.zip"

    kext = preferred_kext if preferred_kext.is_dir() else None
    archive = preferred_zip if preferred_zip.is_file() else None

    if kext is None and BUILD_ROOT.exists():
        candidates = sorted(
            p for p in BUILD_ROOT.rglob("AirportBrcmFixup.kext")
            if p.is_dir() and "Debug" in p.parts
        )
        if candidates:
            kext = candidates[-1]

    if archive is None and BUILD_ROOT.exists():
        candidates = sorted(
            p for p in BUILD_ROOT.rglob(f"AirportBrcmFixup-{EXPECTED_VERSION}-DEBUG.zip")
            if p.is_file()
        )
        if candidates:
            archive = candidates[-1]

    return kext, archive


def validate_debug_product(kext):
    if kext is None or not kext.is_dir():
        raise RuntimeError(
            "xcodebuild completed but AirportBrcmFixup.kext was not found under build/Debug."
        )

    info = kext / "Contents" / "Info.plist"
    binary = kext / "Contents" / "MacOS" / "AirportBrcmFixup"

    if not info.is_file() or not binary.is_file():
        raise RuntimeError(
            f"Built kext is incomplete: {kext}"
        )

    with info.open("rb") as f:
        plist = plistlib.load(f)

    versions = {
        str(plist.get("CFBundleShortVersionString", "")),
        str(plist.get("CFBundleVersion", "")),
    }
    if EXPECTED_VERSION not in versions:
        raise RuntimeError(
            f"Built kext does not report version {EXPECTED_VERSION}; found {sorted(versions)!r}."
        )

    data = binary.read_bytes()
    required_debug_strings = [
        b"AutoNSS2:",
        b"AutoASPM:",
        b"AutoNSS2 route installed",
    ]
    missing_strings = [s.decode() for s in required_debug_strings if s not in data]
    if missing_strings:
        raise RuntimeError(
            "Built binary is missing expected custom DEBUG log strings: "
            + ", ".join(missing_strings)
            + ". This is not the expected custom DEBUG product."
        )


def build_debug():
    validate_build_prerequisites()

    print()
    print("Building AirportBrcmFixup 2.1.9 DEBUG (x86_64)...")
    print("NOTE: Make sure the Lilu.kext in this repository is the DEBUG build.")
    print()

    cmd = [
        "xcodebuild",
        "-project", str(PROJECT),
        "-jobs", "1",
        "-configuration", "Debug",
        "ARCHS=x86_64",
        "ONLY_ACTIVE_ARCH=YES",
        "clean",
        "build",
    ]
    subprocess.run(cmd, cwd=ROOT, check=True)

    kext, archive = locate_debug_product()
    validate_debug_product(kext)

    if archive is None:
        raise RuntimeError(
            "The custom DEBUG kext built successfully, but the expected DEBUG zip archive "
            "was not produced by the Xcode Archive build phase."
        )

    shutil.copy2(archive, CUSTOM_ARCHIVE)

    print()
    print("DEBUG BUILD VERIFIED.")
    print(f"  Kext:    {kext}")
    print(f"  Archive: {archive}")
    print(f"  Copy:    {CUSTOM_ARCHIVE}")
    print()
    print("The built binary contains the custom AutoNSS2/AutoASPM DEBUG log strings.")
    print("For runtime debug output, use DEBUG Lilu together with this DEBUG AirportBrcmFixup.")

validate_project_version()

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

if CHECK_ONLY:
    print("CHECK OK: every required AirportBrcmFixup 2.1.9 source/project location was found.")
    if changed:
        print("The tree is patchable. No files were changed.")
    else:
        print("The requested AutoNSS2 + AutoASPM v5 modifications are already present.")
        print("No files were changed.")
    print("DEBUG project configuration for version 2.1.9 is present.")
    sys.exit(0)

if changed:
    for key in changed:
        path = FILES[key]
        backup = path.with_name(path.name + ".pre-autoaspm-v5.bak")
        if not backup.exists():
            shutil.copy2(path, backup)
        path.write_text(''.join(work[key]))

    print("SUCCESS: AutoNSS2 + AutoASPM v5 modifications applied.")
else:
    print("Source already contains the requested AutoNSS2 + AutoASPM v5 modifications.")

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

if PATCH_ONLY:
    print()
    print("PATCH-ONLY complete. DEBUG build was not started.")
    print("Next:")
    print("  git diff --check")
    print("  git diff")
    sys.exit(0)

try:
    build_debug()
except subprocess.CalledProcessError as e:
    raise RuntimeError(
        f"xcodebuild failed with exit status {e.returncode}. "
        "The source changes remain applied; no built kext was accepted as valid."
    ) from e
