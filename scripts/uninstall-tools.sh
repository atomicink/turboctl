#!/bin/sh
set -eu

ROOT="${1:-/}"

if [ "$(id -u)" -ne 0 ]; then
	echo "uninstall-tools.sh must be run as root" >&2
	exit 1
fi

if [ "$ROOT" = "/" ]; then
	launchctl bootout system /Library/LaunchDaemons/dev.yu.turboctld.plist >/dev/null 2>&1 || true
fi

rm -f "$ROOT/usr/local/bin/turboctl"
rm -f "$ROOT/usr/local/sbin/turboctld"
rm -f "$ROOT/Library/LaunchDaemons/dev.yu.turboctld.plist"

echo "Removed turboctl tools and LaunchDaemon"
