/*===========================================================================
File: kernel/kernel/power/suspend_configure.c
Description: suspend dynamic configure for gpio and regulator

Revision History:
when       	who				what, where, why
--------   	---				---------------------------------------------------------
04/16/12	KevinA_Lin	Initial version
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
#include <linux/uaccess.h>
//ERB uese
#include <mach/irqs.h>
#include <mach/gpiomux.h>
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8038.h>
//ERB uese
/*add subfunction for regulator match*/
#include <linux/regulator/consumer.h>
#define MAX_BUF_LEN 50
#define MAX_BUF_NUM 100
#define NR_MSM_GPIOS 117 //modify for msm8x26 use
#define PMIC_NR_GPIOS 8 //modify for pm8x26 use 

static unsigned int suspend_config_enable = 0;
unsigned int msm_gpio_suspend_config_last = 0;
unsigned int pmic_gpio_suspend_config_last = 0;
unsigned int pmic_mpp_suspend_config_last = 0;
unsigned int regulator_suspend_config_last = 0;
static DEFINE_MUTEX(msm_gpio_buf_mutex);
static DEFINE_MUTEX(pmic_gpio_buf_mutex);
static DEFINE_MUTEX(pmic_mpp_buf_mutex);
static DEFINE_MUTEX(regulator_buf_mutex);
static struct dentry *suspend_config_debugfs_dir;

struct suspend_config {
	unsigned gpio;
	unsigned dir;
	unsigned val;
	/*regulator*/
	char regulator[20];
	unsigned mode;
	unsigned optimum_mode;
	unsigned force_disable;
	unsigned enable;
	int min;
	int max;
	/*regulator*/
};

static struct msm_gpiomux_config msm_gpio_suspend_config[MAX_BUF_NUM];
static struct suspend_config pmic_gpio_suspend_config[MAX_BUF_NUM];
static struct suspend_config pmic_mpp_suspend_config[MAX_BUF_NUM];
static struct suspend_config regulator_suspend_config[MAX_BUF_NUM];

//dynamic configure enable
static int set_enable(void *data, u64 val)
{
	suspend_config_enable = (unsigned int)val;
	return 0;
}

static int get_enable(void *data, u64 * val)
{
	int ret;

	ret = suspend_config_enable;
	*val = ret;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(enable_fops, get_enable, set_enable, "%llu\n");

static int suspend_config_msm_gpio_show(struct seq_file *s, void *unused)
{
	return 0;
}

static int suspend_config_msm_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, suspend_config_msm_gpio_show, NULL);
}

static int suspend_config_pmic_gpio_show(struct seq_file *s, void *unused)
{
	return 0;
}

static int suspend_config_pmic_gpio_open(struct inode *inode, struct file *file)
{
	return single_open(file, suspend_config_pmic_gpio_show, NULL);
}

static int suspend_config_pmic_mpp_show(struct seq_file *s, void *unused)
{
	return 0;
}

static int suspend_config_pmic_mpp_open(struct inode *inode, struct file *file)
{
	return single_open(file, suspend_config_pmic_mpp_show, NULL);
}

static int suspend_config_regulator_show(struct seq_file *s, void *unused)
{
	return 0;
}

static int suspend_config_regulator_open(struct inode *inode, struct file *file)
{
	return single_open(file, suspend_config_regulator_show, NULL);
}

