/* Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/cpu.h>
#include <linux/interrupt.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/pmic8901.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/qpnp/power-on.h>

#include <asm/mach-types.h>
#include <asm/cacheflush.h>

#include <mach/msm_iomap.h>
#include <mach/restart.h>
#include <mach/socinfo.h>
#include <mach/irqs.h>
#include <mach/scm.h>
#include "msm_watchdog.h"
#include "timer.h"
#include "wdog_debug.h"

#define WDT0_RST	0x38
#define WDT0_EN		0x40
#define WDT0_BARK_TIME	0x4C
#define WDT0_BITE_TIME	0x5C

//[VY5x] ==> CCI KLog, added by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
#include <linux/cciklog.h>
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, added by Jimmy@CCI
#include <linux/proc_fs.h>

#define PSHOLD_CTL_SU (MSM_TLMM_BASE + 0x820)

#define RESTART_REASON_ADDR 0x65C
#define DLOAD_MODE_ADDR     0x0
#define EMERGENCY_DLOAD_MODE_ADDR    0xFE0
#define EMERGENCY_DLOAD_MAGIC1    0x322A4F99
#define EMERGENCY_DLOAD_MAGIC2    0xC67E4350
#define EMERGENCY_DLOAD_MAGIC3    0x77777777

#define SCM_IO_DISABLE_PMIC_ARBITER	1

#ifdef CONFIG_MSM_RESTART_V2
#define use_restart_v2()	1
#else
#define use_restart_v2()	0
#endif

#define CONFIG_WARMBOOT_MAGIC_ADDR  0xAA0
#define CONFIG_WARMBOOT_MAGIC_VAL   0xdeadbeef
 
#define CONFIG_WARMBOOT_ADDR        MSM_IMEM_BASE + 0x65C
#define CONFIG_WARMBOOT_CLEAR       0xabadbabe
#define CONFIG_WARMBOOT_NONE        0x00000000
#define CONFIG_WARMBOOT_S1          0x6f656d53
#define CONFIG_WARMBOOT_FB          0x77665500
#define CONFIG_WARMBOOT_FOTA        0x6f656d46
#define CONFIG_WARMBOOT_FOTA_CACHE  0x6F656D50
#define CONFIG_WARMBOOT_NORMAL      0x77665501
#define CONFIG_WARMBOOT_CRASH       0xc0dedead
#define CONFIG_WARMBOOT_CRASH_ND    0x000052D1
static void* warm_boot_addr;
void set_warmboot(void);

static int restart_mode;
void *restart_reason;

int pmic_reset_irq;
static void __iomem *msm_tmr0_base;

#define ABNORAML_NONE		0x0
#define ABNORAML_REBOOT		0x1
#define ABNORAML_CRASH		0x2
#define ABNORAML_POWEROFF	0x3
long abnormalflag = ABNORAML_NONE;
#ifdef CONFIG_MSM_DLOAD_MODE
static int in_panic;
static void *dload_mode_addr;
static bool dload_mode_enabled;
static void *emergency_dload_mode_addr;

/* Download mode master kill-switch */
static int dload_set(const char *val, struct kernel_param *kp);
static int download_mode = 1;
module_param_call(download_mode, dload_set, param_get_int,
			&download_mode, 0644);
//[VY5x] ==> CCI KLog, added by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
extern void record_shutdown_time(int state);
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, added by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
#define POWER_OFF_SPECIAL_ADDR	IOMEM(0xFB9FFFFC)
#define CRASH_SPECIAL_ADDR	IOMEM(0xFB9FFFF8)
#define UNKONW_CRASH_SPECIAL_ADDR	IOMEM(0xFB9FFFF0)
long* powerpt = (long*)POWER_OFF_SPECIAL_ADDR;
long* unknowflag = (long*)UNKONW_CRASH_SPECIAL_ADDR;
long* backupcrashflag = (long*)CRASH_SPECIAL_ADDR;
#endif
static int panic_prep_restart(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	in_panic = 1;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= panic_prep_restart,
};

static void set_dload_mode(int on)
{
	if (dload_mode_addr) {
# if 0
		__raw_writel(on ? 0xE47B337D : 0, dload_mode_addr);
		__raw_writel(on ? 0xCE14091A : 0,
		       dload_mode_addr + sizeof(unsigned int));
		mb();
		dload_mode_enabled = on;
#endif
	}
}

