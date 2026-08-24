**Read Me:
Bellow is a list of boot-args that may be used with this kext:**

brcmfx-nss1=1     = may be used to force NSS:1 (for testing purposes) 
brcmfx-aspm       = (used to be needed to avoid this power management bug with c-state). May still be applied manually if AutoASPM results aren't Satisfactory. 
brcmfx-delay       = existing initialization delay option to improve Airplay connectivity. 
-brcmfxdbg         = Existing AirportBrcmFixup debug logging flag

**Here’s the complete progression of changes made to this Kext so far:**

	•	1. Added NSS debugging/probing to find the NSS:1 problem. We used AirportBrcmFixup’s DEBUG-side NSS hooks and temporarily routed _wlc_stf_txchain_set so we could see what the Broadcom driver was actually doing rather than just trusting the reported link rate. The logs repeatedly showed the driver operating with NSS=1, and the temporary STF hook exposed the important call where the driver requested TX chain mask 1.
	•	2. Identified the actual cold-boot NSS bug. We narrowed it down to _wlc_stf_txchain_set() being called with reason == 2 and a low-byte chain mask of 1. That is the call that was constraining the known 2×2 card to one transmit chain. Instead of faking the reported NSS, our fix changes that specific request from chain mask 1 to 3.
	•	3. Made the NSS fix surgical. The callback only intervenes when all of the conditions match. It preserves all of the upper bits of the original argument and changes only the low byte: chain = (chain & ~0xFFULL) | 0x3. Everything else goes straight through untouched to the original Broadcom function.
	•	4. Converted the NSS experiment into a production Release-build fix. The early STF hook was DEBUG-only. We moved the _wlc_stf_txchain_set route outside the DEBUG-only machinery and route it directly when AirPort_BrcmNIC loads, so the correction remains active in a normal Release AirportBrcmFixup build.
	•	5. Added hardware gating for AutoNSS2. We do not apply the 1 → 3 chain correction to every BCM4360. AutoNSS2 is enabled only for 14e4:43a0 / 106b:0117, the Apple-compatible 2×2 identity we've actually established is appropriate for this correction. The 3×3 BCM94360 variants are deliberately not forced to mask 3.
	•	6. Added the brcmfx-nss1=1 escape hatch. The custom kext gained a new boot argument specifically for testing/regression purposes. brcmfx-nss1=1 disables AutoNSS2 and lets the original NSS:1 behavior occur. Internally we added force_nss1 and enable_nss2 state to support this.
	•	7. Added automatic PCI identity detection. AirportBrcmFixup now reads the Wi-Fi device's PCI vendor ID, device ID, subsystem vendor ID, and subsystem device ID itself. That lets the kext decide what policy to use based on the actual Broadcom card rather than requiring you to manually select behavior with boot arguments.
	•	8. Added userAspmOverride tracking. We changed the existing brcmfx-aspm handling so the kext remembers whether the value came explicitly from the user—either as a boot argument or IORegistry property. If you explicitly specify brcmfx-aspm=..., your value always wins and AutoASPM does not interfere.
	•	9. Added AutoASPM. For BCM4360/BCM94360 hardware (14e4:43a0), the kext can now choose an ASPM policy automatically instead of requiring brcmfx-aspm in boot-args. We use AirportBrcmFixup's existing 0xFF sentinel to mean essentially don't override pci-aspm-default; preserve the firmware/platform configuration. For appropriate non-Apple BCM4360 identities we use 0x102, decimal 258, meaning L1 + CLKREQ.
	•	10. Initially supported the known Apple BCM94360 identities explicitly. The earlier AutoASPM version recognized 106b:0111, 106b:0117, and 106b:0134 and preserved platform ASPM for those cards. Other 14e4:43a0 identities were moved to 258/L1+CLKREQ.
	•	11. Generalized non-Apple BCM4360 handling. Instead of recognizing only one particular third-party subsystem ID for the 258 policy, we expanded that so other/non-Apple 14e4:43a0 identities receive 0x102 / 258 automatically. This makes AutoASPM useful for a much wider range of BCM94360-family cards.
	•	12. Today's v5 broadened Apple-compatible BCM94360 support even further. We removed the hardcoded AutoASPM whitelist of only 0111/0117/0134. Now the rule is simply: if the device is 14e4:43a0 and the subsystem vendor is Apple 106b, preserve platform ASPM regardless of the subsystem-device ID. AutoNSS2 remains restricted to 106b:0117. The v5 script explicitly verifies that the old three-device AutoASPM whitelist is gone.
	•	13. Unknown Broadcom hardware remains untouched. We deliberately didn't start guessing about 43a3, 43ba, BCM43602, etc. If the device ID isn't 43a0, our AutoASPM addition leaves AirportBrcmFixup's original behavior alone. Current v5 therefore has a narrow BCM4360 policy rather than becoming a generic Broadcom power-management hack.
	•	14. Kept NSS correction and ASPM correction independent. This is an important design decision. A card can receive the appropriate automatic ASPM policy without receiving the NSS correction. That's what lets us safely support Apple 3×3 BCM94360 variants for ASPM while restricting the 1 → 3 TX-chain change to the known 2×2 identity.
	•	15. Made the modifications Release-friendly. DEBUG logging such as DBGLOG can disappear from the optimized Release binary, but the actual NSS callback and AutoASPM decisions are compiled into the Release kext. The _wlc_stf_txchain_set route has an error path that remains available even in Release. The current script validates all of those pieces before modifying the source.
	•	16. Built safety into the source-patching scripts. The scripts verify expected source anchors, support a --check dry run, validate that all required code markers exist before writing anything, create backups, and refuse partial modifications. v5 additionally understands an already-v4-patched source tree and upgrades only the AutoASPM portion instead of forcing us to reconstruct the NSS work.
So the current v5 behavior boils down nicely to:

<img width="468" height="343" alt="image" src="https://github.com/user-attachments/assets/a2ba40c2-7218-45d9-9fa9-08e723710bfe" />


The two substantial features added to AirportBrcmFixup are therefore AutoNSS2 and AutoASPM. Everything else done has been about making those two features correctly targeted, reversible, Release-capable, and more broadly compatible without blindly changing unrelated Broadcom hardware.


Disclaimer: This project contains AI generated code.I. I have only tested this with a BCM94360NG card. Others may build on this work or incorporate it in any way they would like, just please give me some credit for what has been performed so far. Please feel free to report any bugs you may find. Thank you.
