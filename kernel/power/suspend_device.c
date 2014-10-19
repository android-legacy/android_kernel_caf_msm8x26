/*===========================================================================
File: kernel/kernel/power/suspend_device.c
Description: Central suspend/resume debug mechanism for peripheral devices sleep mode state 

Revision History:
when       	who     	  	  what, where, why
--------   	---     	  	  ---------------------------------------------------------
06/15/12   	Changhan Tsai  Initial version
06/10/13   	Changhan Tsai  Add to support peripheral device debug information
===========================================================================*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <asm/hardware/gic.h>
#include <linux/spinlock.h>
#include <mach/msm_iomap.h>
#include <mach/gpio.h>
#include <linux/suspend_info.h>
#include <linux/suspend_device.h>
#include <linux/seq_file.h>

static LIST_HEAD(suspend_dev_ops_list);
static DEFINE_MUTEX(suspend_dev_ops_lock);

struct suspend_dev_info {
	char *name;
	enum suspend_dev_state suspend_state;
	enum suspend_dev_state resume_state;
};

static struct suspend_dev_info dev_info[(DEV_MAX-1)];
static struct suspend_dev_info dev_current_info[(DEV_MAX-1)];
static unsigned int is_suspend_device_valid = 0;

void suspend_device_register(struct suspend_dev_ops *ops)
{
	mutex_lock(&suspend_dev_ops_lock);
	list_add_tail(&ops->node, &suspend_dev_ops_list);
	mutex_unlock(&suspend_dev_ops_lock);
}
EXPORT_SYMBOL_GPL(suspend_device_register);

void suspend_device_unregister(struct suspend_dev_ops *ops)
{
	mutex_lock(&suspend_dev_ops_lock);
	list_del(&ops->node);
	mutex_unlock(&suspend_dev_ops_lock);
}
EXPORT_SYMBOL_GPL(suspend_device_unregister);

/* Changhan, add the test code */
void suspend_device_get_gpio_data(unsigned int *state, char *name[][24])
{
	struct suspend_dev_ops *ops;
	unsigned int i, gpio; 
       char buf[24];
       char *pbuf = buf;

	list_for_each_entry(ops, &suspend_dev_ops_list, node)
		for (i = 0; i < ops->gpio->ngpios; i++) {
			gpio = (((ops->gpio->gpios->state) >> 4) & 0x3FF);
			state[gpio] = (unsigned int)(ops->gpio->gpios->state);
			pbuf += sprintf(pbuf, "%s: ", ops->name);
			pbuf += sprintf(pbuf, "%s", ops->gpio->gpios->name);
			strncpy(name[gpio][24], &buf[0], strlen(buf));
		}
}
EXPORT_SYMBOL_GPL(suspend_device_get_gpio_data);

void suspend_device_notify_state(enum suspend_dev id, char *name, 
	enum suspend_dev_suspend_resume suspend_resume, 
	enum suspend_dev_state state)
{
	if (id >=DEV_MAX)
		return;

	dev_info[id].name = name;
	if (suspend_resume == DEV_SUSPEND) {
		dev_info[id].suspend_state = state;
	} else if (suspend_resume == DEV_RESUME) {
		dev_info[id].resume_state = state;
	}
}
EXPORT_SYMBOL_GPL(suspend_device_notify_state);

static void suspend_device_get_suspend_state(void)
{
	struct suspend_dev_ops *ops;

	list_for_each_entry(ops, &suspend_dev_ops_list, node)
		{
		dev_current_info[(ops->id)].suspend_state = ops->suspend_state;
		dev_current_info[(ops->id)].name = ops->name;
		}
}

static void suspend_device_get_resume_state(void)
{
	struct suspend_dev_ops *ops;

	list_for_each_entry(ops, &suspend_dev_ops_list, node)
		{
		dev_current_info[(ops->id)].resume_state = ops->resume_state;
		dev_current_info[(ops->id)].name = ops->name;
		}
}

static void suspend_device_set_suspend_info(void)
{
	suspend_device_get_suspend_state();
	is_suspend_device_valid = 1;
}

static int print_suspend_device_state(struct seq_file *s, void *unused)
{
	struct suspend_dev_ops *ops;
	int i, total_actma = 0, total_lowpwr_ma = 0;

	seq_printf(s, "Peripheral device current state \n");		
	seq_printf(s, "id   name        mode    mode_paras   func   cmd   active_ma   lowpwr_ma \n");		
	seq_printf(s, "================================================\n");		

	list_for_each_entry(ops, &suspend_dev_ops_list, node)
		{
		total_actma += (ops->active_ma->actma[(ops->mode_paras)]);
		total_lowpwr_ma += (ops->lowpwr_ma);
		seq_printf(s, "%2d     [%s]        %2d       %2d  %2d  %2d  %2d   %2d\n", 
		ops->id, ops->name, ops->mode, ops->mode_paras, ops->func, ops->cmd, 
		ops->active_ma->actma[(ops->mode_paras)], ops->lowpwr_ma);
		}

	seq_printf(s, "================================================\n");		
	seq_printf(s, "total active ma: %4d \n", total_actma);		
	seq_printf(s, "total lowpwr ma: %4d \n", total_lowpwr_ma);		

	if (is_suspend_device_valid) {
		seq_printf(s, "Peripheral device suspend state \n");		
		seq_printf(s, "id      name      suspend_state      resume_state \n");		
		seq_printf(s, "================================================\n");		

		suspend_device_get_resume_state();
		
		for (i = 0; i < DEV_MAX; i++) {
			if (dev_info[i].name != NULL) {
				seq_printf(s, "%2d     [%s]        %2d       %2d\n", 
					i, dev_current_info[i].name, dev_current_info[i].suspend_state, dev_current_info[i].resume_state);
			}
		}
	}

	return 0;
}

static int suspend_device_info_suspend(void)
{
	suspend_device_set_suspend_info();

	return 0;
}

static void suspend_device_info_resume(void)
{

}

static void suspend_device_info_dbg_show(struct seq_file *s)
{
	print_suspend_device_state(s, NULL);
}

static struct suspend_info_ops suspend_device_info_ops = {
	.suspend = suspend_device_info_suspend,
	.resume = suspend_device_info_resume,
	.dbg_show = suspend_device_info_dbg_show,
};

static int __init suspend_device_init(void)
{
	memset(&dev_info, 0 , sizeof(dev_info));
	memset(&dev_current_info, 0 , sizeof(dev_current_info));
	register_suspend_info_ops(&suspend_device_info_ops);

	return 0;
}
module_init(suspend_device_init);