void set_warmboot()
{
	if (warm_boot_addr) {
		__raw_writel(CONFIG_WARMBOOT_MAGIC_VAL, warm_boot_addr);	   
		mb();
	}
}

static bool get_dload_mode(void)
{
	return dload_mode_enabled;
}

static void enable_emergency_dload_mode(void)
{
	if (emergency_dload_mode_addr) {
//[VY5x] ==> CCI KLog, modified by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
		cklc_save_magic(KLOG_MAGIC_DOWNLOAD_MODE, KLOG_STATE_DOWNLOAD_MODE);
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, modified by Jimmy@CCI
		__raw_writel(EMERGENCY_DLOAD_MAGIC1,
				emergency_dload_mode_addr);
		__raw_writel(EMERGENCY_DLOAD_MAGIC2,
				emergency_dload_mode_addr +
				sizeof(unsigned int));
		__raw_writel(EMERGENCY_DLOAD_MAGIC3,
				emergency_dload_mode_addr +
				(2 * sizeof(unsigned int)));

		/* Need disable the pmic wdt, then the emergency dload mode
		 * will not auto reset. */
		qpnp_pon_wd_config(0);
		mb();
	}
}

static int dload_set(const char *val, struct kernel_param *kp)
{
	int ret;
	int old_val = download_mode;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;

	/* If download_mode is not zero or one, ignore. */
	if (download_mode >> 1) {
		download_mode = old_val;
		return -EINVAL;
	}

	set_dload_mode(download_mode);

	return 0;
}
#else
#define set_dload_mode(x) do {} while (0)

static void enable_emergency_dload_mode(void)
{
	printk(KERN_ERR "dload mode is not enabled on target\n");
}

static bool get_dload_mode(void)
{
	return false;
}
#endif

void msm_set_restart_mode(int mode)
{
	restart_mode = mode;
}
EXPORT_SYMBOL(msm_set_restart_mode);

static bool scm_pmic_arbiter_disable_supported;
/*
 * Force the SPMI PMIC arbiter to shutdown so that no more SPMI transactions
 * are sent from the MSM to the PMIC.  This is required in order to avoid an
 * SPMI lockup on certain PMIC chips if PS_HOLD is lowered in the middle of
 * an SPMI transaction.
 */
static void halt_spmi_pmic_arbiter(void)
{
	if (scm_pmic_arbiter_disable_supported) {
		pr_crit("Calling SCM to disable SPMI PMIC arbiter\n");
		scm_call_atomic1(SCM_SVC_PWR, SCM_IO_DISABLE_PMIC_ARBITER, 0);
	}
}

static void __msm_power_off(int lower_pshold)
{
//[VY5x] ==> CCI KLog, added by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
	cklc_save_magic(KLOG_MAGIC_POWER_OFF, KLOG_STATE_NONE);
	record_shutdown_time(0x06);
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, added by Jimmy@CCI
	printk(KERN_CRIT "Powering off the SoC\n");
#ifdef CONFIG_MSM_DLOAD_MODE
	set_dload_mode(0);
#endif
	pm8xxx_reset_pwr_off(0);
	qpnp_pon_system_pwr_off(PON_POWER_OFF_SHUTDOWN);

	if (lower_pshold) {
		if (!use_restart_v2()) {
			__raw_writel(0, PSHOLD_CTL_SU);
		} else {
			halt_spmi_pmic_arbiter();
			__raw_writel(0, MSM_MPM2_PSHOLD_BASE);
		}

		mdelay(10000);
		printk(KERN_ERR "Powering off has failed\n");
	}
	return;
}

static void msm_power_off(void)
{
#ifdef CCI_KLOG_ALLOW_FORCE_PANIC	
	if(abnormalflag == ABNORAML_CRASH)
	{
		pr_warn("kernel not allow other act(msm_power_off) after crash\n");		
		return;
	}	
#endif	

	abnormalflag = ABNORAML_POWEROFF;
	__raw_writel(CONFIG_WARMBOOT_NONE, restart_reason);
#ifdef CONFIG_CCI_KLOG	
	*unknowflag = 0;
#ifdef #ifdef CONFIG_CCI_KLOG
	*backupcrashflag = 0;
#endif
#endif
	mb();
	/* MSM initiated power off, lower ps_hold */
	__msm_power_off(1);
}

