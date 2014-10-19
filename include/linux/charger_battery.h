#ifndef __LINUX_MFD_CHARGER_BATTERY_H
#define __LINUX_MFD_CHARGER_BATTERY_H

enum charger_battery_type {
	CHARGER_TEST						,  /* CHARGER_REGISTER */
	BATTERY_TEST						,  /* BATTERY_REGISTER */
	CHARGER_BATTERY_TYPE_MAX		,
};

enum charger_battery_access_method {
	CHARGER_BATTERY_PARA_READ		,  /* Parameter read */
	CHARGER_BATTERY_PARA_WRITE		,  /* Parameter write */
	CHARGER_BATTERY_REG_PROCESS		,  /* Read register */
	CHARGER_AUTO_CHARGE			,
	BATTERY_GAUGE_TEST			,
};

enum charger_battery_reg_functions {
	CHARGER_BATTERY_REG_READ			, /*Registers read*/
	CHARGER_BATTERY_REG_BULK_READ	, /*Bulk registers read*/
	CHARGER_BATTERY_REG_WRITE		,  /* Write registers */
};

enum charger_auto_functions {
	CHG_TEST_ENABLE 			= 0		,  /*  */
	CHG_CHARGE_THRESHOLD	 	= 4	,
	CHG_DISCHARGE_THRESHOLD	= 12,
	CHG_TEST_FUNC_MAX 	= (CHG_DISCHARGE_THRESHOLD + 8)	,
	CHG_TEST_MODE_VAL = CHG_TEST_FUNC_MAX, 
};

enum charger_reg_definition {
	CHG_REG_LSB	= 0	,
	CHG_REG_MSB 	= 16	, 
	CHG_REG_TEST_MAX 	= (CHG_REG_MSB + 16)	,
};


enum charger_pse_definition {
	CHG_PSE_VAL	= 0	,
	CHG_PSE_FLAG 	= 16	, 
	CHG_PSE_MAX 	= (CHG_PSE_FLAG + 3)	,
};

enum batt_test_functions {
	BATT_TEST_ENABLE		= 0	,
	BATT_FAKE_CAPACITY 			= 4	,  /*  */
	BATT_FAKE_TEMPERATURE 		= 12	,
	BATT_TEST_FUNC_MAX 	= (BATT_FAKE_TEMPERATURE + 8)	,
	BATT_TEST_MODE_VAL = BATT_TEST_FUNC_MAX , 
};

enum batt_reg_definition {
	BATT_REG_LSB	= 0	,
	BATT_REG_MSB 	= 16	, 
	BATT_REG_TEST_MAX 	= (BATT_REG_MSB + 16)	,
};


enum charger_parameters {
	CHARGER_TRICKLE_CHG_VOLTAGE, 
	CHARGER_CV_CHG_VOLTAGE		,  /* charge point */
	CHARGER_CC_CHG_VOLTAGE		,  /* Trickle charge point */
	CHARGER_DELTA_CHG_VOLTAGE		,  //jonny_test delta voltage for resume chg

	CHARGER_INPUT_CURRENT	,  /* MAX input charge current */
	CHARGER_TRICKLE_CHG_CURRENT, 
	CHARGER_CC_CHG_CURRENT		,  /* MAX charge current */
	CHARGER_TERM_CHG_CURRENT, 
	
	CHARGER_PERIODIC_UPDATE_TIME, 
	CHARGER_TOTAL_CHG_MAX_TIME, 

	CHARGER_AC_TOTAL_CHG_MAX_TIME,
	CHARGER_USB_TOTAL_CHG_MAX_TIME,

	CHARGER_TRICKLE_CHG_MAX_TIME, 
	CHARGER_CC_CHG_MAX_TIME, 
	CHARGER_CV_CHG_MAX_TIME, 

	CHARGER_DLIN_IN_TRACKING_MODE, 
	CHARGER_HIGH_TEMP_PROTECTION_ENABLE, 
	CHARGER_LOW_VOLTAGE_PROTECTION_ENABLE, 

	CHARGER_CHG_FSM, 
	CHARGER_CHG_PROTECTION_STATE, 

	CHARGER_FORCE_CHARGE,

	CHARGER_PSE_LEVEL_0,
	CHARGER_PSE_LEVEL_1,
	CHARGER_PSE_LEVEL_2,
	CHARGER_PSE_LEVEL_3,
	CHARGER_PSE_LEVEL_4,
	CHARGER_PSE_LEVEL_5,
	CHARGER_ALL_PARAMETERS , 
	CHARGER_MAX		,
};

enum battery_parameters {
	BATTERY_CAPACITY_NOW, 
	BATTERY_TOTAL_CAPACITY, 
	BATTERY_VOLTAGE, 
	BATTERY_CHARGE_CURRENT, 
	BATTERY_CHARGE_SUSPEND_CURRENT,
	BATTERY_CHARGE_SUSPEND_CURRENT_ENABLE,
	BATTERY_TEMPERATURE, 
	BATTERY_FIRMWARE_VER, 
	BATTERY_HEALTH_STATUS,
	BATTERY_UEVENT_UPDATE, 
	BATTERY_SAFETY_IS_TRIGGERED, 
	BATTERY_CHARGING_STATE, 
	BATTERY_PRESENT,
	BATTERY_VALID,
	BATTERY_ALL_PARAMETERS , 
	BATTERY_MAX			,
};


enum pse_degree_parameters {
	PSE_60_DEGREE, 
	PSE_45_TO_60_DEGREE,
	PSE_10_TO_45_DEGREE,
	PSE_0_TO_10_DEGREE,
	PSE_MINUS_10_TO_0_DEGREE,
	PSE_MINUS_10_DEGREE, 
	PSE_MAX			,
};

enum pse_threshold_parameters {
	PSE_CURR_TRICKLE, 
	PSE_CURR_OSBL,
	PSE_CURR_SDLR,
	PSE_CURR_UI,
	PSE_VOLT_EOC,
	PSE_VOLT_RECHG, 
	PSE_VOLT_MAINTAIN, 
	PSE_THODSHOLD_MAX			,
};

struct charger_battery_parameters {
	int 	parameters;
	u16  	addr;
};


typedef struct  {
	u16 		parameters;
	u16  	curr_3p2V;
	u16  	curr_3p2_3p4V;
	u16  	curr_3p4_3p7V;
	u16  	curr_3p7V;
	u16        volt_eoc;
	u16        volt_rechg;
	u16        volt_maintain;
}pse_parameters, *ppse_parameters;

typedef  struct {
	u32 		trickle;
	u32  	osbl;
	u32  	sldr;
}pse_voltage_threshold, *ppse_voltage_threshold;



typedef int (*charger_battery_callback)(enum charger_battery_access_method method, int para, unsigned int set_val, void *data);
extern int charger_battery_register(enum charger_battery_type type, charger_battery_callback cb, void *data);
extern u16 charger_battery_mask_val(int size);
extern int chg_func_test_val_read(enum charger_auto_functions para);
extern int batt_func_test_val_read(enum batt_test_functions para);

#endif
