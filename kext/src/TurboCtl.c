#include <mach/kmod.h>
#include <mach/mach_types.h>
#include <sys/errno.h>
#include <sys/sysctl.h>
#include <i386/cpuid.h>
#include <i386/proc_reg.h>
#include <libkern/libkern.h>

#define TURBOCTL_VERSION "0.1.0"
#define TURBOCTL_TURBO_DISABLE_BIT (1ULL << 38)

extern void mp_rendezvous_no_intrs(void (*action_func)(void *), void *arg);

static int g_desired_disabled = 1;

static int turboctl_cpu_supported(void)
{
	uint32_t data[4];

	do_cpuid(0, data);
	if (data[1] != 0x756e6547 || data[3] != 0x49656e69 ||
	    data[2] != 0x6c65746e) {
		return 0;
	}

	do_cpuid(1, data);
	return (data[3] & (1U << 5)) ? 1 : 0;
}

static int turboctl_read_disabled_current_cpu(void)
{
	uint64_t value = rdmsr64(MSR_IA32_MISC_ENABLE);
	return (value & TURBOCTL_TURBO_DISABLE_BIT) ? 1 : 0;
}

static void turboctl_apply_current_cpu(void *arg)
{
	uint64_t value = rdmsr64(MSR_IA32_MISC_ENABLE);

	if (arg != NULL) {
		value |= TURBOCTL_TURBO_DISABLE_BIT;
	} else {
		value &= ~TURBOCTL_TURBO_DISABLE_BIT;
	}

	wrmsr64(MSR_IA32_MISC_ENABLE, value);
}

static void turboctl_apply_all_cpus(int disabled)
{
	void *arg = disabled ? (void *)1 : NULL;

	mp_rendezvous_no_intrs(turboctl_apply_current_cpu, arg);
	g_desired_disabled = disabled ? 1 : 0;
}

static int turboctl_sysctl_disabled SYSCTL_HANDLER_ARGS
{
	int value = turboctl_read_disabled_current_cpu();
	int error = sysctl_handle_int(oidp, &value, 0, req);

	if (error != 0 || req->newptr == 0) {
		return error;
	}

	if (value != 0 && value != 1) {
		return EINVAL;
	}

	turboctl_apply_all_cpus(value);
	return 0;
}

static int turboctl_sysctl_reapply SYSCTL_HANDLER_ARGS
{
	int value = 0;
	int error = sysctl_handle_int(oidp, &value, 0, req);

	if (error != 0) {
		return error;
	}

	if (req->newptr != 0) {
		turboctl_apply_all_cpus(g_desired_disabled);
	}

	return 0;
}

SYSCTL_NODE(_kern, OID_AUTO, turboctl, CTLFLAG_RD | CTLFLAG_LOCKED, 0,
    "TurboCtl controls");
SYSCTL_PROC(_kern_turboctl, OID_AUTO, disabled,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, 0, 0,
    turboctl_sysctl_disabled, "I", "1 disables Intel Turbo Boost");
SYSCTL_PROC(_kern_turboctl, OID_AUTO, reapply,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, 0, 0,
    turboctl_sysctl_reapply, "I", "write to reapply desired Turbo Boost state");
SYSCTL_INT(_kern_turboctl, OID_AUTO, desired_disabled,
    CTLFLAG_RD | CTLFLAG_LOCKED, &g_desired_disabled, 0,
    "desired Turbo Boost disabled state");

static void turboctl_register_sysctls(void)
{
	sysctl_register_oid(&sysctl__kern_turboctl);
	sysctl_register_oid(&sysctl__kern_turboctl_disabled);
	sysctl_register_oid(&sysctl__kern_turboctl_reapply);
	sysctl_register_oid(&sysctl__kern_turboctl_desired_disabled);
}

static void turboctl_unregister_sysctls(void)
{
	sysctl_unregister_oid(&sysctl__kern_turboctl_desired_disabled);
	sysctl_unregister_oid(&sysctl__kern_turboctl_reapply);
	sysctl_unregister_oid(&sysctl__kern_turboctl_disabled);
	sysctl_unregister_oid(&sysctl__kern_turboctl);
}

static kern_return_t turboctl_start(kmod_info_t *ki, void *data)
{
	(void)ki;
	(void)data;

	if (!turboctl_cpu_supported()) {
		printf("TurboCtl %s refused to load: unsupported CPU\n",
		    TURBOCTL_VERSION);
		return KERN_FAILURE;
	}

	turboctl_register_sysctls();
	turboctl_apply_all_cpus(1);
	printf("TurboCtl %s loaded: Turbo Boost disabled\n", TURBOCTL_VERSION);
	return KERN_SUCCESS;
}

static kern_return_t turboctl_stop(kmod_info_t *ki, void *data)
{
	(void)ki;
	(void)data;

	turboctl_apply_all_cpus(0);
	turboctl_unregister_sysctls();
	printf("TurboCtl %s unloaded: Turbo Boost enabled\n", TURBOCTL_VERSION);
	return KERN_SUCCESS;
}

KMOD_EXPLICIT_DECL(dev.yu.turboctl, TURBOCTL_VERSION,
    turboctl_start, turboctl_stop)
