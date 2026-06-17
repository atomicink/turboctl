#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>

#define TURBOCTL_SYSCTL_DISABLED "kern.turboctl.disabled"
#define TURBOCTL_SYSCTL_DESIRED "kern.turboctl.desired_disabled"
#define TURBOCTL_SYSCTL_REAPPLY "kern.turboctl.reapply"

static int read_int_sysctl(const char *name, int *value)
{
	size_t size = sizeof(*value);
	return sysctlbyname(name, value, &size, NULL, 0);
}

static int write_int_sysctl(const char *name, int value)
{
	return sysctlbyname(name, NULL, NULL, &value, sizeof(value));
}

static void print_sysctl_error(const char *operation)
{
	if (errno == ENOENT) {
		fprintf(stderr, "%s failed: TurboCtl.kext is not loaded\n", operation);
		return;
	}

	if (errno == EPERM || errno == EACCES) {
		fprintf(stderr, "%s failed: root privileges are required\n", operation);
		return;
	}

	fprintf(stderr, "%s failed: %s\n", operation, strerror(errno));
}

static int command_status(void)
{
	int disabled = 0;
	int desired = 0;

	if (read_int_sysctl(TURBOCTL_SYSCTL_DISABLED, &disabled) != 0) {
		print_sysctl_error("status");
		return 1;
	}

	if (read_int_sysctl(TURBOCTL_SYSCTL_DESIRED, &desired) != 0) {
		desired = -1;
	}

	printf("Turbo Boost: %s\n", disabled ? "off" : "on");
	if (desired >= 0) {
		printf("Desired boot/wake state: %s\n", desired ? "off" : "on");
	}

	return 0;
}

static int command_set(bool disabled)
{
	if (write_int_sysctl(TURBOCTL_SYSCTL_DISABLED, disabled ? 1 : 0) != 0) {
		print_sysctl_error(disabled ? "off" : "on");
		return 1;
	}

	printf("Turbo Boost: %s\n", disabled ? "off" : "on");
	return 0;
}

static int command_reapply(void)
{
	if (write_int_sysctl(TURBOCTL_SYSCTL_REAPPLY, 1) != 0) {
		print_sysctl_error("reapply");
		return 1;
	}

	return command_status();
}

static void usage(FILE *stream)
{
	fprintf(stream,
	    "usage: turboctl <command>\n"
	    "\n"
	    "commands:\n"
	    "  off       disable Intel Turbo Boost\n"
	    "  on        enable Intel Turbo Boost\n"
	    "  status    show current state\n"
	    "  reapply   reapply the desired state after wake\n");
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		usage(stderr);
		return 2;
	}

	if (strcmp(argv[1], "off") == 0) {
		return command_set(true);
	}

	if (strcmp(argv[1], "on") == 0) {
		return command_set(false);
	}

	if (strcmp(argv[1], "status") == 0) {
		return command_status();
	}

	if (strcmp(argv[1], "reapply") == 0) {
		return command_reapply();
	}

	usage(stderr);
	return 2;
}
