/*
 * Copyright (c) 2011 Yamaha Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <linux/input/yas.h>

#define YAS_KXTJ2_RESOLUTION                                                1024

/* Axes data range  [um/s^2] */
#define YAS_KXTJ2_GRAVITY_EARTH                                          9806550
#define YAS_KXTJ2_ABSMIN_2G                       (-YAS_KXTJ2_GRAVITY_EARTH * 2)
#define YAS_KXTJ2_ABSMAX_2G                        (YAS_KXTJ2_GRAVITY_EARTH * 2)

#define YAS_KXTJ2_POWERUP_TIME                                                10
#define YAS_KXTJ2_RESET_TIME                                                   1

/* Default parameters */
#define YAS_KXTJ2_DEFAULT_DELAY                                              100
#define YAS_KXTJ2_DEFAULT_POSITION                                             0

#define YAS_KXTJ2_MAX_DELAY                                                  200
#define YAS_KXTJ2_MIN_DELAY                                                   10

/* Registers */
#define YAS_KXTJ2_ACC_REG                                                   0x06

#define YAS_KXTJ2_WHO_AM_I_REG                                              0x0f
#define YAS_KXTJ2_WHO_AM_I_VAL                                              0x09

#define YAS_KXTJ2_CTRL_REG1                                                 0x1b
#define YAS_KXTJ2_CTRL_REG1_PC1                                             0x80
#define YAS_KXTJ2_CTRL_REG1_RES                                             0x40

#define YAS_KXTJ2_POW_REG                                                   0x1b
#define YAS_KXTJ2_POW_MASK                                                  0x80
#define YAS_KXTJ2_POW_SHIFT                                                    7
#define YAS_KXTJ2_POW_ON                                                       1
#define YAS_KXTJ2_POW_OFF                                                      0

#define YAS_KXTJ2_CTRL_REG2                                                 0x1d
#define YAS_KXTJ2_CTRL_REG2_SRST                                            0x80

#define YAS_KXTJ2_DATA_CTRL_REG                                             0x21
#define YAS_KXTJ2_DATA_CTRL_1600HZ                                          0x07
#define YAS_KXTJ2_DATA_CTRL_800HZ                                           0x06
#define YAS_KXTJ2_DATA_CTRL_400HZ                                           0x05
#define YAS_KXTJ2_DATA_CTRL_200HZ                                           0x04
#define YAS_KXTJ2_DATA_CTRL_100HZ                                           0x03
#define YAS_KXTJ2_DATA_CTRL_50HZ                                            0x02
#define YAS_KXTJ2_DATA_CTRL_25HZ                                            0x01
#define YAS_KXTJ2_DATA_CTRL_12HZ                                            0x00
#define YAS_KXTJ2_DATA_CTRL_6HZ                                             0x0b

#define YAS_KXTJ2_STARTUP_TIME_1600HZ                                          2
#define YAS_KXTJ2_STARTUP_TIME_800HZ                                           3
#define YAS_KXTJ2_STARTUP_TIME_400HZ                                           4
#define YAS_KXTJ2_STARTUP_TIME_200HZ                                           6
#define YAS_KXTJ2_STARTUP_TIME_100HZ                                          11
#define YAS_KXTJ2_STARTUP_TIME_50HZ                                           21
#define YAS_KXTJ2_STARTUP_TIME_25HZ                                           41
#define YAS_KXTJ2_STARTUP_TIME_12HZ                                           80
#define YAS_KXTJ2_STARTUP_TIME_6HZ                                           151

/* -------------------------------------------------------------------------- */
/*  Structure definition                                                      */
/* -------------------------------------------------------------------------- */
/* Output data rate */
struct yas_kxtj2_odr {
	unsigned long delay;          /* min delay (msec) in the range of ODR */
	unsigned char odr;            /* bandwidth register value             */
};

/* Axes data */
struct yas_kxtj2_acceleration {
	int x;
	int y;
	int z;
	int x_raw;
	int y_raw;
	int z_raw;
};

/* Driver private data */
struct yas_kxtj2_data {
	uint8_t chip_id;
	int initialize;
	int i2c_open;
	int enable;
	int delay;
	int odr;
	int position;
	int threshold;
	int filter_enable;
	struct yas_vector offset;
	struct yas_kxtj2_acceleration last;
};

