#!/bin/sh
set -eu

ROOT="${1:-/}"
PROJECT_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

if [ "$(id -u)" -ne 0 ]; then
	echo "install-tools.sh must be run as root" >&2
	exit 1
fi

if [ ! -x "$PROJECT_ROOT/build/turboctl" ] || [ ! -x "$PROJECT_ROOT/build/turboctld" ]; then
	echo "build/turboctl and build/turboctld are missing; run make tools first" >&2
	exit 1
fi

install -d -m 0755 "$ROOT/usr/local/bin"
install -d -m 0755 "$ROOT/usr/local/sbin"
install -d -m 0755 "$ROOT/Library/LaunchDaemons"

install -m 0755 "$PROJECT_ROOT/build/turboctl" "$ROOT/usr/local/bin/turboctl"
install -m 0755 "$PROJECT_ROOT/build/turboctld" "$ROOT/usr/local/sbin/turboctld"
install -m 0644 "$PROJECT_ROOT/launchd/dev.yu.turboctld.plist" \
	"$ROOT/Library/LaunchDaemons/dev.yu.turboctld.plist"

if [ "$ROOT" = "/" ]; then
	launchctl bootout system /Library/LaunchDaemons/dev.yu.turboctld.plist >/dev/null 2>&1 || true
	launchctl bootstrap system /Library/LaunchDaemons/dev.yu.turboctld.plist
	launchctl enable system/dev.yu.turboctld
	launchctl kickstart -k system/dev.yu.turboctld
fi

echo "Installed turboctl, turboctld, and dev.yu.turboctld LaunchDaemon"
