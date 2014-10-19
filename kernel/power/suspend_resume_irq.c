/*===========================================================================
File: kernel/kernel/power/suspend_resume_irq.c
Description: suspend/resume show irq mechanism

Revision History:
when       	who			what, where, why
--------   	---			---------------------------------------------------------
06/19/12	KevinA_Lin		Initial version
===========================================================================*/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/suspend_info.h>
#include <linux/syscore_ops.h>
#include <linux/irq.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>

struct suspend_resume_irq {
	unsigned int count;
};

/*Integrate resume irq time record debug */
struct suspend_resume_irq_timestamp {
	int irq;
	int count;	
	ktime_t timestamp;
	s64 timeduration;
};

struct suspend_resume_irq_pmic_timestamp {
	int irq;
	int count;	
	ktime_t timestamp;
	s64 timeduration;
	uint8_t pmic_slave;
	uint8_t pmic_per;
	uint8_t pmic_irq;
	uint8_t hwirq;
};


static unsigned int is_suspend_resume_irq_valid = 0;
static struct suspend_resume_irq sri_current_info[NR_IRQS];
static struct suspend_resume_irq sri_resume_info[NR_IRQS];
/*Integrate pmic resume irq  debug */
#define qpnp_slave_size 15
#define qpnp_per_size 255
#define qpnp_irq_size 7
static struct suspend_resume_irq sri_pmic_current_info[qpnp_slave_size][qpnp_per_size][qpnp_irq_size];
/*Integrate pmic resume irq  debug */

/*Integrate resume irq time record debug */
#define SRI_TIMESTAMP_SIZE 100
static int sri_timestamp_index = 0;
static struct suspend_resume_irq_timestamp sri_timestamp_info[SRI_TIMESTAMP_SIZE];
/*bypass pmic timestamp mechanism*/
//#define SRI_PMIC_TIMESTAMP_SIZE 100
//static int sri_pmic_timestamp_index = 0;
//static struct suspend_resume_irq_pmic_timestamp sri_pmic_timestamp_info[SRI_PMIC_TIMESTAMP_SIZE];
/*bypass pmic timestamp mechanism*/

