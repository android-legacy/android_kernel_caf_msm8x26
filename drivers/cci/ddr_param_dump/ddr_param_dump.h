/*===========================================================================
File: kernel\drivers\cci\ddr_param_dump\ddr_param_dump.h
Description: header file for ddr_param_dump.c

Revision History:
when		who				what, where, why
--------	---				----------------------------------------------------
12/03/13	Grace Chang		Initial version
===========================================================================*/

#ifndef CCI_DDR_PARAM_DUMP
#define CCI_DDR_PARAM_DUMP

typedef struct
{
	char* ddr_param_name;			// Name of the DDR paramter
	char* ddr_param_reg_name;		// Name of the DDR paramter's register
	unsigned int ddr_param_reg_addr;	// DDR paramter's physical address
	unsigned int ddr_param_mask;		// DDR paramter's bit mask
	unsigned int ddr_param_offset;		// DDR paramter's offset
	unsigned int ddr_param_val;		// DDR paramter's value
} DdrSection_t;

DdrSection_t ddr_param_section[] = \
{
	{"tRFC",	"BIMC_S_DDR0_DPE_DRAM_TIMING_3",	0xFC3CC150,	0x0FFF0000,	4,	0},
	{"tRAS_Min",	"BIMC_S_DDR0_DPE_DRAM_TIMING_0",	0xFC3CC144,	0x000003FF,	0,	0},
	{"tREFI",	"BIMC_S_DDR0_SHKE_AUTO_REFRESH_CNTL",	0xFC3CD0F0,	0x000003FF,	0,	0}, // Need calculate!!
	{"tXSR",	"BIMC_S_DDR0_DPE_DRAM_TIMING_10",	0xFC3CC16C,	0x0FFF0000,	4,	0},
	{"tXP_11",	"BIMC_S_DDR0_DPE_DRAM_TIMING_11",	0xFC3CC170,	0x00FF0000,	4,	0},
	{"tXP_12",	"BIMC_S_DDR0_DPE_DRAM_TIMING_12",	0xFC3CC174,	0x00FF0000,	4,	0},
	{"tWTR",	"BIMC_S_DDR0_DPE_DRAM_TIMING_2",	0xFC3CC14C,	0x007F0000,	4,	0},
	{"tRP_AB",	"BIMC_S_DDR0_DPE_DRAM_TIMING_5",	0xFC3CC158,	0x01FF0000,	4,	0},
	{"tRRD",	"BIMC_S_DDR0_DPE_DRAM_TIMING_2",	0xFC3CC14C,	0x000000FF,	0,	0},
	{"tWR",		"BIMC_S_DDR0_DPE_DRAM_TIMING_1",	0xFC3CC148,	0x00FF0000,	4,	0},
	{"tCKE" ,	"BIMC_S_DDR0_DPE_DRAM_TIMING_6",	0xFC3CC15C,	0x000000FF,	0,	0},
	{"tRCD",	"BIMC_S_DDR0_DPE_DRAM_TIMING_1",	0xFC3CC148,	0x000001FF,	0,	0},
	{"tRP_PB",	"BIMC_S_DDR0_DPE_DRAM_TIMING_5",	0xFC3CC158,	0x000001FF,	0,	0},
	{"tFAW",	"BIMC_S_DDR0_DPE_DRAM_TIMING_6",	0xFC3CC15C,	0x03FF0000,	4,	0},
	{"tRTP",	"BIMC_S_DDR0_DPE_DRAM_TIMING_4",	0xFC3CC154,	0x0000007F,	0,	0},
	{"tMRR",	"BIMC_S_DDR0_DPE_TIMER_2",		0xFC3CC0EC,	0x00000007,	0,	0},
	{"tZQCL",	"BIMC_S_DDR0_DPE_DRAM_TIMING_17",	0xFC3CC188,	0x00000FFF,	0,	0},
	{"tZQCS",	"BIMC_S_DDR0_DPE_DRAM_TIMING_7",	0xFC3CC160,	0x000007FF,	0,	0},
	{"tMRW",	"BIMC_S_DDR0_DPE_TIMER_2",		0xFC3CC0EC,	0x000000F0,	1,	0},
	{"tCKESR",	"BIMC_S_DDR0_DPE_DRAM_TIMING_9",	0xFC3CC168,	0x000000FF,	0,	0},
};

const int ddr_param_num = sizeof(ddr_param_section) / sizeof(DdrSection_t);

typedef struct {
	DdrSection_t* ddrsection;
	unsigned int ddrsection_num;
} ddrdumpinfo_t;

#endif
