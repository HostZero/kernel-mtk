/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


#ifndef __STEP_C_H__
#define __STEP_C_H__


#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/atomic.h>
#include <linux/ioctl.h>

#include <batch.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>



#define STEP_C_TAG					"<STEP_COUNTER> "
#define STEP_C_FUN(f)				pr_err(STEP_C_TAG"%s\n", __func__)
#define STEP_C_ERR(fmt, args...)	pr_err(STEP_C_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define STEP_C_LOG(fmt, args...)	pr_err(STEP_C_TAG fmt, ##args)
#define STEP_C_VER(fmt, args...)	pr_err(STEP_C_TAG"%s: "fmt, __func__, ##args) /* ((void)0) */

#define	OP_STEP_C_DELAY		0X01
#define	OP_STEP_C_ENABLE		0X02
#define	OP_STEP_C_GET_DATA	0X04

#define STEP_C_INVALID_VALUE -1

#define EVENT_TYPE_STEP_C_VALUE				ABS_X
#define EVENT_TYPE_STEP_C_STATUS			ABS_WHEEL
#define EVENT_TYPE_STEP_DETECTOR_VALUE		REL_Y
#define EVENT_TYPE_SIGNIFICANT_VALUE		REL_Z



#define STEP_C_VALUE_MAX (32767)
#define STEP_C_VALUE_MIN (-32768)
#define STEP_C_STATUS_MIN (0)
#define STEP_C_STATUS_MAX (64)
#define STEP_C_DIV_MAX (32767)
#define STEP_C_DIV_MIN (1)


#define MAX_CHOOSE_STEP_C_NUM 5

struct step_c_control_path {
	int (*open_report_data)(int open);/* open data rerport to HAL */
	int (*enable_nodata)(int en);/* only enable not report event to HAL */
	int (*enable_step_detect)(int en);
	int (*enable_significant)(int en);
	int (*set_delay)(u64 delay);
	bool is_report_input_direct;
	bool is_support_batch;/* version2.used for batch mode support flag */
};

struct step_c_data_path {
	int (*get_data)(u64 *value, int *status);
	int (*get_data_step_d)(u64 *value, int *status);
	int (*get_data_significant)(u64 *value, int *status);
	int vender_div;
};

struct step_c_init_info {
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver *platform_diver_addr;
};

struct step_c_data {
	uint32_t counter;
	int status;
	int data_updata;
};

struct step_c_drv_obj {
	void *self;
	int polling;
	int (*step_c_operate)(void *self, uint32_t command, void *buff_in, int size_in,
		void *buff_out, int size_out, int *actualout);
};

struct step_c_context {
	struct input_dev   *idev;
	struct miscdevice   mdev;
	struct work_struct  report;
	struct mutex step_c_op_mutex;
	atomic_t            delay; /*polling period for reporting input event*/
	atomic_t            wake;  /*user-space request to wake-up, used with stop*/
	struct timer_list   timer;  /* polling timer */
	atomic_t            trace;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_drv;
	atomic_t early_suspend;
#endif

	struct step_c_data       drv_data;
	struct step_c_control_path   step_c_ctl;
	struct step_c_data_path   step_c_data;
	bool			is_active_nodata;
	bool			is_active_data;		/* Active and HAL need data . */
	bool		is_first_data_after_enable;
	bool		is_polling_run;
	bool		is_batch_enable;	/* version2.this is used for judging whether sensor is in batch mode */
	bool        is_step_d_active; /* for step detect sensor support, add by liaoxl.lenovo 5.12.2015 */
	bool        is_sigmot_active; /* for significant motion sensor support, add by liaoxl.lenovo 5.12.2015 */
};

/* for auto detect */
typedef enum {
	TYPE_STEP_NON = 0,
	TYPE_STEP_DETECTOR = 1,
	TYPE_SIGNIFICANT = 2,
	TYPE_STEP_COUNTER = 3,
	TYPE_STEP_SUSPEND = 4
} STEP_NOTIFY_TYPE;

extern int  step_notify(STEP_NOTIFY_TYPE type);

extern int step_c_driver_add(struct step_c_init_info *obj);
extern int step_c_data_report(struct input_dev *dev, uint32_t value, int status);
extern int step_c_register_control_path(struct step_c_control_path *ctl);
extern int step_c_register_data_path(struct step_c_data_path *data);

#endif