/*dynamic msm_gpio configure*/
static ssize_t suspend_config_msm_gpio_write(
	struct file *file,
	const char __user *buf,
	size_t count,
	loff_t *ppos)
{
	int filled = 0;
	int gpio = 0, dir = 0, func = 0, drv = 0, pull = 0; 
	static char msm_gpio_buf[MAX_BUF_LEN];
	static struct gpiomux_setting msm_gpio_buf_setting;

	if (count < MAX_BUF_LEN) {
		mutex_lock(&msm_gpio_buf_mutex);

		if (copy_from_user(msm_gpio_buf, buf, count))
			return -EFAULT;

		msm_gpio_buf[count] = '\0';
		filled = sscanf(msm_gpio_buf, "%d %d %d %d %d", &gpio, &dir, &func, &drv, &pull);
		pr_info("%s(): gpio =%d , dir =%d, fun =%d, drv =%d, pull=%d\n", __func__, gpio, dir, func, drv, pull);
		msm_gpio_buf_setting.dir = dir;
		msm_gpio_buf_setting.func = func;
		msm_gpio_buf_setting.drv = drv;
		msm_gpio_buf_setting.pull = pull;
		mutex_unlock(&msm_gpio_buf_mutex);
		/* check that user entered five numbers */
		if (filled < 5 || dir < 0 || func < 0 || drv < 0 || pull < 0) {
			pr_info("Error, correct format: 'echo gpio dir hi_lo fun drive pull'");
			return -ENOMEM;
		} else {
				pr_info("success");
				msm_gpio_suspend_config[msm_gpio_suspend_config_last].gpio = gpio;
				msm_gpio_suspend_config[msm_gpio_suspend_config_last].settings[1] = &msm_gpio_buf_setting;
				msm_gpio_suspend_config_last++;
		}
	} else {
		pr_err("Error-Input gpio state"
				" string exceeds maximum buffer length");

		return -ENOMEM;
	}

	return count;
}
/*dynamic pmic_gpio configure*/
static ssize_t suspend_config_pmic_gpio_write(
	struct file *file,
	const char __user *buf,
	size_t count,
	loff_t *ppos)
{
	int filled = 0;
	int gpio = 0, dir = 0, val = 0; 
	static char pmic_gpio_buf[MAX_BUF_LEN];
	
	if (count < MAX_BUF_LEN) {
		mutex_lock(&pmic_gpio_buf_mutex);

		if (copy_from_user(pmic_gpio_buf, buf, count))
			return -EFAULT;

		pmic_gpio_buf[count] = '\0';
		filled = sscanf(pmic_gpio_buf, "%d %d %d", &gpio, &dir, &val);
		pr_info("%s(): gpio =%d , dir =%d, val =%d\n", __func__, gpio, dir, val);
		mutex_unlock(&pmic_gpio_buf_mutex);
		/* check that user entered three numbers */
		if (filled < 3 || dir < 0 || val < 0) {
			pr_info("Error, correct format: 'echo gpio dir val'");
			return -ENOMEM;
		} else {
				pr_info("success");
				pmic_gpio_suspend_config[pmic_gpio_suspend_config_last].gpio = gpio;
				pmic_gpio_suspend_config[pmic_gpio_suspend_config_last].dir = dir;
				pmic_gpio_suspend_config[pmic_gpio_suspend_config_last].val = val;
				pmic_gpio_suspend_config_last++;
		}
	} else {
		pr_err("Error-Input gpio state"
				" string exceeds maximum buffer length");

		return -ENOMEM;
	}
	
	return count;
}

