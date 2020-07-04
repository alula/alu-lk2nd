#include <string.h>
#include <printf.h>
#include <debug.h>
#include <pm8x41_regulator.h>
#include <libfdt.h>
#include <lk2nd-device.h>
#include <lib/fs.h>
#include <list.h>
#include <lib/bio.h>
#include <kernel/mutex.h>
#include "fastboot.h"

#if TARGET_MSM8916
static void cmd_oem_dump_regulators(const char *arg, void *data, unsigned sz)
{
	char response[MAX_RSP_SIZE];
	struct spmi_regulator *vreg;
	for (vreg = target_get_regulators(); vreg->name; ++vreg) {
		snprintf(response, sizeof(response), "%s: enabled: %d, voltage: %d mV",
			 vreg->name, regulator_is_enabled(vreg),
			 regulator_get_voltage(vreg));
		fastboot_info(response);
	}
	fastboot_okay("");
}
#endif

#if WITH_DEBUG_LOG_BUF
static void cmd_oem_lk_log(const char *arg, void *data, unsigned sz)
{
	fastboot_stage(lk_log_getbuf(), lk_log_getsize());
}
#endif

static void cmd_oem_dtb(const char *arg, void *data, unsigned sz)
{
	fastboot_stage(lk2nd_dev.fdt, fdt_totalsize(lk2nd_dev.fdt));
}

static void cmd_oem_pstore(const char *arg, void *data, unsigned sz)
{
	fastboot_stage(lk2nd_dev.pstore, lk2nd_dev.pstore_size);
}

static void cmd_oem_cmdline(const char *arg, void *data, unsigned sz)
{
	fastboot_stage(lk2nd_dev.cmdline, strlen(lk2nd_dev.cmdline));
}

extern struct bdev_struct *bdevs;

static void cmd_oem_bdevs(const char* arg, void* data, unsigned sz)
{
	char reason[512] = {0};
	bdev_t *entry;
	fastboot_info("block devices:");
	mutex_acquire(&bdevs->lock);
	list_for_every_entry(&bdevs->list, entry, bdev_t, node) {
		snprintf(&reason, 512, "\t%s, size %lld, bsize %zd, ref %d\n", entry->name, entry->size, entry->block_size, entry->ref);
		fastboot_info(&reason);
	}
	mutex_release(&bdevs->lock);
	fastboot_okay("");
}

static void cmd_oem_ls(const char *arg, void *data, unsigned sz)
{
	char reason[512] = {0};
	struct dirent entry;
	dirhandle* handle;
	int entries = 0;
	int err;

	if ((err = fs_open_dir(arg, &handle)) < 0) {
		snprintf(&reason, 512, "Failed to open directory %s: %d", arg, err);
		fastboot_fail(&reason);
		return;
	}

	while(fs_read_dir(handle, &entry) >= 0) {
		fastboot_info(entry.name);
		entries++;
	}

	fs_close_dir(handle);
	snprintf(&reason, 512, "%d entries present.", entries);
	fastboot_info(&reason);
	fastboot_okay("");
}

void fastboot_lk2nd_register_commands(void) {
#if TARGET_MSM8916
	fastboot_register("oem dump-regulators", cmd_oem_dump_regulators);
#endif
#if WITH_DEBUG_LOG_BUF
	fastboot_register("oem lk_log", cmd_oem_lk_log);
#endif

	if (lk2nd_dev.fdt)
		fastboot_register("oem dtb", cmd_oem_dtb);

	if (lk2nd_dev.pstore)
		fastboot_register("oem pstore", cmd_oem_pstore);

	if (lk2nd_dev.cmdline)
		fastboot_register("oem cmdline", cmd_oem_cmdline);

	fastboot_register("oem ls", cmd_oem_ls);
	fastboot_register("oem bdevs", cmd_oem_bdevs);
}
