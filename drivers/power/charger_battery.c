#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/charger_battery.h>
#include <linux/debugfs.h>


static struct dentry *charger_battery_info_debugfs_dir;

struct charger_battery_info {
    charger_battery_callback cb;
    void *data;
};

static struct charger_battery_info parameter_info[CHARGER_BATTERY_TYPE_MAX] ;


#define CHG_AUTO_TEST_MODE_DEFAULT 0x00064000;
#define CHG_AUTO_TEST_MODE_DEFAULT_LENGTH 32
static unsigned  int chg_test_mode_status;

#define CHG_REG_TEST_DEFAULT 0x00000000; //27 degree and 50 persent capacity
#define CHG_REG_TEST_DEFAULT_LENGTH 32
static unsigned  int chg_reg_test_status;

#define BATT_TEST_MODE_DEFAULT 0x0001B320; //27 degree and 50 persent capacity
#define BATT_TEST_MODE_DEFAULT_LENGTH 32
static unsigned  int batt_test_mode_status;

#define BATT_REG_TEST_DEFAULT 0x00000000; //27 degree and 50 persent capacity
#define BATT_REG_TEST_DEFAULT_LENGTH 32
static unsigned  int batt_reg_test_status;


struct charger_battery_debugfs {
	char		*name;
	int		val;
};

static struct charger_battery_debugfs battery_para[] = {
	{"batt_para_total_capacity", 		BATTERY_CAPACITY_NOW},
	{"batt_para_remain_capacity", 		BATTERY_TOTAL_CAPACITY},	
	{"batt_para_batt_voltage", 			BATTERY_VOLTAGE},
	{"batt_para_batt_current",			BATTERY_CHARGE_CURRENT},
	{"batt_para_batt_suspend_current",			BATTERY_CHARGE_SUSPEND_CURRENT},
	{"batt_para_suspend_enable ",		BATTERY_CHARGE_SUSPEND_CURRENT_ENABLE},
	{"batt_para_batt_temperature", 		BATTERY_TEMPERATURE},
	{"batt_para_gauge_firmware_ver",	BATTERY_FIRMWARE_VER},
	{"batt_para_health_status", 		BATTERY_HEALTH_STATUS},
	{"batt_para_uevent_update", 		BATTERY_UEVENT_UPDATE},
	{"batt_para_safety_is_triggered", 	BATTERY_SAFETY_IS_TRIGGERED},
	{"batt_para_charging_state", 		BATTERY_CHARGING_STATE},
	{"batt_para_present", 				BATTERY_PRESENT},
	{"batt_para_batt_valid", 			BATTERY_VALID},
	{"batt_para_read_all_para", 			BATTERY_ALL_PARAMETERS},
};

static struct charger_battery_debugfs battery_reg[] = {
	{"batt_reg_write", 				CHARGER_BATTERY_REG_WRITE},
	{"batt_reg_read", 				CHARGER_BATTERY_REG_READ},	
	{"batt_reg_bulk_read", 			CHARGER_BATTERY_REG_BULK_READ},
};

static struct charger_battery_debugfs battery_func[] = {
	{"batt_test_enable", 				BATT_TEST_ENABLE},
	{"batt_test_fake_capacity", 			BATT_FAKE_CAPACITY},	
	{"batt_test_fake_temperature", 		BATT_FAKE_TEMPERATURE},
	{"batt_test_mode_val", 			BATT_TEST_MODE_VAL},
};