static int suspend_resume_irq_buf_clear(void *data, u64 val)
{

	if (val) {
		memset(&sri_resume_info[0], 0 , sizeof(sri_resume_info));
		memset(&sri_current_info[0], 0 , sizeof(sri_current_info));

		/*Integrate resume irq time record debug */
			memset(&sri_timestamp_info[0], 0 , sizeof(sri_timestamp_info));
			sri_timestamp_index = 0;
			sri_timestamp_info[sri_timestamp_index].irq = 0;
			sri_timestamp_info[sri_timestamp_index].count = 0;
			sri_timestamp_info[sri_timestamp_index].timestamp =  ktime_get_boottime();
			sri_timestamp_index += 1;
		/*Integrate resume irq time record debug */
		/*Integrate pmic resume irq  debug */
		memset(&sri_pmic_current_info[0], 0 , sizeof(sri_pmic_current_info));
		/*Integrate pmic resume irq  debug */
		/*bypass pmic timestamp mechanism*/
			#if 0
			memset(&sri_pmic_timestamp_info[0], 0 , sizeof(sri_pmic_timestamp_info));
			sri_pmic_timestamp_index = 0;
			sri_pmic_timestamp_info[sri_pmic_timestamp_index].pmic_slave = 0;
			sri_pmic_timestamp_info[sri_pmic_timestamp_index].pmic_per = 0;
			sri_pmic_timestamp_info[sri_pmic_timestamp_index].pmic_irq = 0;
			sri_pmic_timestamp_info[sri_pmic_timestamp_index].hwirq = 0;
			sri_pmic_timestamp_info[sri_pmic_timestamp_index].irq = 0;
			sri_pmic_timestamp_info[sri_pmic_timestamp_index].count = 0;
			sri_pmic_timestamp_info[sri_pmic_timestamp_index].timestamp = ktime_get_boottime();
			sri_pmic_timestamp_index += 1;
			#endif 
		/*bypass pmic timestamp mechanism*/
		pr_info("%s(): resume irq mem clear\n",__func__); 
	}

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(reset_suspend_resume_irq_fops, NULL, suspend_resume_irq_buf_clear, "%llu\n");

void suspned_resume_irq_write(int irq)
{
	sri_current_info[irq].count ++; 
	
	/*Integrate resume irq time record debug */
#ifdef CONFIG_SONY_EAGLE
		sri_timestamp_info[sri_timestamp_index].irq = irq;
		sri_timestamp_info[sri_timestamp_index].count = sri_current_info[irq].count;
		sri_timestamp_info[sri_timestamp_index].timestamp = ktime_get_boottime();
		sri_timestamp_index += 1;
#endif
	/*Integrate resume irq time record debug */

	pr_info("%s():sri_current_info[%d].count = %d\n", __func__, irq, sri_current_info[irq].count);
}
EXPORT_SYMBOL_GPL(suspned_resume_irq_write);
/*Integrate pmic resume irq  debug */
void suspned_resume_irq_pmic_write(uint8_t pmic_slave, uint8_t pmic_per, uint8_t pmic_irq, unsigned long hwirq, int irq)
{
	sri_pmic_current_info[pmic_slave][pmic_per][pmic_irq].count ++; 

	#if 0 
	sri_pmic_timestamp_info[sri_pmic_timestamp_index].pmic_slave = pmic_slave;
	sri_pmic_timestamp_info[sri_pmic_timestamp_index].pmic_per = pmic_per;
	sri_pmic_timestamp_info[sri_pmic_timestamp_index].pmic_irq = pmic_irq;
	sri_pmic_timestamp_info[sri_pmic_timestamp_index].hwirq = hwirq;
	sri_pmic_timestamp_info[sri_pmic_timestamp_index].irq = irq;
	sri_pmic_timestamp_info[sri_pmic_timestamp_index].count = 
		sri_pmic_current_info[pmic_slave][pmic_per][pmic_irq].count;
	sri_pmic_timestamp_info[sri_pmic_timestamp_index].timestamp = ktime_get_boottime();
	sri_pmic_timestamp_index += 1;
	#endif 

	pr_info("%s():pmic_slave= %d , pmic_per = %d , pmic_irq = %d , hwirq = %d, irq = %d, sri_pmic_current_info.count = %d\n", __func__, pmic_slave , pmic_per , pmic_irq , 
		(int)hwirq, irq , sri_pmic_current_info[pmic_slave][pmic_per][pmic_irq].count);
		
}
EXPORT_SYMBOL_GPL(suspned_resume_irq_pmic_write);
/*Integrate pmic resume irq  debug */
static void suspend_resume_irq_get_resume_state(void)
{
	int i; 
	for (i = 0; i < NR_IRQS; i++) {
		if (sri_current_info[i].count) {
			pr_info("%s():sri_current_info[%d].count = %d\n",  __func__, i, sri_current_info[i].count);
			sri_resume_info[i].count = sri_current_info[i].count;
		}
	}
}

static void suspend_resume_irq_set_resume_info(void)
{
	suspend_resume_irq_get_resume_state();
	is_suspend_resume_irq_valid = 1;
}

static int print_suspend_resume_irq_state(struct seq_file *s, void *unused)
{
	int i, j, k;
	long timestamp_init_ms, timestamp_current_ms, timeduration_ms; 
	s64 timestamp_init_ms64;
	s64 timestamp_current_ms64 ;
	s64 timeduration_ms64;
	
	seq_printf(s, "============================================\n");

	if (is_suspend_resume_irq_valid) {	
		seq_printf(s, "resume irq wakeup state record:\n");
		seq_printf(s, "resume_irq_name             resume_count\n");	

		for (i = 0; i < NR_IRQS; i++) {
			if (sri_current_info[i].count) {
				pr_info("%s():sri_resume_info[%d].count = %d\n", __func__, i, sri_resume_info[i].count);
				seq_printf(s, "resume_irq[%d]:             		%d\n", i, sri_resume_info[i].count);
			}
		}
		/*Integrate pmic resume irq  debug */
		for (i=0; i < qpnp_slave_size; i++)
			for(j=0; j < qpnp_per_size; j++)
				for(k=0; k < qpnp_irq_size; k ++) {
					if(sri_pmic_current_info[i][j][k].count) {
						pr_info("%s():sri_pmic_current_info[%d][%d][%d].count = %d\n", __func__, i, j, k, sri_pmic_current_info[i][j][k].count);
						seq_printf(s, "pmic_resume_irq[%d][%d][%d] :             		%d\n", i, j, k, sri_pmic_current_info[i][j][k].count);
						}
		}
		/*Integrate pmic resume irq  debug */
		/*Integrate resume irq time record debug */
			seq_printf(s, "\n");
			seq_printf(s, "resume_irq_timestamp information:\n");
			timestamp_init_ms64 = ktime_to_ns(sri_timestamp_info[0].timestamp);
			do_div(timestamp_init_ms64, NSEC_PER_MSEC);
			timestamp_init_ms = (long)timestamp_init_ms64;
			seq_printf(s, "resume_irq_init  timestamp=%8ld.%-5lds\n", timestamp_init_ms / MSEC_PER_SEC, timestamp_init_ms % MSEC_PER_SEC);
			for (i = 1; i < sri_timestamp_index; i++) {
				timestamp_current_ms64 = ktime_to_ns(sri_timestamp_info[i].timestamp);
				do_div(timestamp_current_ms64, NSEC_PER_MSEC);
				timestamp_current_ms = (long)timestamp_current_ms64;
				timeduration_ms64 =  sri_timestamp_info[i].timeduration;
				do_div(timeduration_ms64, NSEC_PER_MSEC);
				timeduration_ms = (long )timeduration_ms64;
				seq_printf(s, "resume_irq[%3d]  count=%4d  timestamp=%8ld.%-5lds  timestamp_dff=%8ld.%-5lds  timeduration=%8ld.%-5lds\n", 
					sri_timestamp_info[i].irq, 
					sri_timestamp_info[i].count, 
					timestamp_current_ms / MSEC_PER_SEC,
					timestamp_current_ms % MSEC_PER_SEC,
					(timestamp_current_ms - timestamp_init_ms) / MSEC_PER_SEC,
					(timestamp_current_ms - timestamp_init_ms) % MSEC_PER_SEC,
					timeduration_ms / MSEC_PER_SEC,
					timeduration_ms % MSEC_PER_SEC);
			}
		/*Integrate resume irq time record debug */
		
	} else {
		seq_printf(s, "Not get into suspend state yet\n");
	}
	seq_printf(s, "============================================\n");

	return 0;
}

 static int suspend_resume_irq_info_suspend(void)
{ 
	s64 timestamp_start, timestamp_end;

	timestamp_start = ktime_to_ns(sri_timestamp_info[sri_timestamp_index-1].timestamp);
	timestamp_end = ktime_to_ns(ktime_get_boottime());
	// timeduration nsec
	sri_timestamp_info[sri_timestamp_index-1].timeduration = timestamp_end - timestamp_start;
	return 0;
}

static void suspend_resume_irq_info_resume(void)
{
	suspend_resume_irq_set_resume_info();

}

static void suspend_resume_irq_info_dbg_show(struct seq_file *s)
{
	print_suspend_resume_irq_state(s, NULL);
}

static struct suspend_info_ops suspend_resume_irq_info_ops = {
	.suspend = suspend_resume_irq_info_suspend,
	.resume = suspend_resume_irq_info_resume,
	.dbg_show = suspend_resume_irq_info_dbg_show,
};

static int __init suspend_resume_irq_init(void)
{
	memset(&sri_resume_info, 0 , sizeof(sri_resume_info));
	memset(&sri_current_info, 0 , sizeof(sri_current_info));
	register_suspend_info_ops(&suspend_resume_irq_info_ops);
	debugfs_create_file("reset_suspend_resume_irq", 0644, NULL, NULL, &reset_suspend_resume_irq_fops);
	return 0;
}
module_init(suspend_resume_irq_init);