/* Sleep duration */
struct yas_kxtj2_sd {
	uint8_t bw;
	uint8_t sd;
};

/* -------------------------------------------------------------------------- */
/*  Data                                                                      */
/* -------------------------------------------------------------------------- */
/* Control block */
static struct yas_acc_driver  cb;
static struct yas_acc_driver *pcb;
static struct yas_kxtj2_data  acc_data;

/* Output data rate */
static const struct yas_kxtj2_odr yas_kxtj2_odr_tbl[] = {
	{1,   YAS_KXTJ2_DATA_CTRL_1600HZ},
	{2,   YAS_KXTJ2_DATA_CTRL_800HZ},
	{4,   YAS_KXTJ2_DATA_CTRL_400HZ},
	{8,   YAS_KXTJ2_DATA_CTRL_200HZ},
	{20,  YAS_KXTJ2_DATA_CTRL_100HZ},
	{30,  YAS_KXTJ2_DATA_CTRL_50HZ},
	{50,  YAS_KXTJ2_DATA_CTRL_25HZ},
	{100, YAS_KXTJ2_DATA_CTRL_12HZ},
	{180, YAS_KXTJ2_DATA_CTRL_6HZ},
};

/* Transformation matrix for chip mounting position 1 */
static const int yas_kxtj2_position_map[][3][3] = {
	{ { 0, -1,  0}, { 1,  0,  0}, { 0,  0,  1} },   /* top/upper-left     */
	{ { 1,  0,  0}, { 0,  1,  0}, { 0,  0,  1} },   /* top/upper-right    */
	{ { 0,  1,  0}, {-1,  0,  0}, { 0,  0,  1} },   /* top/lower-right    */
	{ {-1,  0,  0}, { 0, -1,  0}, { 0,  0,  1} },   /* top/lower-left     */
	{ { 0,  1,  0}, { 1,  0,  0}, { 0,  0, -1} },   /* bottom/upper-right */
	{ {-1,  0,  0}, { 0,  1,  0}, { 0,  0, -1} },   /* bottom/upper-left  */
	{ { 0, -1,  0}, {-1,  0,  0}, { 0,  0, -1} },   /* bottom/lower-left  */
	{ { 1,  0,  0}, { 0, -1,  0}, { 0,  0, -1} },   /* bottom/lower-right */
};

/* -------------------------------------------------------------------------- */
/*  Prototype declaration                                                     */
/* -------------------------------------------------------------------------- */
static void yas_kxtj2_init_data(void);
static int yas_kxtj2_ischg_enable(int);
static int yas_kxtj2_read_reg(unsigned char, unsigned char *, int);
static int yas_kxtj2_write_reg(unsigned char, unsigned char *, int);
static unsigned char yas_kxtj2_read_reg_byte(unsigned char);
static int yas_kxtj2_write_reg_byte(unsigned char, unsigned char);
static int yas_kxtj2_lock(void);
static int yas_kxtj2_unlock(void);
static int yas_kxtj2_i2c_open(void);
static int yas_kxtj2_i2c_close(void);
static int yas_kxtj2_msleep(int);
static int yas_kxtj2_reset(void);
static int yas_kxtj2_power_up(void);
static int yas_kxtj2_power_down(void);
static int yas_kxtj2_startup_time(int);
static int yas_kxtj2_data_filter(int [], int [],
				 struct yas_kxtj2_acceleration *);
static int yas_kxtj2_init(void);
static int yas_kxtj2_term(void);
static int yas_kxtj2_get_delay(void);
static int yas_kxtj2_set_delay(int);
static int yas_kxtj2_get_enable(void);
static int yas_kxtj2_set_enable(int);
static int yas_kxtj2_get_position(void);
static int yas_kxtj2_set_position(int);
static int yas_kxtj2_get_offset(struct yas_vector *);
static int yas_kxtj2_set_offset(struct yas_vector *);
static int yas_kxtj2_get_filter(struct yas_acc_filter *);
static int yas_kxtj2_set_filter(struct yas_acc_filter *);
static int yas_kxtj2_get_filter_enable(void);
static int yas_kxtj2_set_filter_enable(int);
static int yas_kxtj2_measure(int *, int *);
#if DEBUG
static int yas_kxtj2_get_register(uint8_t, uint8_t *);
#endif /* DEBUG */