static void cpu_power_off(void *data)
{
	int rc;
#ifdef CCI_KLOG_ALLOW_FORCE_PANIC
	if(abnormalflag == ABNORAML_CRASH)
	{
		pr_warn("kernel not allow other act(cpu_power_off) after crash\n");		
		return;
	}	
#endif		
	abnormalflag = ABNORAML_POWEROFF;
	__raw_writel(CONFIG_WARMBOOT_NONE, restart_reason);
#ifdef CONFIG_CCI_KLOG	
	*unknowflag = 0;
	*backupcrashflag = 0;
#endif
	mb();
	pr_err("PMIC Initiated shutdown %s cpu=%d\n", __func__,
						smp_processor_id());
	if (smp_processor_id() == 0) {
		/*
		 * PMIC initiated power off, do not lower ps_hold, pmic will
		 * shut msm down
		 */
		__msm_power_off(0);

		pet_watchdog();
		pr_err("Calling scm to disable arbiter\n");
		/* call secure manager to disable arbiter and never return */
		rc = scm_call_atomic1(SCM_SVC_PWR,
						SCM_IO_DISABLE_PMIC_ARBITER, 1);

		pr_err("SCM returned even when asked to busy loop rc=%d\n", rc);
		pr_err("waiting on pmic to shut msm down\n");
	}

	preempt_disable();
	while (1)
		;
}

static irqreturn_t resout_irq_handler(int irq, void *dev_id)
{
	pr_warn("%s PMIC Initiated shutdown\n", __func__);
	oops_in_progress = 1;
	smp_call_function_many(cpu_online_mask, cpu_power_off, NULL, 0);
	if (smp_processor_id() == 0)
		cpu_power_off(NULL);
	preempt_disable();
	while (1)
		;
	return IRQ_HANDLED;
}

#define MSM_IMEM_SIZE 0x0001000
#define CCI_RCOVRY_ON_FLAG_ADDR	MSM_IMEM_BASE + (MSM_IMEM_SIZE - 4)

int read_rcovry_on_flag( char *page, char **start, off_t off, int count, int *eof, void *data )
{
   int len = 0;
   int *bIsOnOff = CCI_RCOVRY_ON_FLAG_ADDR;
   char tmp[5] = {0};

   memcpy(tmp, (char*)bIsOnOff, 4);
   len = sprintf(page, "%s", tmp);

   __raw_writel(0x0, bIsOnOff);
   mb();

   return len;
}

