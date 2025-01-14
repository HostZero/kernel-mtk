#ifndef _CAM_CAL_DATA_H
#define _CAM_CAL_DATA_H

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

typedef struct{
	u32 u4Offset;
	u32 u4Length;
	u32 *pu1Params;  //huangsh4 modify for calibration ioctl failed for k52/kungfu project	

	
} stCAM_CAL_INFO_STRUCT, *stPCAM_CAL_INFO_STRUCT;

#ifdef CONFIG_COMPAT

typedef struct{
	u32 u4Offset;
	u32 u4Length;
	compat_uptr_t pu1Params; //huangsh4 modify for calibration ioctl failed for k52/kungfu project
} COMPAT_stCAM_CAL_INFO_STRUCT;
#endif

#endif/*_CAM_CAL_DATA_H*/

