/*===========================================================================
File: kernel/include/linux/suspend_device.h
Description: Central suspend/resume debug mechanism for peripheral devices (gpio/regulator/wake-up) 

Revision History:
when       	who     	  	  what, where, why
--------   	---     	  	  ---------------------------------------------------------
04/16/12   	Changhan Tsai  Initial version
===========================================================================*/

#ifndef _LINUX_SUSPEND_DEVICE_H
#define _LINUX_SUSPEND_DEVICE_H

#include <linux/list.h>

enum suspend_dev {
	DEV_NONE = 0,
	DEV_DDR,
	DEV_EMMC,
	DEV_USB,
	DEV_UART,
	DEV_HWID,
	DEV_LCD,
	DEV_BACKLIGHT,
	DEV_TOUCH,
	DEV_KEYPAD,
	DEV_KEYPAD_BACKLIGHT,
	DEV_LED,
	DEV_AUDIO,
	DEV_HEADSET,
	DEV_EXT_CHARGER,
	DEV_EXT_GAUGE,
	DEV_PMIC_CHARGER,
	DEV_PMIC_GAUGE,
	DEV_WIRELESS_CHARGER,
	DEV_WIRELESS_GAUGE,
	DEV_CAM_FRONT,
	DEV_CAM_REAR,
	DEV_CAM_FLASHLIGHT,
	DEV_WIFI,
	DEV_BT,
	DEV_FMR,
	DEV_SD,
	DEV_SENSOR_LIGHT,
	DEV_SENSOR_PROX,
	DEV_SENSOR_GYRO,
	DEV_SENSOR_GRAV,
	DEV_SENSOR_ECOMP,
	DEV_VIBRATOR,
	DEV_DOCKING,
	DEV_HDMI,
	DEV_HDMI_MHL,
	DEV_NFC,
	DEV_MAX,
};

enum suspend_dev_suspend_resume {
	DEV_SUSPEND_RESUME_NONE = 0,
	DEV_SUSPEND,
	DEV_RESUME,
};

enum suspend_dev_state {
	DEV_STATE_NONE = 0,
	DEV_STATE_SUCCESS,
	DEV_STATE_NOT_READY,
	DEV_STATE_IOCTL_ERROR,
	DEV_STATE_PWRCTL_ERROR,
	DEV_STATE_INTFCTL_ERROR,
	DEV_STATE_ICCTL_ERROR,
};

enum suspend_dev_mode {
	DEV_MODE_NONE = 0,
	DEV_MODE_ACTIVE,
	DEV_MODE_LOWPWR,
};

enum suspend_dev_cmd {
	DEV_CMD_NONE = 0,
	DEV_CMD_AUTO,
	DEV_CMD_MANUAL,
};

enum suspend_dev_func {
	DEV_FUNC_NONE = 0,
	DEV_FUNC_EARLY_SUSPEND = (1 << 0),
	DEV_FUNC_LATE_RESUME = (1 << 1),
	DEV_FUNC_SUSPEND = (1 << 2),
	DEV_FUNC_RESUME = (1 << 3),
};

struct suspend_gpio_state {
	const char *name;
	unsigned state;
};

struct suspend_gpio_data {
	int ngpios;
	const struct suspend_gpio_state *gpios;
};

struct suspend_active_ma {
	int nactma;
	int *actma;
};

struct suspend_dev_ops {
	struct list_head node;
	enum suspend_dev id;
	char *name;
	struct suspend_gpio_data *gpio;
	enum suspend_dev_state suspend_state;
	enum suspend_dev_state resume_state;
	enum suspend_dev_mode mode;
	int mode_paras;
	enum suspend_dev_func func;
	enum suspend_dev_cmd cmd;
	struct suspend_active_ma *active_ma;
	int lowpwr_ma;
	int is_wakelock;
	int is_wakeup_source;
	int (*get_suspend_state)(void);
	int (*get_resume_state)(void);
};
extern void suspend_device_register(struct suspend_dev_ops *ops);
extern void suspend_device_unregister(struct suspend_dev_ops *ops);
extern void suspend_device_get_gpio_data(unsigned int *state, char *name[][24]);
extern void suspend_device_notify_state(enum suspend_dev id, char *name, 
	enum suspend_dev_suspend_resume suspend_resume, 
	enum suspend_dev_state state);

#endif
