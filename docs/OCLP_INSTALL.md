# OCLP / OpenCore Install

These steps are for Intel Macs booting modern macOS through OCLP/OpenCore.
Back up your EFI before editing it.

## 1. Build

```sh
make clean
make
```

Confirm the kext is x86_64:

```sh
file build/TurboCtl.kext/Contents/MacOS/TurboCtl
```

Expected:

```text
Mach-O 64-bit kext bundle x86_64
```

## 2. Mount EFI

Find the EFI partition:

```sh
diskutil list
```

Mount it, replacing `diskXs1` with your EFI identifier:

```sh
sudo diskutil mount diskXs1
```

## 3. Copy Kext

```sh
sudo cp -R build/TurboCtl.kext /Volumes/EFI/EFI/OC/Kexts/
```

## 4. Add to OpenCore config

Open:

```text
/Volumes/EFI/EFI/OC/config.plist
```

Use OCAuxiliaryTools or ProperTree and add this entry under `Kernel -> Add`:

```text
Arch            x86_64
BundlePath      TurboCtl.kext
Comment         Disable Intel Turbo Boost via IA32_MISC_ENABLE bit 38
Enabled         true
ExecutablePath  Contents/MacOS/TurboCtl
MaxKernel
MinKernel
PlistPath       Contents/Info.plist
```

Leave `MinKernel` and `MaxKernel` empty for the first test on your OCLP
Sequoia install.

## 5. Reboot and verify

After reboot:

```sh
sysctl kern.turboctl.disabled
sysctl kern.turboctl.desired_disabled
```

Expected:

```text
kern.turboctl.disabled: 1
kern.turboctl.desired_disabled: 1
```

If those keys do not exist, the kext did not load. Check OpenCore config,
bundle path, and OCLP boot logs.

## 6. Install user-space tools

```sh
sudo scripts/install-tools.sh
```

Then:

```sh
turboctl status
turboctl on
turboctl off
```

`turboctld` will run as a LaunchDaemon and call the kext again after wake.

## Notes

- OCLP updates or EFI rebuilds can overwrite `config.plist`; re-add
  `TurboCtl.kext` after rebuilding if needed.
- This kext is unsigned. OpenCore injection is the intended path.
- macOS native `/Library/Extensions` installation is not the main path for this
  project because Sequoia's kext signing and approval flow is much stricter.