static ssize_t suspend_config_pmic_mpp_write(
	struct file *file,
	const char __user *buf,
	size_t count,
	loff_t *ppos)
{
	int filled = 0;
	int gpio = 0, dir = 0, val = 0; 
	static char pmic_mpp_buf[MAX_BUF_LEN];
	
	if (count < MAX_BUF_LEN) {
		mutex_lock(&pmic_mpp_buf_mutex);

		if (copy_from_user(pmic_mpp_buf, buf, count))
			return -EFAULT;

		pmic_mpp_buf[count] = '\0';
		filled = sscanf(pmic_mpp_buf, "%d %d %d", &gpio, &dir, &val);
		pr_info("%s(): gpio =%d , dir =%d, val =%d\n", __func__, gpio, dir, val);
		mutex_unlock(&pmic_mpp_buf_mutex);
		/* check that user entered three numbers */
		if (filled < 3 || dir < 0 || val < 0) {
			pr_info("Error, correct format: 'echo gpio dir val'");
			return -ENOMEM;
		} else {
				pr_info("success");
				pmic_mpp_suspend_config[pmic_mpp_suspend_config_last].gpio = gpio;
				pmic_mpp_suspend_config[pmic_mpp_suspend_config_last].dir = dir;
				pmic_mpp_suspend_config[pmic_mpp_suspend_config_last].val = val;

				pmic_mpp_suspend_config_last++;
		}
	} else {
		pr_err("Error-Input gpio state"
				" string exceeds maximum buffer length");

		return -ENOMEM;
	}

	return count;
}
/*dynamic regulator configure*/
static ssize_t suspend_config_regulator_write(
	struct file *file,
	const char __user *buf,
	size_t count,
	loff_t *ppos)
{
	char regulator[20];
	int filled = 0;
	int regulator_match = 0;
	int mode = 0, force_disable = 0, enable = 0, optimum_mode = 0, min = 0, max = 0 ;
	static char regulator_buf[MAX_BUF_LEN];

	if (count < MAX_BUF_LEN) {
		mutex_lock(&regulator_buf_mutex);

		if (copy_from_user(regulator_buf, buf, count))
			return -EFAULT;

		regulator_buf[count] = '\0';
		filled = sscanf(regulator_buf, "%s %d %d %d %d %d %d", regulator, &mode, &optimum_mode, &force_disable, &enable, &min, &max);

		mutex_unlock(&regulator_buf_mutex);
		/* check that user entered 7 numbers */
		  if (filled < 7 || mode < -1 || optimum_mode < -1 || force_disable < -1 || enable < -1 || min < -1 || max < -1 || max < min) {
			pr_info("Error, correct format: 'echo regulator mode optimum_mode force_disable enable min max");
			return -ENOMEM;
		} else {
		/* check input regulator have the same with regulator_map_list*/
			regulator_match = regulator_map_list_match(regulator);
			if(regulator_match) {
				strncpy(regulator_suspend_config[regulator_suspend_config_last].regulator, regulator, strlen(regulator));
				regulator_suspend_config[regulator_suspend_config_last].regulator[strlen(regulator)] = '\0';
				regulator_suspend_config[regulator_suspend_config_last].mode = mode;
				regulator_suspend_config[regulator_suspend_config_last].optimum_mode = optimum_mode;
				regulator_suspend_config[regulator_suspend_config_last].force_disable = force_disable;
				regulator_suspend_config[regulator_suspend_config_last].enable = enable;
				regulator_suspend_config[regulator_suspend_config_last].min = min;
				regulator_suspend_config[regulator_suspend_config_last].max = max;
				regulator_suspend_config_last++;
			}
		}
	} else {
		pr_err("Error-Input gpio state"
				" string exceeds maximum buffer length");

		return -ENOMEM;
	}

	return count;
}
static int suspend_config_suspend_info_suspend(void)
{
	unsigned int i;
	int rc = 0;
	int min, max;
	int err_mode, err_optimum_mode, err_force_disable, err_enable, err_voltage;
	struct regulator *reg;

	if (suspend_config_enable) {
		//msm gpio suspend set
		for (i=0; i < msm_gpio_suspend_config_last; i++) {	
			rc = msm_gpiomux_write(msm_gpio_suspend_config[i].gpio, 1, msm_gpio_suspend_config[i].settings[1], NULL);
			pr_info("%s(): config gpio = %d\n",  __func__, msm_gpio_suspend_config[i].gpio);
		}
		//pmic gpio suspend set
		for  (i = 0; i < pmic_gpio_suspend_config_last; i++) {
			if (pmic_gpio_suspend_config[i].dir) {
				gpio_direction_output(NR_MSM_GPIOS + pmic_gpio_suspend_config[i].gpio,  pmic_gpio_suspend_config[i].val);
				pr_info("%s(): config output pmic gpio= %d\n",  __func__, NR_MSM_GPIOS + pmic_gpio_suspend_config[i].gpio);
			} else {
				gpio_direction_input(NR_MSM_GPIOS + pmic_gpio_suspend_config[i].gpio );
				pr_info("%s(): config input pmic gpio= %d\n",  __func__, NR_MSM_GPIOS + pmic_gpio_suspend_config[i].gpio);
			}
		}
		//pmic mpp suspend set
		for  (i = 0; i < pmic_mpp_suspend_config_last; i++) {
			if (pmic_mpp_suspend_config[i].dir) {
				gpio_direction_output(NR_MSM_GPIOS + PMIC_NR_GPIOS + pmic_mpp_suspend_config[i].gpio, pmic_mpp_suspend_config[i].val);
				pr_info("%s(): config output MPP= %d\n",  __func__, NR_MSM_GPIOS + PMIC_NR_GPIOS + pmic_mpp_suspend_config[i].gpio);
			} else {
				gpio_direction_input(NR_MSM_GPIOS + PMIC_NR_GPIOS + pmic_mpp_suspend_config[i].gpio );
				pr_info("%s(): config input MPP= %d\n",  __func__, NR_MSM_GPIOS + PMIC_NR_GPIOS + pmic_mpp_suspend_config[i].gpio);
			}
		}
		//regulator suspend set
		for  (i = 0; i < regulator_suspend_config_last; i++) {
			reg = regulator_get(NULL, regulator_suspend_config[i].regulator);
			//enable
			if (regulator_suspend_config[i].enable == 1) {
				err_enable = regulator_enable(reg);
				pr_info("%s(): config regulator enable\n",  __func__);
			} else if (regulator_suspend_config[i].enable == 0) {
				err_enable = regulator_disable(reg);
				pr_info("%s(): config regulator disable ",  __func__);
			}
			//mode : qualcomm: fast 0x1(no use) normal 0x2 idle 0x4 standby 0x8
			if (regulator_suspend_config[i].mode != -1) {
				err_mode = regulator_set_mode(reg, regulator_suspend_config[i].mode);
				pr_info("%s(): config regulator mode= %d\n",  __func__, regulator_suspend_config[i].mode);
			}
			//optimum_mode : uA_load: load current
			if (regulator_suspend_config[i].optimum_mode != -1) {
				err_optimum_mode = regulator_set_optimum_mode(reg, regulator_suspend_config[i].optimum_mode);
				pr_info("%s(): config regulator optimum_mode\n",  __func__);
			}
			//force_disable
			if(regulator_suspend_config[i].force_disable == 1) {
				err_force_disable = regulator_force_disable(reg);
				pr_info("%s(): config regulator force_disable\n",  __func__);
			}
			//voltage 
			min = regulator_suspend_config[i].min;
			max = regulator_suspend_config[i].max;
			if (min >= 0 && max >= 0) {
				err_voltage = regulator_set_voltage(reg, min, max);
				pr_info("%s(): config regulator voltage min = %d, max = %d\n",  __func__, min, max);
			}
		}
	}
	return 0;
}