/* -------------------------------------------------------------------------- */
/*  Local functions                                                           */
/* -------------------------------------------------------------------------- */

static void yas_kxtj2_init_data(void)
{
	acc_data.chip_id = 0;
	acc_data.initialize = 0;
	acc_data.enable = 0;
	acc_data.delay = YAS_KXTJ2_DEFAULT_DELAY;
	acc_data.offset.v[0] = 0;
	acc_data.offset.v[1] = 0;
	acc_data.offset.v[2] = 0;
	acc_data.position = YAS_KXTJ2_DEFAULT_POSITION;
	acc_data.threshold = YAS_ACC_DEFAULT_FILTER_THRESH;
	acc_data.filter_enable = 0;
	acc_data.last.x = 0;
	acc_data.last.y = 0;
	acc_data.last.z = 0;
	acc_data.last.x_raw = 0;
	acc_data.last.y_raw = 0;
	acc_data.last.z_raw = 0;
}

static int yas_kxtj2_ischg_enable(int enable)
{
	if (acc_data.enable == enable)
		return 0;

	return 1;
}

/* register access functions */
static int yas_kxtj2_read_reg(unsigned char adr, unsigned char *buf, int len)
{
	struct yas_acc_driver_callback *cbk = &pcb->callback;

	if (acc_data.i2c_open)
		return cbk->device_read(adr, buf, len);

	return YAS_NO_ERROR;
}

static int yas_kxtj2_write_reg(unsigned char adr, unsigned char *buf, int len)
{
	struct yas_acc_driver_callback *cbk = &pcb->callback;

	if (acc_data.i2c_open)
		return cbk->device_write(adr, buf, len);

	return YAS_NO_ERROR;
}

static unsigned char yas_kxtj2_read_reg_byte(unsigned char adr)
{
	unsigned char buf = 0xff;
	int err;

	err = yas_kxtj2_read_reg(adr, &buf, 1);
	if (err == 0)
		return buf;

	return 0;
}

static int yas_kxtj2_write_reg_byte(unsigned char adr, unsigned char val)
{
	return yas_kxtj2_write_reg(adr, &val, 1);
}

