#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOMessage.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysctl.h>
#include <syslog.h>
#include <unistd.h>

#define TURBOCTL_SYSCTL_DISABLED "kern.turboctl.disabled"

struct power_context {
	io_connect_t root_port;
};

static bool apply_disabled(void)
{
	int value = 1;

	if (sysctlbyname(TURBOCTL_SYSCTL_DISABLED, NULL, NULL, &value,
	    sizeof(value)) == 0) {
		syslog(LOG_INFO, "Turbo Boost disabled");
		return true;
	}

	syslog(LOG_ERR, "failed to disable Turbo Boost: %s", strerror(errno));
	return false;
}

static void power_callback(void *refcon, io_service_t service,
    natural_t message_type, void *message_argument)
{
	struct power_context *context = (struct power_context *)refcon;

	(void)service;

	switch (message_type) {
	case kIOMessageCanSystemSleep:
	case kIOMessageSystemWillSleep:
		IOAllowPowerChange(context->root_port, (long)message_argument);
		break;
	case kIOMessageSystemHasPoweredOn:
		sleep(2);
		apply_disabled();
		break;
	default:
		break;
	}
}

int main(void)
{
	struct power_context context = { 0 };
	IONotificationPortRef notify_port = NULL;
	io_object_t notifier = IO_OBJECT_NULL;

	openlog("turboctld", LOG_PID | LOG_NDELAY, LOG_DAEMON);
	apply_disabled();

	context.root_port = IORegisterForSystemPower(&context, &notify_port,
	    power_callback, &notifier);
	if (context.root_port == MACH_PORT_NULL || notify_port == NULL) {
		syslog(LOG_ERR, "power notifications unavailable; using timer fallback");
		for (;;) {
			sleep(60);
			apply_disabled();
		}
	}

	CFRunLoopAddSource(CFRunLoopGetCurrent(),
	    IONotificationPortGetRunLoopSource(notify_port),
	    kCFRunLoopDefaultMode);
	CFRunLoopRun();

	if (notifier != IO_OBJECT_NULL) {
		IOObjectRelease(notifier);
	}
	if (context.root_port != MACH_PORT_NULL) {
		IODeregisterForSystemPower(&notifier);
		IOServiceClose(context.root_port);
	}
	if (notify_port != NULL) {
		IONotificationPortDestroy(notify_port);
	}

	return 0;
}
