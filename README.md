# turboctl

`turboctl` is a small Intel Mac tool for disabling Turbo Boost without a
password prompt after every boot or wake.

This project targets an OCLP/OpenCore Intel Mac setup. The kernel extension
does the only privileged operation: it writes bit 38 of `IA32_MISC_ENABLE`
(`MSR 0x1A0`) on every logical CPU. User-space code only talks to the kext
through narrow `sysctl` controls.

## Components

- `TurboCtl.kext`: x86_64 kext that exposes `kern.turboctl.*` controls.
- `turboctl`: CLI for `off`, `on`, `status`, and `reapply`.
- `turboctld`: LaunchDaemon helper that disables Turbo Boost at load and after
  wake.

## Build

```sh
make clean
make
```

Build outputs:

```text
build/TurboCtl.kext
build/turboctl
build/turboctld
```

## Install

For OCLP/OpenCore kext injection, see
[docs/OCLP_INSTALL.md](docs/OCLP_INSTALL.md).

After the kext is injected and visible after reboot, install the user-space
tools:

```sh
sudo scripts/install-tools.sh
```

Then check:

```sh
turboctl status
```

## Sysctls

```text
kern.turboctl.disabled          read actual current-CPU state, write 1/0 to disable/enable
kern.turboctl.desired_disabled  read desired state used by reapply
kern.turboctl.reapply           write any int to reapply desired state
```

## Safety Notes

This is intentionally narrow: it only changes `IA32_MISC_ENABLE` bit 38. It is
not a generic MSR read/write driver.

Use it only on Intel Macs. Do not load this kext on Apple Silicon.