static struct charger_battery_debugfs charger_para[] = {
	{"chg_para_trkckle_charge_voltage", 		CHARGER_TRICKLE_CHG_VOLTAGE},
	{"chg_para_cc_voltage", 				CHARGER_CC_CHG_VOLTAGE},	
	{"chg_para_cv_voltage", 				CHARGER_CV_CHG_VOLTAGE},
	{"chg_para_delta_voltage", 				CHARGER_DELTA_CHG_VOLTAGE}, 
	{"chg_para_input_current",				CHARGER_INPUT_CURRENT},
	{"chg_para_trickle_current", 				CHARGER_TRICKLE_CHG_CURRENT},
	{"chg_para_cc_current",					CHARGER_CC_CHG_CURRENT},
	{"chg_para_terminate_current", 			CHARGER_TERM_CHG_CURRENT},
	{"chg_para_period_update_time", 		CHARGER_PERIODIC_UPDATE_TIME},
	{"chg_para_max_total_charge_time", 		CHARGER_TOTAL_CHG_MAX_TIME},
	{"chg_para_max_ac_total_charge_time", 	CHARGER_AC_TOTAL_CHG_MAX_TIME},
	{"chg_para_max_usb_total_charge_time", 	CHARGER_USB_TOTAL_CHG_MAX_TIME},
	{"chg_para_max_trkckle_charge_time", 	CHARGER_TRICKLE_CHG_MAX_TIME},
	{"chg_para_max_cc_charge_time", 		CHARGER_CC_CHG_MAX_TIME},
	{"chg_para_max_cv_charge_time", 		CHARGER_CV_CHG_MAX_TIME},
	{"chg_para_dlin_during_tracking_mode",	CHARGER_DLIN_IN_TRACKING_MODE,},
	{"chg_para_high_temperature_thr", 		CHARGER_HIGH_TEMP_PROTECTION_ENABLE},
	{"chg_para_low_voltage_thr", 			CHARGER_LOW_VOLTAGE_PROTECTION_ENABLE},
	{"chg_para_charging_FSM", 				CHARGER_CHG_FSM},
	{"chg_para_protection_state", 			CHARGER_CHG_PROTECTION_STATE},
	{"chg_para_force_charge", 				CHARGER_FORCE_CHARGE},
	{"chg_para_pse_level_0", 				CHARGER_PSE_LEVEL_0},
	{"chg_para_pse_level_1", 				CHARGER_PSE_LEVEL_1},
	{"chg_para_pse_level_2", 				CHARGER_PSE_LEVEL_2},
	{"chg_para_pse_level_3", 				CHARGER_PSE_LEVEL_3},
	{"chg_para_pse_level_4", 				CHARGER_PSE_LEVEL_4},
	{"chg_para_pse_level_5", 				CHARGER_PSE_LEVEL_5},
	{"chg_para_read_all_para", 				CHARGER_ALL_PARAMETERS},	
};

static struct charger_battery_debugfs charger_reg[] = {
	{"chg_reg_write", 				CHARGER_BATTERY_REG_WRITE},
	{"chg_reg_read", 				CHARGER_BATTERY_REG_READ},	
	{"chg_reg_bulk_read", 			CHARGER_BATTERY_REG_BULK_READ},
};

static struct charger_battery_debugfs charger_func[] = {
	{"chg_test_mode_enable", 				CHG_TEST_ENABLE},
	{"chg_test_discharge_threshold", 		CHG_DISCHARGE_THRESHOLD},	
	{"chg_test_charge_threshold", 			CHG_CHARGE_THRESHOLD},
	{"chg_test_mode_val", 					CHG_TEST_MODE_VAL},
};

u16 charger_battery_mask_val(int size)
{
	u16 mask_val = 1; // pow(2,0)
	int i = 0;
	
	for (i = 1; i<size;i++) {
		mask_val = mask_val | (mask_val*2);
	};

	return mask_val;
}
static int chg_para_get(void *data, u64 * val)
{
	int ret;
	int para = (int)data;
	
	charger_battery_callback cb = parameter_info[CHARGER_TEST].cb;

	if (para == CHARGER_ALL_PARAMETERS) {
		for (para =0; para < CHARGER_ALL_PARAMETERS;para++) {
			ret = cb(CHARGER_BATTERY_PARA_READ, para, 0, parameter_info[CHARGER_TEST].data);
		}
	} else {
		ret = cb(CHARGER_BATTERY_PARA_READ, para, 0, parameter_info[CHARGER_TEST].data);
	}
	*val = ret;
	return 0;
}

