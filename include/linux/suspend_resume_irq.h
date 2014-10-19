/*===========================================================================
File: kernel/include/linux/suspend_resume_irq.h
Description: suspend/resume show irq mechanism

Revision History:
when       	who			what, where, why
--------   	---			---------------------------------------------------------
06/19/12	KevinA_Lin		Initial version
===========================================================================*/

#ifndef _LINUX_SUSPEND_RESUME_IRQ_H
#define _LINUX_SUSPEND_RESUME_IRQ_H

extern void suspned_resume_irq_write(int);
extern void suspned_resume_irq_pmic_write(uint8_t, uint8_t, uint8_t, unsigned long, int);
#endif