#define yas_kxtj2_read_bits(r) \
	((yas_kxtj2_read_reg_byte(r##_REG) & r##_MASK) >> r##_SHIFT)

#define yas_kxtj2_update_bits(r, v) \
	yas_kxtj2_write_reg_byte(r##_REG, \
			     ((yas_kxtj2_read_reg_byte(r##_REG) & ~r##_MASK) | \
			      ((v) << r##_SHIFT)))

static int yas_kxtj2_lock(void)
{
	struct yas_acc_driver_callback *cbk = &pcb->callback;
	int err;

	if (cbk->lock != NULL && cbk->unlock != NULL)
		err = cbk->lock();
	else
		err = YAS_NO_ERROR;

	return err;
}

static int yas_kxtj2_unlock(void)
{
	struct yas_acc_driver_callback *cbk = &pcb->callback;
	int err;

	if (cbk->lock != NULL && cbk->unlock != NULL)
		err = cbk->unlock();
	else
		err = YAS_NO_ERROR;

	return err;
}

static int yas_kxtj2_i2c_open(void)
{
	struct yas_acc_driver_callback *cbk = &pcb->callback;
	int err;

	if (acc_data.i2c_open == 0) {
		err = cbk->device_open();
		if (err != YAS_NO_ERROR)
			return YAS_ERROR_DEVICE_COMMUNICATION;
		acc_data.i2c_open = 1;
	}

	return YAS_NO_ERROR;
}

static int yas_kxtj2_i2c_close(void)
{
	struct yas_acc_driver_callback *cbk = &pcb->callback;
	int err;

	if (acc_data.i2c_open != 0) {
		err = cbk->device_close();
		if (err != YAS_NO_ERROR)
			return YAS_ERROR_DEVICE_COMMUNICATION;
		acc_data.i2c_open = 0;
	}
	return YAS_NO_ERROR;
}

static int yas_kxtj2_msleep(int msec)
{
	struct yas_acc_driver_callback *cbk = &pcb->callback;

	if (msec <= 0)
		return YAS_ERROR_ARG;

	cbk->msleep(msec);

	return YAS_NO_ERROR;
}

static int yas_kxtj2_reset(void)
{
	unsigned char reg;

	yas_kxtj2_write_reg_byte(YAS_KXTJ2_CTRL_REG2, YAS_KXTJ2_CTRL_REG2_SRST);
	yas_kxtj2_msleep(YAS_KXTJ2_RESET_TIME);

	while (1) {
		reg = yas_kxtj2_read_reg_byte(YAS_KXTJ2_CTRL_REG2);
		if ((reg & YAS_KXTJ2_CTRL_REG2_SRST) == 0)
			break;
	}

	return YAS_NO_ERROR;
}

static int yas_kxtj2_power_up(void)
{
	yas_kxtj2_update_bits(YAS_KXTJ2_POW, YAS_KXTJ2_POW_ON);
	yas_kxtj2_msleep(yas_kxtj2_startup_time(acc_data.odr));

	return YAS_NO_ERROR;
}

static int yas_kxtj2_power_down(void)
{
	yas_kxtj2_update_bits(YAS_KXTJ2_POW, YAS_KXTJ2_POW_OFF);

	return YAS_NO_ERROR;
}

static int yas_kxtj2_startup_time(int odr)
{
	switch (odr) {
	case YAS_KXTJ2_DATA_CTRL_1600HZ:
		return YAS_KXTJ2_STARTUP_TIME_1600HZ;
	case YAS_KXTJ2_DATA_CTRL_800HZ:
		return YAS_KXTJ2_STARTUP_TIME_800HZ;
	case YAS_KXTJ2_DATA_CTRL_400HZ:
		return YAS_KXTJ2_STARTUP_TIME_400HZ;
	case YAS_KXTJ2_DATA_CTRL_200HZ:
		return YAS_KXTJ2_STARTUP_TIME_200HZ;
	case YAS_KXTJ2_DATA_CTRL_100HZ:
		return YAS_KXTJ2_STARTUP_TIME_100HZ;
	case YAS_KXTJ2_DATA_CTRL_50HZ:
		return YAS_KXTJ2_STARTUP_TIME_50HZ;
	case YAS_KXTJ2_DATA_CTRL_25HZ:
		return YAS_KXTJ2_STARTUP_TIME_25HZ;
	case YAS_KXTJ2_DATA_CTRL_12HZ:
		return YAS_KXTJ2_STARTUP_TIME_12HZ;
	case YAS_KXTJ2_DATA_CTRL_6HZ:
		return YAS_KXTJ2_STARTUP_TIME_6HZ;
	default:
		break;
	}

	return YAS_KXTJ2_STARTUP_TIME_6HZ;
}

static int yas_kxtj2_data_filter(int data[], int raw[],
				 struct yas_kxtj2_acceleration *accel)
{
	int filter_enable = acc_data.filter_enable;
	int threshold = acc_data.threshold;

	if (filter_enable) {
		if ((ABS(acc_data.last.x - data[0]) > threshold) ||
		    (ABS(acc_data.last.y - data[1]) > threshold) ||
		    (ABS(acc_data.last.z - data[2]) > threshold)) {
			accel->x = data[0];
			accel->y = data[1];
			accel->z = data[2];
			accel->x_raw = raw[0];
			accel->y_raw = raw[1];
			accel->z_raw = raw[2];
		} else {
			*accel = acc_data.last;
		}
	} else {
		accel->x = data[0];
		accel->y = data[1];
		accel->z = data[2];
		accel->x_raw = raw[0];
		accel->y_raw = raw[1];
		accel->z_raw = raw[2];
	}

	return YAS_NO_ERROR;
}

static int yas_kxtj2_init(void)
{
	struct yas_acc_filter filter;
	unsigned char id = 0;  //[CCI]Ginger modified for factory test
	extern unsigned char g_accel_product_id;
	int err;

	/* Check intialize */
	if (acc_data.initialize == 1)
		return YAS_ERROR_NOT_INITIALIZED;

	/* Wait powerup time */
	yas_kxtj2_msleep(YAS_KXTJ2_POWERUP_TIME);

	/* Init data */
	yas_kxtj2_init_data();

	/* Open i2c */
	err = yas_kxtj2_i2c_open();
	if (err != YAS_NO_ERROR)
		return err;

	/* Check id */
	id = yas_kxtj2_read_reg_byte(YAS_KXTJ2_WHO_AM_I_REG);
	//S [CCI]Ginger modified for factory test
	YLOGD(("who_am_i_val:%02x\n", id));
	g_accel_product_id = id;
	//E [CCI]Ginger modified for factory test
	if (id != YAS_KXTJ2_WHO_AM_I_VAL) {
		yas_kxtj2_i2c_close();
		return YAS_ERROR_CHIP_ID;
	}

	/* Reset chip */
	yas_kxtj2_reset();

	/* Set resolusion and axes range */
	yas_kxtj2_write_reg_byte(YAS_KXTJ2_CTRL_REG1, YAS_KXTJ2_CTRL_REG1_RES);

	acc_data.chip_id = id;
	acc_data.initialize = 1;

	yas_kxtj2_set_delay(YAS_KXTJ2_DEFAULT_DELAY);
	yas_kxtj2_set_position(YAS_KXTJ2_DEFAULT_POSITION);
	filter.threshold = YAS_ACC_DEFAULT_FILTER_THRESH;
	yas_kxtj2_set_filter(&filter);

	return YAS_NO_ERROR;
}

static int yas_kxtj2_term(void)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_set_enable(0);

	/* Close I2C */
	yas_kxtj2_i2c_close();

	acc_data.initialize = 0;

	return YAS_NO_ERROR;
}

static int yas_kxtj2_get_delay(void)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	return acc_data.delay;
}

static int yas_kxtj2_set_delay(int delay)
{
	int i;

	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	/* Determine optimum odr */
	for (i = 1; i < (int)(sizeof(yas_kxtj2_odr_tbl) /
			      sizeof(struct yas_kxtj2_odr)) &&
		     delay >= (int)yas_kxtj2_odr_tbl[i].delay; i++)
		;

	acc_data.delay = delay;
	acc_data.odr = yas_kxtj2_odr_tbl[i-1].odr;

	if (yas_kxtj2_get_enable()) {
		yas_kxtj2_power_down();
		yas_kxtj2_write_reg_byte(YAS_KXTJ2_DATA_CTRL_REG, acc_data.odr);
		yas_kxtj2_power_up();
	}

	return YAS_NO_ERROR;
}

static int yas_kxtj2_get_enable(void)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	return acc_data.enable;
}

static int yas_kxtj2_set_enable(int enable)
{
	int err;

	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	if (yas_kxtj2_ischg_enable(enable)) {
		if (enable) {
			/* Open i2c */
			err = yas_kxtj2_i2c_open();
			if (err != YAS_NO_ERROR)
				return err;
			/* Wait powerup time */
			yas_kxtj2_msleep(YAS_KXTJ2_POWERUP_TIME);
			/* Reset chip */
			yas_kxtj2_reset();
			/* Set resolusion and axes range */
			yas_kxtj2_write_reg_byte(YAS_KXTJ2_CTRL_REG1,
						 YAS_KXTJ2_CTRL_REG1_RES);
			acc_data.enable = enable;
			yas_kxtj2_set_delay(acc_data.delay);
		} else {
			yas_kxtj2_power_down();
			err = yas_kxtj2_i2c_close();
			if (err != YAS_NO_ERROR)
				return err;
			acc_data.enable = enable;
		}
	}

	return YAS_NO_ERROR;
}

static int yas_kxtj2_get_position(void)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	return acc_data.position;
}

static int yas_kxtj2_set_position(int position)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	acc_data.position = position;

	return YAS_NO_ERROR;
}

static int yas_kxtj2_get_offset(struct yas_vector *offset)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	*offset = acc_data.offset;

	return YAS_NO_ERROR;
}

static int yas_kxtj2_set_offset(struct yas_vector *offset)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	acc_data.offset = *offset;

	return YAS_NO_ERROR;
}

static int yas_kxtj2_get_filter(struct yas_acc_filter *filter)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	filter->threshold = acc_data.threshold;

	return YAS_NO_ERROR;
}