static int chg_para_set(void *data, u64 val)
{
	int para = (int)data;

	charger_battery_callback cb = parameter_info[CHARGER_TEST].cb;
	pr_info("%s, val is %d\n", __func__, (unsigned int)val);
	cb(CHARGER_BATTERY_PARA_WRITE, para, (unsigned int)val, parameter_info[CHARGER_TEST].data);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(chg_para_fops, chg_para_get, chg_para_set, "0x%02llx\n");


int chg_func_test_val_read(enum charger_auto_functions para)
{
	int is_enabled = 0, discharge_thr = 100, charge_thr = 0;
	int retval =0;
	u16 mask_val = 1;

	mask_val = charger_battery_mask_val(CHG_TEST_ENABLE);
	is_enabled = chg_test_mode_status & mask_val;
	if (is_enabled == 1) {
		mask_val = charger_battery_mask_val(CHG_DISCHARGE_THRESHOLD - CHG_CHARGE_THRESHOLD);
		charge_thr = (chg_test_mode_status >> CHG_CHARGE_THRESHOLD) & mask_val;	
		
		mask_val = charger_battery_mask_val(CHG_TEST_FUNC_MAX - CHG_DISCHARGE_THRESHOLD);
		discharge_thr = (chg_test_mode_status >> CHG_DISCHARGE_THRESHOLD) & mask_val;
	} else {
		return -1;
	}

	switch (para) {
	case CHG_TEST_ENABLE :
		retval = is_enabled;
	break;

	case CHG_CHARGE_THRESHOLD:
		retval = charge_thr;
	break;

	case CHG_DISCHARGE_THRESHOLD:
		retval = discharge_thr;
	break;
	
	default : 

	break;
	}
	
	return retval;	
}
static int chg_func_test_val_get(void *data, u64 *val)
{
	unsigned char test_case = (int)data;
	u16 mask_val = 1;
	
	switch (test_case) {
	case CHG_TEST_ENABLE :
		mask_val = charger_battery_mask_val(CHG_TEST_ENABLE);
		*val = chg_test_mode_status & mask_val;
	break;

	case CHG_CHARGE_THRESHOLD:
		mask_val = charger_battery_mask_val(CHG_CHARGE_THRESHOLD - CHG_TEST_ENABLE);
		*val = (chg_test_mode_status >> test_case) & mask_val;
	break;

	case CHG_DISCHARGE_THRESHOLD:
		mask_val = charger_battery_mask_val(CHG_TEST_FUNC_MAX - CHG_CHARGE_THRESHOLD);
		*val = (chg_test_mode_status >> test_case) & mask_val;
	break;

	default : 
		*val = chg_test_mode_status;
	break;
	}
	
	pr_info("%s, charger auto test status is 0x%8X\n", __func__, chg_test_mode_status);
	return 0;
}

static int chg_func_test_val_set(void *data, u64 val)
{
	int ret = 0;
	unsigned char test_case = (int)data;
	unsigned int temp_val = (unsigned int) val;
	unsigned int temp_val01 = 0, temp_val02 = 0;
	charger_battery_callback cb = parameter_info[CHARGER_TEST].cb;

	pr_debug("%s [S], test_case is %d, temp_val is 0x%8X, charger auto test status is 0x%8X\n", __func__, 
		test_case, temp_val, chg_test_mode_status);
		
	//shift the value we enter
	temp_val = temp_val << test_case;
	
	switch (test_case) {
	case CHG_TEST_ENABLE : 
		//clear to 0
		temp_val01 = chg_test_mode_status >> (test_case + 1 );
		temp_val01 = temp_val01 << (test_case +1 );
	break;
	#if 0
	case CHG_FORCE_CHARGE : 
		temp_val01 = chg_test_mode_status >> (test_case + 1 );
		temp_val01 = temp_val01 << (test_case + 1 );		
	break;
	#endif
	case CHG_CHARGE_THRESHOLD : 
		temp_val01 = chg_test_mode_status >> (test_case + 8 );
		temp_val01 = temp_val01 << (test_case + 8 );		
	break;
	
	case CHG_DISCHARGE_THRESHOLD : 
		temp_val01 = chg_test_mode_status >> (test_case + 8 );
		temp_val01 = temp_val01 << (test_case + 8 );		
	break;
	
	default : 
		chg_test_mode_status = CHG_AUTO_TEST_MODE_DEFAULT;
	break;
	}
	
	temp_val02 = chg_test_mode_status << (CHG_AUTO_TEST_MODE_DEFAULT_LENGTH - test_case );
	pr_debug(" temp_val is 0x%8X, temp_val01 is 0x%8X, temp_val02 is 0x%8X, \n",
		temp_val, temp_val01, temp_val02);	
	
	temp_val02 = temp_val02 >> (CHG_AUTO_TEST_MODE_DEFAULT_LENGTH - test_case );
	pr_debug(" temp_val is 0x%8X, temp_val01 is 0x%8X, temp_val02 is 0x%8X, \n",
		temp_val, temp_val01, temp_val02);
	

	chg_test_mode_status = (temp_val | temp_val01) | (temp_val02);
	
	pr_debug("%s [E], test_case is %d, temp_val is 0x%8X, charger auto test status is 0x%8X\n", __func__, 
		test_case, temp_val, chg_test_mode_status);
	
	ret = cb(CHARGER_AUTO_CHARGE, 0x00, chg_test_mode_status, parameter_info[CHARGER_TEST].data);
	ret =0;
	
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(chg_func_test_val_fops, chg_func_test_val_get, chg_func_test_val_set, "0x%02llx\n");

static void chg_val_init(void)
{
	chg_test_mode_status = CHG_AUTO_TEST_MODE_DEFAULT;
	chg_reg_test_status = CHG_REG_TEST_DEFAULT;
}

static int chg_reg_get(void *data, u64 * val)
{
	unsigned int val_temp = chg_reg_test_status;
	*val = val_temp;
	pr_info("%s, chg_test status is 0x%8X\n", __func__, chg_reg_test_status);
	return 0;
}

static int chg_reg_set(void *data, u64 val)
{
	int ret = 0;
	unsigned int i =0;
	unsigned char test_case = (int)data;
	unsigned int mask_val = 1, max_reg = 0, min_reg = 0;
	charger_battery_callback cb = parameter_info[CHARGER_TEST].cb;
	
	chg_reg_test_status = (unsigned int) val;
	pr_info("%s [S], Method is %d, temp_val is 0x%8X, chg test status is 0x%8X\n", __func__, 
		test_case, (unsigned int)val, chg_reg_test_status);

	switch (test_case) {
	case CHARGER_BATTERY_REG_READ: 
	case CHARGER_BATTERY_REG_WRITE: 
		ret = cb(CHARGER_BATTERY_REG_PROCESS, test_case, chg_reg_test_status, parameter_info[CHARGER_TEST].data);
	break;

	case CHARGER_BATTERY_REG_BULK_READ :
		mask_val = charger_battery_mask_val(CHG_REG_MSB - CHG_REG_LSB);
		min_reg = chg_reg_test_status & mask_val;

		mask_val = charger_battery_mask_val(CHG_REG_TEST_MAX - CHG_REG_MSB);
		max_reg = (chg_reg_test_status >> CHG_REG_MSB) & mask_val;


		pr_info("%s [E], reg_min is %d, reg_max is %d\n", __func__, 
		min_reg, max_reg);
		
		if (max_reg >= min_reg) {
			for (i = min_reg; i <= max_reg; i++) {
				ret = cb(CHARGER_BATTERY_REG_PROCESS, CHARGER_BATTERY_REG_READ, i, parameter_info[CHARGER_TEST].data);
				printk(KERN_NOTICE "Register 0x%8X, val is 0x%8X\n",i, ret);
			}
		} else {

		}

	break;

	default : 
		chg_reg_test_status = CHG_REG_TEST_DEFAULT;
	break;
	}
	
	pr_debug("%s [E], test_case is %d, chg test status is 0x%8X\n", __func__, 
		test_case, chg_reg_test_status);
	
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(chg_reg_fops, chg_reg_get, chg_reg_set, "0x%02llx\n");
static void create_charger_debugfs_entries(void)
{
	struct dentry *charger_battery_info_debugfs_sub_dir;
	int debug_node_num  = 0;
	int i = 0;

	charger_battery_info_debugfs_sub_dir = debugfs_create_dir("charger", charger_battery_info_debugfs_dir);
	
	if (IS_ERR_OR_NULL(charger_battery_info_debugfs_sub_dir))
		pr_err("%s():Cannot create debugfs dir\n", __func__);
	
	/*Parameters based*/
	debug_node_num  = ARRAY_SIZE(charger_para);
	for (i = 0; i < debug_node_num; i++)
		debugfs_create_file(charger_para[i].name, 0644, charger_battery_info_debugfs_sub_dir,
			(void *)charger_para[i].val, &chg_para_fops);

	/*Register based*/
	debug_node_num  = ARRAY_SIZE(charger_reg);
	for (i = 0; i < debug_node_num; i++)
		debugfs_create_file(charger_reg[i].name, 0644, charger_battery_info_debugfs_sub_dir,
			(void *)charger_reg[i].val, &chg_reg_fops);

	/*Engineer  testing mode*/
	debug_node_num  = ARRAY_SIZE(charger_func);
	for (i = 0; i < debug_node_num; i++)
		debugfs_create_file(charger_func[i].name, 0644, charger_battery_info_debugfs_sub_dir,
			(void *)charger_func[i].val, &chg_func_test_val_fops);

	/*Initialize*/
	chg_val_init();
	
}

static int batt_para_get(void *data, u64 * val)
{
	int ret;
	int para = (int)data;
	
	charger_battery_callback cb = parameter_info[BATTERY_TEST].cb;
	
	if (para == BATTERY_ALL_PARAMETERS) {
		for (para =BATTERY_CAPACITY_NOW; para < BATTERY_ALL_PARAMETERS;para++) {
			ret = cb(CHARGER_BATTERY_PARA_READ, para, 0, parameter_info[BATTERY_TEST].data);		
		}
	} else {
		ret = cb(CHARGER_BATTERY_PARA_READ, para, 0, parameter_info[BATTERY_TEST].data);	
	}

	*val = ret;
	return 0;
}

static int batt_para_set(void *data, u64 val)
{
	int para = (int)data;

	charger_battery_callback cb = parameter_info[BATTERY_TEST].cb;
	pr_info("%s, val is %d\n", __func__, (unsigned int)val);
	cb(CHARGER_BATTERY_PARA_WRITE, para, (unsigned int)val, parameter_info[BATTERY_TEST].data);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(batt_para_fops, batt_para_get, batt_para_set, "0x%02llx\n");

static int batt_reg_get(void *data, u64 * val)
{
	unsigned int val_temp = batt_reg_test_status;
	*val = val_temp;
	pr_info("%s, batt_test status is 0x%8X\n", __func__, batt_reg_test_status);
	return 0;
}

static int batt_reg_set(void *data, u64 val)
{
	int ret = 0;
	unsigned int i =0;
	unsigned char test_case = (int)data;
	unsigned int mask_val = 1, max_reg = 0, min_reg = 0;
	charger_battery_callback cb = parameter_info[BATTERY_TEST].cb;
	
	batt_reg_test_status = (unsigned int) val;
	pr_info("%s [S], Method is %d, temp_val is 0x%8X, batt test status is 0x%8X\n", __func__, 
		test_case, (unsigned int)val, batt_reg_test_status);

	switch (test_case) {
	case CHARGER_BATTERY_REG_READ: 
	case CHARGER_BATTERY_REG_WRITE: 
		ret = cb(CHARGER_BATTERY_REG_PROCESS, test_case, batt_reg_test_status, parameter_info[BATTERY_TEST].data);
	break;

	case CHARGER_BATTERY_REG_BULK_READ :
		mask_val = charger_battery_mask_val(BATT_REG_MSB - BATT_REG_LSB);
		pr_info(" min mask is 0x%x", mask_val);
		min_reg = batt_reg_test_status & mask_val;

		mask_val = charger_battery_mask_val(BATT_REG_TEST_MAX - BATT_REG_MSB);
		pr_info(" max mask is 0x%x", mask_val);
		max_reg = (batt_reg_test_status >> BATT_REG_MSB) & mask_val;
		

		pr_info(" [%s] reg_min is 0x%4X, reg_max is 0x%4X\n",__func__,
			min_reg, max_reg);

		
		if (max_reg >= min_reg) {
			for (i = min_reg; i <= max_reg; i++) {
				ret = cb(CHARGER_BATTERY_REG_PROCESS, CHARGER_BATTERY_REG_READ, i, parameter_info[BATTERY_TEST].data);
				pr_info( "Register 0x%8X, val is 0x%8X\n",i, ret);
			}
		} else {

		}

	break;

	default : 
		batt_reg_test_status = BATT_REG_TEST_DEFAULT;
	break;
	}
	
	pr_info("%s [E], test_case is %d, batt test status is 0x%8X\n", __func__, 
		test_case, batt_reg_test_status);
	
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(batt_reg_fops, batt_reg_get, batt_reg_set, "0x%02llx\n");

int batt_func_test_val_read(enum batt_test_functions para)
{
	int is_enabled = 0, fake_capacity = -1, fake_temperature= -1;
	int retval =0;
	u16 mask_val = 1;
	
	mask_val = charger_battery_mask_val(BATT_TEST_ENABLE);
	is_enabled = batt_test_mode_status & mask_val;
	
	if (is_enabled == 1) {
		mask_val = charger_battery_mask_val(BATT_FAKE_TEMPERATURE - BATT_FAKE_CAPACITY);
		fake_capacity = (batt_test_mode_status >> BATT_FAKE_CAPACITY) & mask_val; 

		mask_val = charger_battery_mask_val(BATT_TEST_FUNC_MAX - BATT_FAKE_TEMPERATURE);
		fake_temperature= ((batt_test_mode_status >> BATT_FAKE_TEMPERATURE) & 0xFF)*10;
	} else {
		return -1;
	}

	switch (para) {
	case BATT_TEST_ENABLE :
		retval = is_enabled;
	break;

	case BATT_FAKE_CAPACITY:
		retval = fake_capacity;
	break;

	case BATT_FAKE_TEMPERATURE:
		retval = fake_temperature;
	break;

	default : 
		
	break;

	}

	pr_info("[%s] para is %d, fake_val is %d ",__func__, para, retval);
	return retval;	
}
static int batt_func_test_val_get(void *data, u64 *val)
{
	unsigned int val_temp = batt_test_mode_status;
	unsigned char test_case = (int)data;
	u16 mask_val = 1;

	switch (test_case) {
	case BATT_TEST_ENABLE :
		mask_val = charger_battery_mask_val(BATT_TEST_ENABLE);
		val_temp = batt_test_mode_status & mask_val;
	break;

	case BATT_FAKE_CAPACITY:
		mask_val = charger_battery_mask_val(BATT_FAKE_TEMPERATURE - BATT_FAKE_CAPACITY);
		val_temp = (batt_test_mode_status >> test_case) & mask_val; 
	break;

	case BATT_FAKE_TEMPERATURE:
		mask_val = charger_battery_mask_val(BATT_TEST_FUNC_MAX - BATT_FAKE_TEMPERATURE);
		val_temp= (batt_test_mode_status >> test_case) & mask_val;
	break;

	default : 
		//need to implement for all values in batt_test_mode_status
	break;

	}
	*val = val_temp;
	pr_info("%s, batt_test status is 0x%8X\n", __func__, batt_test_mode_status);
	return 0;
}

static int batt_func_test_val_set(void *data, u64 val)
{
	int ret = 0;
	unsigned char test_case = (int)data;
	unsigned int temp_val = (unsigned int) val;
	unsigned int temp_val01 = 0, temp_val02 = 0;
	charger_battery_callback cb = parameter_info[BATTERY_TEST].cb;

	pr_info("%s [S], test_case is %d, temp_val is 0x%8X, batt test status is 0x%8X\n", __func__, 
		test_case, temp_val, batt_test_mode_status);
	
	//shift the value we enter
	temp_val = temp_val << test_case;
	
	switch (test_case) {
	case BATT_TEST_ENABLE : 
		//clear to 0
		temp_val01 = batt_test_mode_status >> (test_case + 1 );
		temp_val01 = temp_val01 << (test_case +1 );
	break;
	
	case BATT_FAKE_CAPACITY: 
		temp_val01 = batt_test_mode_status >> (test_case + 8 );
		temp_val01 = temp_val01 << (test_case + 8 );	
	break;

	case BATT_FAKE_TEMPERATURE: 
		temp_val01 = batt_test_mode_status >> (test_case + 8 );
		temp_val01 = temp_val01 << (test_case + 8 );		
	break;
	
	default : 
		batt_test_mode_status = BATT_TEST_MODE_DEFAULT;
	break;
	}

	temp_val02 = batt_test_mode_status << (BATT_TEST_MODE_DEFAULT_LENGTH - test_case );
	pr_info(" temp_val is 0x%8X, temp_val01 is 0x%8X, temp_val02 is 0x%8X, \n",
		temp_val, temp_val01, temp_val02);	
	
	temp_val02 = temp_val02 >> (BATT_TEST_MODE_DEFAULT_LENGTH - test_case );
	pr_info(" temp_val is 0x%8X, temp_val01 is 0x%8X, temp_val02 is 0x%8X, \n",
		temp_val, temp_val01, temp_val02);

	batt_test_mode_status = (temp_val | temp_val01) | (temp_val02);
	
	pr_info("%s [E], test_case is %d, temp_val is 0x%8X, batt test status is 0x%8X\n", __func__, 
		test_case, temp_val, batt_test_mode_status);
	
	ret = cb(BATTERY_GAUGE_TEST, 0x00, batt_test_mode_status, parameter_info[BATTERY_TEST].data);
	ret =0;
	
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(batt_func_test_val_fops, batt_func_test_val_get, batt_func_test_val_set, "0x%02llx\n");

static void batt_val_init(void)
{
	batt_test_mode_status = BATT_TEST_MODE_DEFAULT;
	batt_reg_test_status = BATT_REG_TEST_DEFAULT;
}

static void create_battery_debugfs_entries(void)
{
	struct dentry *charger_battery_info_debugfs_sub_dir;
	int debug_node_num  = 0;
	int i = 0;
	
	charger_battery_info_debugfs_sub_dir = debugfs_create_dir("battery", charger_battery_info_debugfs_dir);
	
	if (IS_ERR_OR_NULL(charger_battery_info_debugfs_sub_dir))
		pr_err("%s():Cannot create debugfs dir\n", __func__);

	/*Parameters based*/
	debug_node_num  = ARRAY_SIZE(battery_para);
	for (i = 0; i < debug_node_num; i++)
		debugfs_create_file(battery_para[i].name, 0644, charger_battery_info_debugfs_sub_dir,
			(void *)battery_para[i].val, &batt_para_fops);

	/*Register based*/
	debug_node_num  = ARRAY_SIZE(battery_reg);
	for (i = 0; i < debug_node_num; i++)
		debugfs_create_file(battery_reg[i].name, 0644, charger_battery_info_debugfs_sub_dir,
			(void *)battery_reg[i].val, &batt_reg_fops);

	/*Engineer  testing mode*/
	debug_node_num  = ARRAY_SIZE(battery_func);
	for (i = 0; i < debug_node_num; i++)
		debugfs_create_file(battery_func[i].name, 0644, charger_battery_info_debugfs_sub_dir,
			(void *)battery_func[i].val, &batt_func_test_val_fops);
	
	/*Initialize*/
	batt_val_init();
}

int charger_battery_register(enum charger_battery_type type, charger_battery_callback cb, void *data)
{
	
	
	if (type >= CHARGER_BATTERY_TYPE_MAX || cb == NULL)
		return -1;

	pr_info("%s\n", __func__);
	
	if (parameter_info[type].cb == NULL){
		parameter_info[type].cb = cb;
		parameter_info[type].data = data;

		if (type == CHARGER_TEST) {
			/*Charger*/
			create_charger_debugfs_entries();
		} else if (type == BATTERY_TEST) {
			/*Battery and gauge*/
			create_battery_debugfs_entries();
		} else {
			pr_err("%s The type %d is undefined\n", __func__, type);
			return -1;
		}
		
	} else {
		pr_err("%s The type %d is registered\n", __func__, type);
		return -1;
	}
	
    return 0;
}
EXPORT_SYMBOL_GPL(charger_battery_register);


static int charger_battery_dynamic_control_init(void)
{
	charger_battery_info_debugfs_dir = debugfs_create_dir("charger_battery_info", NULL);
	if (IS_ERR_OR_NULL(charger_battery_info_debugfs_dir))
		pr_err("%s():Cannot create debugfs dir\n", __func__);

	pr_debug("%s\n", __func__);

	return 0;
}

static void charger_battery_dynamic_control_exit(void)
{
	debugfs_remove_recursive(charger_battery_info_debugfs_dir);
	pr_debug("%s\n", __func__);
}


module_init(charger_battery_dynamic_control_init);
module_exit(charger_battery_dynamic_control_exit);

MODULE_AUTHOR("Jonny_Chan <jonny_chan@compalcomm.com>");
MODULE_DESCRIPTION("Dynamic charger and battery/gauge parameters control");
MODULE_LICENSE("GPL v2");