static void msm_restart_prepare(const char *cmd)
{
//[VY5x] ==> CCI KLog, added by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
	char buf[KLOG_MAGIC_LENGTH + 1] = {0};
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, added by Jimmy@CCI
#ifdef CCI_KLOG_ALLOW_FORCE_PANIC
	if(abnormalflag == ABNORAML_CRASH)
	{
		pr_warn("kernel not allow other act(msm_restart_prepare) after crash\n");		
		return;
	}
#endif		

#ifdef CONFIG_CCI_KLOG	
	*unknowflag = 0;
	*backupcrashflag = 0;
#endif

#ifdef CONFIG_MSM_DLOAD_MODE

	/* This looks like a normal reboot at this point. */
	set_dload_mode(0);

	/* Write download mode flags if we're panic'ing */
	set_dload_mode(in_panic);
	if(in_panic)
	{
#ifdef CCI_KLOG_ALLOW_FORCE_PANIC			
		__raw_writel(CONFIG_WARMBOOT_CRASH, restart_reason);
#else
		__raw_writel(CONFIG_WARMBOOT_NORMAL, restart_reason);
#ifdef CONFIG_CCI_KLOG
		*backupcrashflag = CONFIG_WARMBOOT_CRASH;
#endif
#endif	
		abnormalflag = ABNORAML_CRASH;
		mb();
	}
	/* Write download mode flags if restart_mode says so */
	if (restart_mode == RESTART_DLOAD)
//[VY5x] ==> CCI KLog, modified by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
	{
		cklc_save_magic(KLOG_MAGIC_DOWNLOAD_MODE, KLOG_STATE_DOWNLOAD_MODE);
		set_dload_mode(1);
		__raw_writel(CONFIG_WARMBOOT_S1 , restart_reason);		
		mb();
	}
#else // #ifdef CONFIG_CCI_KLOG
		set_dload_mode(1);
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, modified by Jimmy@CCI

	/* Kill download mode if master-kill switch is set */
	if (!download_mode)
		set_dload_mode(0);
#endif

	pm8xxx_reset_pwr_off(1);

	/* Hard reset the PMIC unless memory contents must be maintained. */
#if 0	
	if (get_dload_mode() || (cmd != NULL && cmd[0] != '\0'))
#else
	if (get_dload_mode() || (cmd != NULL))
#endif	
		qpnp_pon_system_pwr_off(PON_POWER_OFF_WARM_RESET);
	else
		qpnp_pon_system_pwr_off(PON_POWER_OFF_HARD_RESET);

	if (cmd != NULL) {
		if (!strncmp(cmd, "bootloader", 10)) {
//[VY5x] ==> CCI KLog, modified by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
			cklc_save_magic(KLOG_MAGIC_BOOTLOADER, KLOG_STATE_NONE);
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, modified by Jimmy@CCI
			__raw_writel(CONFIG_WARMBOOT_FB, restart_reason);
			mb();
		} else if (!strncmp(cmd, "recovery", 8)) {
//[VY5x] ==> CCI KLog, modified by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
			cklc_save_magic(KLOG_MAGIC_RECOVERY, KLOG_STATE_NONE);
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, modified by Jimmy@CCI
			__raw_writel(CONFIG_WARMBOOT_NORMAL, restart_reason);
			__raw_writel(0x59564352, CCI_RCOVRY_ON_FLAG_ADDR); /*YVCR*/
			mb();
//Joker mark for fix merge conflict, may need to fix
//			__raw_writel(0x77665502, restart_reason);
//		} else if (!strcmp(cmd, "rtc")) {
//			__raw_writel(0x77665503, restart_reason);
//Joker mark for fix merge conflict, may need to fix
		} else if (!strncmp(cmd, "oem-", 4)) {
			unsigned long code;
			code = simple_strtoul(cmd + 4, NULL, 16) & 0xff;
//[VY5x] ==> CCI KLog, modified by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
			snprintf(buf, KLOG_MAGIC_LENGTH + 1, "%s%lX", KLOG_MAGIC_OEM_COMMAND, code);
			kprintk("OEM command:code=%lX, buf=%s\n", code, buf);
#ifdef CCI_KLOG_ALLOW_FORCE_PANIC
			switch(code)
			{
				case 0x60 + KLOG_INDEX_INIT % 0x10://0x60, simulate klog init magic
					cklc_save_magic(KLOG_MAGIC_INIT, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_MARM_FATAL % 0x10://0x61, simulate mARM fatal magic
					cklc_save_magic(KLOG_MAGIC_AARM_PANIC, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_AARM_PANIC % 0x10://0x62, simulate aARM panic magic
					cklc_save_magic(KLOG_MAGIC_AARM_PANIC, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_RPM_CRASH % 0x10://0x63, simulate RPM crash magic
					cklc_save_magic(KLOG_MAGIC_RPM_CRASH, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_SUBSYS_CRASH % 0x10://0x64, simulate sub-system crash magic
					cklc_save_magic(KLOG_MAGIC_SUBSYS_CRASH, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_FIQ_HANG % 0x10://0x65, simulate FIQ hang magic
					cklc_save_magic(KLOG_MAGIC_FIQ_HANG, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_UNKNOWN_CRASH % 0x10://0x66, simulate unknown crash magic
					cklc_save_magic(KLOG_MAGIC_UNKNOWN_CRASH, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_DOWNLOAD_MODE % 0x10://0x67, simulate normal download mode magic
					cklc_save_magic(KLOG_MAGIC_DOWNLOAD_MODE, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_POWER_OFF % 0x10://0x68, simulate power off magic
					cklc_save_magic(KLOG_MAGIC_POWER_OFF, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_REBOOT % 0x10://0x69, simulate normal reboot magic
					cklc_save_magic(KLOG_MAGIC_REBOOT, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_BOOTLOADER % 0x10://0x6A, simulate bootloader mode magic
					cklc_save_magic(KLOG_MAGIC_BOOTLOADER, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_RECOVERY % 0x10://0x6B, simulate recovery mode magic
					cklc_save_magic(KLOG_MAGIC_RECOVERY, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_OEM_COMMAND % 0x10://0x6C, simulate oem-command magic with OEM-6C
					cklc_save_magic(KLOG_MAGIC_OEM_COMMAND"6C", KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_APPSBL % 0x10://0x6D, simulate AppSBL magic
					cklc_save_magic(KLOG_MAGIC_APPSBL, KLOG_STATE_NONE);
					break;

				case 0x60 + KLOG_INDEX_FORCE_CLEAR % 0x10://0x6F, simulate force clear magic
					cklc_save_magic(KLOG_MAGIC_FORCE_CLEAR, KLOG_STATE_NONE);
					break;

				default:
					cklc_save_magic(buf, KLOG_STATE_NONE);
					break;
			}
#else // #ifdef CCI_KLOG_ALLOW_FORCE_PANIC
			cklc_save_magic(buf, KLOG_STATE_NONE);
#endif // #ifdef CCI_KLOG_ALLOW_FORCE_PANIC
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, modified by Jimmy@CCI
			__raw_writel(CONFIG_WARMBOOT_NORMAL, restart_reason);
			mb();
		} else if (!strncmp(cmd, "edl", 3)) {
			enable_emergency_dload_mode();
		} 
		else if (!strncmp(cmd, "oemS", 4)) {
			__raw_writel(CONFIG_WARMBOOT_S1, restart_reason);
			mb();
		}
		else if (!strncmp(cmd, "oemF", 4)) {
			__raw_writel(CONFIG_WARMBOOT_FOTA, restart_reason);
			mb();
		} 
		else if (!strncmp(cmd, "hard", 4)) {
			pr_warn("pmic is running on hardreset, it will starting from cold boot\n");
			qpnp_pon_system_pwr_off(PON_POWER_OFF_HARD_RESET);
			__raw_writel(CONFIG_WARMBOOT_NORMAL, restart_reason);
			mb();
		} 
		else{
			__raw_writel(CONFIG_WARMBOOT_NORMAL, restart_reason);
			mb();
//[VY5x] ==> CCI KLog, modified by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
			cklc_save_magic(KLOG_MAGIC_REBOOT, KLOG_STATE_NONE);
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, modified by Jimmy@CCI
		}
	}
//[VY5x] ==> CCI KLog, modified by Jimmy@CCI
#ifdef CONFIG_CCI_KLOG
	else
	{
		__raw_writel(CONFIG_WARMBOOT_NORMAL , restart_reason);		
		mb();
		pr_warn("pmic is running on hardreset, it will starting from cold boot\n");
			cklc_save_magic(KLOG_MAGIC_REBOOT, KLOG_STATE_NONE);
	}
#endif // #ifdef CONFIG_CCI_KLOG
//[VY5x] <== CCI KLog, modified by Jimmy@CCI

	flush_cache_all();
	outer_flush_all();
}

void msm_restart(char mode, const char *cmd)
{
	printk(KERN_NOTICE "Going down for restart now\n");

	msm_restart_prepare(cmd);

	if (!use_restart_v2()) {
		__raw_writel(0, msm_tmr0_base + WDT0_EN);
		if (!(machine_is_msm8x60_fusion() ||
		      machine_is_msm8x60_fusn_ffa())) {
			mb();
			 /* Actually reset the chip */
			__raw_writel(0, PSHOLD_CTL_SU);
			mdelay(5000);
			pr_notice("PS_HOLD didn't work, falling back to watchdog\n");
		}

		__raw_writel(1, msm_tmr0_base + WDT0_RST);
		__raw_writel(5*0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
		__raw_writel(0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
		__raw_writel(1, msm_tmr0_base + WDT0_EN);
	} else {
		/* Needed to bypass debug image on some chips */
		msm_disable_wdog_debug();
		halt_spmi_pmic_arbiter();
		__raw_writel(0, MSM_MPM2_PSHOLD_BASE);
	}

	mdelay(10000);
	printk(KERN_ERR "Restarting has failed\n");
}

static int __init msm_pmic_restart_init(void)
{
	int rc;

	if (use_restart_v2())
		return 0;

	if (pmic_reset_irq != 0) {
		rc = request_any_context_irq(pmic_reset_irq,
					resout_irq_handler, IRQF_TRIGGER_HIGH,
					"restart_from_pmic", NULL);
		if (rc < 0)
			pr_err("pmic restart irq fail rc = %d\n", rc);
	} else {
		pr_warn("no pmic restart interrupt specified\n");
	}

	return 0;
}

late_initcall(msm_pmic_restart_init);

#define RAMDUMP_ZIP_ON_ADDR MSM_IMEM_BASE + (MSM_IMEM_SIZE - 8)
int read_ramdump_zip_on_flag( char *page, char **start, off_t off, int count, int *eof, void *data )
{
   int len = 0;
#ifdef CCI_KLOG_ALLOW_FORCE_PANIC
   int *bIsOnOff = RAMDUMP_ZIP_ON_ADDR;
   char tmp[5] = {0};

   memcpy(tmp, (char*)bIsOnOff, 4);
   len = sprintf(page, "%s", tmp);

   __raw_writel(0x0, bIsOnOff);
   mb();
#else
   len = sprintf(page, "NONE");
#endif
   return len;
}

//quiet reboot
#define CCI_QUIET_REBOOT_ADDR MSM_IMEM_BASE + (MSM_IMEM_SIZE - 12)
int is_quiet_reboot_flag(void)
{
	int result = 0;
#ifndef CCI_KLOG_ALLOW_FORCE_PANIC
	int *bIsQuietReboot = CCI_QUIET_REBOOT_ADDR;
	char tmp[5] = {0};

	memcpy(tmp, (char*)bIsQuietReboot, 4);

	if (strcmp(tmp, "QRBT") == 0)
	{
		result = 1;
	}
#endif
	return result;
}

void set_quiet_reboot_flag(void)
{
       __raw_writel(0x54425251, CCI_QUIET_REBOOT_ADDR); /*QRBT reverse = TBRQ*/
	mb();
}

int read_quiet_reboot_flag( char *page, char **start, off_t off, int count, int *eof, void *data )
{
   int len = 0;
#ifndef CCI_KLOG_ALLOW_FORCE_PANIC
   int *bIsQuietReboot = CCI_QUIET_REBOOT_ADDR;
   char tmp[5] = {0};

   memcpy(tmp, (char*)bIsQuietReboot, 4);
   len = sprintf(page, "%s", tmp);
#endif
   return len;
}

int read_quiet_reboot_flag_erase( char *page, char **start, off_t off, int count, int *eof, void *data )
{
   int len = 0;
#ifndef CCI_KLOG_ALLOW_FORCE_PANIC
   int *bIsQuietReboot = CCI_QUIET_REBOOT_ADDR;
   char tmp[5] = {0};

   memcpy(tmp, (char*)bIsQuietReboot, 4);
   len = sprintf(page, "%s", tmp);

   __raw_writel(0x0, bIsQuietReboot);
   mb();
#endif
   return len;
}

static int __init msm_restart_init(void)
{
#ifdef CONFIG_MSM_DLOAD_MODE
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	dload_mode_addr = MSM_IMEM_BASE + DLOAD_MODE_ADDR;
	emergency_dload_mode_addr = MSM_IMEM_BASE +
		EMERGENCY_DLOAD_MODE_ADDR;
	warm_boot_addr	= MSM_IMEM_BASE + CONFIG_WARMBOOT_MAGIC_ADDR;
	set_dload_mode(download_mode);
#endif
	msm_tmr0_base = msm_timer_get_timer0_base();
	restart_reason = MSM_IMEM_BASE + RESTART_REASON_ADDR;
	pm_power_off = msm_power_off;

	//quiet reboot
	create_proc_read_entry("quiet_reboot_on", 0, NULL, read_quiet_reboot_flag, NULL);
	create_proc_read_entry("quiet_reboot_on_erase", 0, NULL, read_quiet_reboot_flag_erase, NULL);

	set_warmboot();
#ifdef CCI_KLOG_ALLOW_FORCE_PANIC
	__raw_writel(CONFIG_WARMBOOT_CRASH, restart_reason);
#else	
	__raw_writel(CONFIG_WARMBOOT_NORMAL, restart_reason);
#endif	
	mb();

	create_proc_read_entry("ramdump_zip_on", 0, NULL, read_ramdump_zip_on_flag, NULL);
	create_proc_read_entry("rcovry_on", 0, NULL, read_rcovry_on_flag, NULL);

	if (scm_is_call_available(SCM_SVC_PWR, SCM_IO_DISABLE_PMIC_ARBITER) > 0)
		scm_pmic_arbiter_disable_supported = true;

	return 0;
}
early_initcall(msm_restart_init);
