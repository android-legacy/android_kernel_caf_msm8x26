/*===========================================================================
File: kernel\drivers\cci\ddr_param_dump\ddr_param_dump.c
Description: Dump the DDR parameter value from physical register

Revision History:
when		who				what, where, why
--------	---				----------------------------------------------------
12/03/13	Grace Chang		Initial version
===========================================================================*/

#include <linux/module.h>  
#include <linux/moduleparam.h>  
#include <linux/debugfs.h>  
#include <linux/err.h>  
#include <linux/init.h>  
#include <linux/kernel.h>  
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/msm_iomap.h>

#include "ddr_param_dump.h"

static struct dentry *ddr_param_dump_debugfs_dir;
ddrdumpinfo_t ddrdumpinfo;


static void ddr_param_dump_get_dumpinfo(ddrdumpinfo_t* ddr_param_info) 
{
	ddr_param_info->ddrsection = ddr_param_section;
	ddr_param_info->ddrsection_num = ddr_param_num;
}

static int ddr_param_dump_get_data(void)
{
	int i = 0;
	void __iomem *ddr_reg_phys_addr = 0;
	unsigned int ddr_reg_val = 0;
	pr_info("In %s(): enter\n", __func__ );

	ddr_param_dump_get_dumpinfo(&ddrdumpinfo);

	for (i = 0; i < ddrdumpinfo.ddrsection_num; i++) {
		ddr_reg_phys_addr = ioremap( ddrdumpinfo.ddrsection[i].ddr_param_reg_addr, sizeof(long) );
		if (!ddr_reg_phys_addr) {
			pr_err("%s: Could not remap 0x%X\n", __func__, ddrdumpinfo.ddrsection[i].ddr_param_reg_addr );
			return -1;
		}

		if ( ddrdumpinfo.ddrsection[i].ddr_param_mask != 0 ) {
			ddr_reg_val = ( readl(ddr_reg_phys_addr) & ddrdumpinfo.ddrsection[i].ddr_param_mask ) >> (ddrdumpinfo.ddrsection[i].ddr_param_offset*4);
			pr_info("%s: [%s] 0x%X\n", __func__, ddrdumpinfo.ddrsection[i].ddr_param_name, ddr_reg_val);
			ddrdumpinfo.ddrsection[i].ddr_param_val = ddr_reg_val;
		}
		iounmap(ddr_reg_phys_addr);
	}

	pr_info("In %s(): exit\n", __func__ );
	return 0;
}

/* to show current ddr_param_dump status when cat the debug node in adb shell:
	[step 1] enter "mount -t debugfs none /sys/kernel/debug"
	[step 2] enter "cat /sys/kernel/debug/ddr_param_dump/ddr_param_info"
*/
static int ddr_param_dump_info_show(struct seq_file *s, void *unused)
{
	int i = 0;
	unsigned int ddr_reg_val = 0;

	seq_printf(s, "\n*** MSM8x26 DDR Parameters Dump ***\n");		

	ddr_param_dump_get_data();

	for (i = 0; i < ddrdumpinfo.ddrsection_num; i++) {
		ddr_reg_val = ddrdumpinfo.ddrsection[i].ddr_param_val;
		seq_printf(s, "%2d.  %s\t= 0x%X\t= %d\n", i, ddrdumpinfo.ddrsection[i].ddr_param_name, ddr_reg_val, ddr_reg_val);	
	}

	return 0;
}

static int ddr_param_dump_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, ddr_param_dump_info_show, NULL);
}

static const struct file_operations ddr_param_dump_info_debug_fops = {
	.open	= ddr_param_dump_info_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release	= single_release,
};

static int __init ddr_param_dump_init(void)
{
	ddr_param_dump_debugfs_dir = debugfs_create_dir("ddr_param_dump", 0);
	if (IS_ERR(ddr_param_dump_debugfs_dir)) {
		pr_err("%s(): Cannot create debugfs dir\n", __func__);
		return 0;
	}

	debugfs_create_file("ddr_param_info", 0644, ddr_param_dump_debugfs_dir, NULL, &ddr_param_dump_info_debug_fops);

	pr_crit("%s\n", __func__);
	return 0;
}

static void __exit ddr_param_dump_exit(void)
{
	debugfs_remove_recursive(ddr_param_dump_debugfs_dir);
	pr_crit("%s\n", __func__);
}

module_init(ddr_param_dump_init);
module_exit(ddr_param_dump_exit);

MODULE_DESCRIPTION("kernel ddr parameter dump driver");
MODULE_AUTHOR("Grace Chang <grace_chang@compalcomm.com>");
MODULE_LICENSE("GPL v2");