static int yas_kxtj2_set_filter(struct yas_acc_filter *filter)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	acc_data.threshold = filter->threshold;

	return YAS_NO_ERROR;
}

static int yas_kxtj2_get_filter_enable(void)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	return acc_data.filter_enable;
}

static int yas_kxtj2_set_filter_enable(int enable)
{
	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	acc_data.filter_enable = enable;

	return YAS_NO_ERROR;
}

/*Kionix Auto-Cali Start*/
#define KIONIX_AUTO_CAL
#ifdef KIONIX_AUTO_CAL
#define Sensitivity_def	1024	//	
#define Detection_range 205 	// Follow KXTJ2 SPEC Offset Range define
#define Stable_range 50     	// Stable iteration
#define BUF_RANGE_Limit 30 	
int BUF_RANGE = BUF_RANGE_Limit;			
int temp_zbuf[50]={0};
int temp_zsum = 0; // 1024 * BUF_RANGE ;
int Z_AVG[2] = {Sensitivity_def,Sensitivity_def} ;

int Wave_Max,Wave_Min;
#endif
/*Kionix Auto-Cali End*/

static int yas_kxtj2_measure(int *out_data, int *out_raw)
{
	struct yas_kxtj2_acceleration accel;
	unsigned char buf[6];
	int32_t raw[3], data[3];
	int pos = acc_data.position;
	int i, j;
/*Kionix Auto-Cali Start*/
#ifdef KIONIX_AUTO_CAL	
	int k=0;
#endif
/*Kionix Auto-Cali End*/

	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	/* Check device */
	if (acc_data.i2c_open == 0) {
		out_data[0] = acc_data.last.x;
		out_data[1] = acc_data.last.y;
		out_data[2] = acc_data.last.z;
		out_raw[0] = acc_data.last.x_raw;
		out_raw[1] = acc_data.last.y_raw;
		out_raw[2] = acc_data.last.z_raw;
		return YAS_NO_ERROR;
	}

	/* Read acceleration data */
	if (yas_kxtj2_read_reg(YAS_KXTJ2_ACC_REG, buf, 6) != 0) {
		for (i = 0; i < 3; i++)
			raw[i] = 0;
	} else {
		for (i = 0; i < 3; i++)
			raw[i] = ((int16_t)((buf[i*2+1] << 8)) | buf[i*2]) >> 4;
			
/*Kionix Auto-Cali Start*/
#ifdef KIONIX_AUTO_CAL


		if(			(abs(raw[0]) < Detection_range)  
				&&	(abs(raw[1]) < Detection_range)	
				&&	(abs((abs(raw[2])- Sensitivity_def))  < ((Detection_range)+ 154)))		// 154 = 1024*15% 
		  {
			//printk("KXTJ2 Calibration Raw Data,%d,%d,%d\n",raw[0],raw[1],raw[2]);
			
			temp_zsum = 0;
			Wave_Max =-4095;
			Wave_Min = 4095;
			
			BUF_RANGE = 1000 / acc_data.delay; 
			if ( BUF_RANGE > BUF_RANGE_Limit ) BUF_RANGE = BUF_RANGE_Limit; 
			//printk("KXTJ2 Buffer Range =%d\n",BUF_RANGE);
			
			for (k=0; k < BUF_RANGE-1; k++) {
				temp_zbuf[k] = temp_zbuf[k+1];
				if (temp_zbuf[k] == 0) temp_zbuf[k] = Sensitivity_def ;
				temp_zsum += temp_zbuf[k];
				if (temp_zbuf[k] > Wave_Max) Wave_Max = temp_zbuf[k];
				if (temp_zbuf[k] < Wave_Min) Wave_Min = temp_zbuf[k];
			}

			temp_zbuf[k] = raw[2]; // k=BUF_RANGE-1, update Z raw to bubber
			temp_zsum += temp_zbuf[k];
			if (temp_zbuf[k] > Wave_Max) Wave_Max = temp_zbuf[k];
			if (temp_zbuf[k] < Wave_Min) Wave_Min = temp_zbuf[k];	   
			if (Wave_Max-Wave_Min < Stable_range ){
				
			if ( temp_zsum > 0)
					Z_AVG[0] = temp_zsum / BUF_RANGE;
				else 
					Z_AVG[1] = temp_zsum / BUF_RANGE;	
				//printk("KXTJ2 start Z compensation Z_AVG Max Min,%d,%d,%d\n",(temp_zsum / BUF_RANGE),Wave_Max,Wave_Min);
			}
		}
		else if(abs((abs(raw[2])- Sensitivity_def))  > ((Detection_range)+ 154))
		{
			printk("KXTJ2 out of SPEC Raw Data,%d,%d,%d\n",raw[0],raw[1],raw[2]);
		}
		//else
		//{
		//    printk("KXTJ2 not in horizontal X=%d, Y=%d\n", raw[0], raw[1]);
		//}

		if ( raw[2] >=0) 
					raw[2] = raw[2] * 1024 / abs(Z_AVG[0]); // Gain Compensation
		else
					raw[2] = raw[2] * 1024 / abs(Z_AVG[1]); // Gain Compensation
		//printk("KXTJ2 Raw Data after,%d,%d,%d\n",raw[0],raw[1],raw[2]);
				
/*Kionix Auto-Cali End*/
#endif
	}

	/* for X, Y, Z axis */
	for (i = 0; i < 3; i++) {
		/* coordinate transformation */
		data[i] = 0;
		for (j = 0; j < 3; j++)
			data[i] += raw[j] * yas_kxtj2_position_map[pos][i][j];
		/* normalization */
		data[i] = data[i]*(YAS_KXTJ2_GRAVITY_EARTH / YAS_KXTJ2_RESOLUTION);
	}

	yas_kxtj2_data_filter(data, raw, &accel);
	out_data[0] = accel.x - acc_data.offset.v[0];
	out_data[1] = accel.y - acc_data.offset.v[1];
	out_data[2] = accel.z - acc_data.offset.v[2];
	
	out_raw[0] = accel.x_raw;
	out_raw[1] = accel.y_raw;
	out_raw[2] = accel.z_raw;

	acc_data.last = accel;

	return YAS_NO_ERROR;
}