/*suspned info resume*/
static void suspend_config_suspend_info_resume(void)
{
	msm_gpio_suspend_config_last = 0;
	pmic_gpio_suspend_config_last = 0;
	pmic_mpp_suspend_config_last = 0;
	regulator_suspend_config_last = 0;
}

/*suspned info dbgshow*/
static void suspend_config_suspend_info_dbg_show(struct seq_file *s)
{

}

static struct suspend_info_ops suspend_config_info_ops = {
	.suspend = suspend_config_suspend_info_suspend,
	.resume = suspend_config_suspend_info_resume,
	.dbg_show = suspend_config_suspend_info_dbg_show,
};

static const struct file_operations msm_gpio_debugfs_fops = {
	.open = suspend_config_msm_gpio_open,
	.release = single_release,
	.write = suspend_config_msm_gpio_write,
	.read = seq_read,
	.llseek = seq_lseek,
 	.release = single_release,
	.owner = THIS_MODULE,
};

static const struct file_operations pmic_gpio_debugfs_fops = {
	.open = suspend_config_pmic_gpio_open,
	.release = single_release,
	.write = suspend_config_pmic_gpio_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.owner = THIS_MODULE,
};

static const struct file_operations pmic_mpp_debugfs_fops = {
	.open = suspend_config_pmic_mpp_open,
	.release = single_release,
	.write = suspend_config_pmic_mpp_write,
	.read = seq_read,
 	.llseek = seq_lseek,
 	.release = single_release,
	.owner = THIS_MODULE,
};

static const struct file_operations regulator_debugfs_fops = {
	.open = suspend_config_regulator_open,
	.release = single_release,
	.write = suspend_config_regulator_write,
	.read = seq_read,
 	.llseek = seq_lseek,
 	.release = single_release,
	.owner = THIS_MODULE,
};

static int __init suspend_config_init(void)
{
	register_suspend_info_ops(&suspend_config_info_ops);

	suspend_config_debugfs_dir = debugfs_create_dir("suspend_configure", NULL);
	if (IS_ERR_OR_NULL(suspend_config_debugfs_dir))
		pr_err("%s():Cannot create debugfs dir\n", __func__);
	//suspend dynamci configure enable
	debugfs_create_file("enable", 0644, suspend_config_debugfs_dir, NULL, &enable_fops);	
	//msm gpio confiugre	
	debugfs_create_file("msm_gpio", 0644, suspend_config_debugfs_dir, NULL, &msm_gpio_debugfs_fops);
	//pmic gpio configue	
	debugfs_create_file("pmic_gpio", 0644, suspend_config_debugfs_dir, NULL, &pmic_gpio_debugfs_fops);
	//mpp configure
	debugfs_create_file("pmic_mpp", 0644, suspend_config_debugfs_dir, NULL, &pmic_mpp_debugfs_fops);
	//regulator configure
	debugfs_create_file("regulator", 0644, suspend_config_debugfs_dir, NULL, &regulator_debugfs_fops);
	
	pr_info("%s\n", __func__);

	return 0;
}

static void suspend_config_exit(void)
{
	debugfs_remove_recursive(suspend_config_debugfs_dir);
	unregister_suspend_info_ops(&suspend_config_info_ops);
}

late_initcall(suspend_config_init);
module_exit(suspend_config_exit);