#if DEBUG
static int yas_kxtj2_get_register(uint8_t adr, uint8_t *val)
{
	int open;

	open = acc_data.i2c_open;

	if (open == 0)
		yas_kxtj2_i2c_open();

	*val = yas_kxtj2_read_reg_byte(adr);

	if (open == 0)
		yas_kxtj2_i2c_close();

	return YAS_NO_ERROR;
}

#endif /* DEBUG */

/* -------------------------------------------------------------------------- */
static int yas_init(void)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_lock();
	err = yas_kxtj2_init();
	yas_kxtj2_unlock();

	return err;
}

static int yas_term(void)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_lock();
	err = yas_kxtj2_term();
	yas_kxtj2_unlock();

	return err;
}

static int yas_get_delay(void)
{
	int ret;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_lock();
	ret = yas_kxtj2_get_delay();
	yas_kxtj2_unlock();

	return ret;
}

static int yas_set_delay(int delay)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (delay < 0 || delay > YAS_KXTJ2_MAX_DELAY)
		return YAS_ERROR_ARG;
	else if (delay < YAS_KXTJ2_MIN_DELAY)
		delay = YAS_KXTJ2_MIN_DELAY;

	yas_kxtj2_lock();
	err = yas_kxtj2_set_delay(delay);
	yas_kxtj2_unlock();

	return err;
}

static int yas_get_enable(void)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_lock();
	err = yas_kxtj2_get_enable();
	yas_kxtj2_unlock();

	return err;
}

static int yas_set_enable(int enable)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (enable != 0)
		enable = 1;

	yas_kxtj2_lock();
	err = yas_kxtj2_set_enable(enable);
	yas_kxtj2_unlock();

	return err;
}

static int yas_get_position(void)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_lock();
	err = yas_kxtj2_get_position();
	yas_kxtj2_unlock();

	return err;
}

static int yas_set_position(int position)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (!((position >= 0) && (position <= 7)))
		return YAS_ERROR_ARG;

	yas_kxtj2_lock();
	err = yas_kxtj2_set_position(position);
	yas_kxtj2_unlock();

	return err;
}

static int yas_get_offset(struct yas_vector *offset)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (offset == NULL)
		return YAS_ERROR_ARG;

	yas_kxtj2_lock();
	err = yas_kxtj2_get_offset(offset);
	yas_kxtj2_unlock();

	return err;
}

static int yas_set_offset(struct yas_vector *offset)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (offset == NULL)
		return YAS_ERROR_ARG;

	yas_kxtj2_lock();
	err = yas_kxtj2_set_offset(offset);
	yas_kxtj2_unlock();

	return err;
}

static int yas_get_filter(struct yas_acc_filter *filter)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (filter == NULL)
		return YAS_ERROR_ARG;

	yas_kxtj2_lock();
	err = yas_kxtj2_get_filter(filter);
	yas_kxtj2_unlock();

	return err;
}

static int yas_set_filter(struct yas_acc_filter *filter)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (filter == NULL ||
	    filter->threshold < 0 ||
	    filter->threshold > YAS_KXTJ2_ABSMAX_2G)
		return YAS_ERROR_ARG;

	yas_kxtj2_lock();
	err = yas_kxtj2_set_filter(filter);
	yas_kxtj2_unlock();

	return err;
}

static int yas_get_filter_enable(void)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_lock();
	err = yas_kxtj2_get_filter_enable();
	yas_kxtj2_unlock();

	return err;
}

static int yas_set_filter_enable(int enable)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (enable != 0)
		enable = 1;

	yas_kxtj2_lock();
	err = yas_kxtj2_set_filter_enable(enable);
	yas_kxtj2_unlock();

	return err;
}

static int yas_measure(struct yas_acc_data *data)
{
	int err;

	/* Check intialize */
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	if (data == NULL)
		return YAS_ERROR_ARG;

	yas_kxtj2_lock();
	err = yas_kxtj2_measure(data->xyz.v, data->raw.v);
	yas_kxtj2_unlock();

	return err;
}

#if DEBUG
static int yas_get_register(uint8_t adr, uint8_t *val)
{
	if (pcb == NULL)
		return YAS_ERROR_NOT_INITIALIZED;

	/* Check initialize */
	if (acc_data.initialize == 0)
		return YAS_ERROR_NOT_INITIALIZED;

	yas_kxtj2_lock();
	yas_kxtj2_get_register(adr, val);
	yas_kxtj2_unlock();

	return YAS_NO_ERROR;
}

#endif /* DEBUG */

/* -------------------------------------------------------------------------- */
/*  Global function                                                           */
/* -------------------------------------------------------------------------- */
int yas_acc_driver_init(struct yas_acc_driver *f)
{
	struct yas_acc_driver_callback *cbk;

	/* Check parameter */
	if (f == NULL)
		return YAS_ERROR_ARG;

	cbk = &f->callback;
	if (cbk->device_open == NULL ||
	    cbk->device_close == NULL ||
	    cbk->device_write == NULL ||
	    cbk->device_read == NULL ||
	    cbk->msleep == NULL)
		return YAS_ERROR_ARG;

	/* Clear intialize */
	yas_kxtj2_term();

	/* Set callback interface */
	cb.callback = *cbk;

	/* Set driver interface */
	f->init = yas_init;
	f->term = yas_term;
	f->get_delay = yas_get_delay;
	f->set_delay = yas_set_delay;
	f->get_offset = yas_get_offset;
	f->set_offset = yas_set_offset;
	f->get_enable = yas_get_enable;
	f->set_enable = yas_set_enable;
	f->get_filter = yas_get_filter;
	f->set_filter = yas_set_filter;
	f->get_filter_enable = yas_get_filter_enable;
	f->set_filter_enable = yas_set_filter_enable;
	f->get_position = yas_get_position;
	f->set_position = yas_set_position;
	f->measure = yas_measure;
#if DEBUG
	f->get_register = yas_get_register;
#endif /* DEBUG */
	pcb = &cb;

	return YAS_NO_ERROR;
}
