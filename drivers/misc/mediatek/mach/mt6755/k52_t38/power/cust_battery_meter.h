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

#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

/* ============================================================*/
/* define*/
/* ============================================================*/
/*#define SOC_BY_AUXADC*/
#define SOC_BY_HW_FG
/*#define HW_FG_FORCE_USE_SW_OCV*/
/*#define SOC_BY_SW_FG*/

/*#define CONFIG_DIS_CHECK_BATTERY*/
/*#define FIXED_TBAT_25*/

/* ADC resistor  */
#define R_BAT_SENSE	4
#define R_I_SENSE	4
#define R_CHARGER_1	330
#define R_CHARGER_2	39

#define TEMPERATURE_T0	110
#define TEMPERATURE_T1	0
#define TEMPERATURE_T2	25
#define TEMPERATURE_T3	50
#define TEMPERATURE_T	255 /* This should be fixed, never change the value*/

#define FG_METER_RESISTANCE	0

/* Qmax for battery  */
/*lenovo-sw mahj2 update ZCV param Begin*/
#define Q_MAX_POS_50 3430
#define Q_MAX_POS_25 3500
#define Q_MAX_POS_0 2710
#define Q_MAX_NEG_10 1915

#define Q_MAX_POS_50_H_CURRENT 3361
#define Q_MAX_POS_25_H_CURRENT 3430
#define Q_MAX_POS_0_H_CURRENT 2656
#define Q_MAX_NEG_10_H_CURRENT 1877
/*lenovo-sw mahj2 update ZCV param End*/

/* Discharge Percentage */
#define OAM_D5	1		/*  1 : D5,   0: D2*/


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#ifdef CONFIG_MTK_HAFG_20
#define CUST_TRACKING_POINT	0
#else
#define CUST_TRACKING_POINT	1
#endif
#define CUST_R_SENSE         56
#define CUST_HW_CC	0
#define AGING_TUNING_VALUE	103
#define CUST_R_FG_OFFSET	0

#define OCV_BOARD_COMPESATE	0 /*mV */
#define R_FG_BOARD_BASE	1000
#define R_FG_BOARD_SLOPE	1000 /*slope*/
//lenovo-sw mahj2 modify Begin
#define CAR_TUNE_VALUE		111 /*1.00*/
//lenovo-sw mahj2 modify End


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG	10  /*1mA*/
#define MinErrorOffset	1000
#define FG_VBAT_AVERAGE_SIZE	18
#define R_FG_VALUE	10 /* mOhm, base is 20*/

/* fg 2.0 */
#define DIFFERENCE_HWOCV_RTC	30
//lenovo-sw mahj2 modify Begin
#define DIFFERENCE_HWOCV_SWOCV		16

#define DIFFERENCE_SWOCV_RTC		25
//lenovo-sw mahj2 modify End
#define DIFFERENCE_VBAT_RTC 30
#define DIFFERENCE_SWOCV_RTC_POS 15
#define MAX_SWOCV	3

#define DIFFERENCE_VOLTAGE_UPDATE	20
#define AGING1_LOAD_SOC	70
#define AGING1_UPDATE_SOC	30
//lenovo-sw mahj2 modify for 100% and 1% Begin
#define BATTERYPSEUDO100		98
#define BATTERYPSEUDO1 		2
//lenovo-sw mahj2 modify for 100% and 1% End

#define Q_MAX_BY_SYS			/*8. Qmax variant by system drop voltage.*/
#define Q_MAX_SYS_VOLTAGE		3350
#define SHUTDOWN_GAUGE0
#define SHUTDOWN_GAUGE1_XMINS
#define SHUTDOWN_GAUGE1_MINS	60

//lenovo-sw mahj2 modify Begin
#ifndef SHUTDOWN_SYSTEM_VOLTAGE
#define SHUTDOWN_SYSTEM_VOLTAGE	3400
#endif
//lenovo-sw mahj2 modify End
#define CHARGE_TRACKING_TIME	60
#define DISCHARGE_TRACKING_TIME	10

#define RECHARGE_TOLERANCE	10
/* SW Fuel Gauge */
#define MAX_HWOCV	5
#define MAX_VBAT	90
#define DIFFERENCE_HWOCV_VBAT	30

/* fg 1.0 */
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE	5
#define CUST_POWERON_MAX_VBAT_TOLRANCE	90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE	30
#define CUST_POWERON_DELTA_HW_SW_OCV_CAPACITY_TOLRANCE	10


/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP	3600		/*3.6V*/
#define VBAT_LOW_POWER_WAKEUP	3500		/*3.5v*/
//lenovo-sw mahj2 modify Begin
#define NORMAL_WAKEUP_PERIOD	3600	/*60 * 60 = 60 min*/
//lenovo-sw mahj2 modify End
#define LOW_POWER_WAKEUP_PERIOD	300		/*5 * 60 = 5 min*/
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30	/*30 s*/

#define INIT_SOC_BY_SW_SOC

/*3. UI SOC sync to FG SOC immediately*/
/*#define SYNC_UI_SOC_IMM*/

/*6. Q_MAX aging algorithm*/
#define MTK_ENABLE_AGING_ALGORITHM

/*5. Gauge Adjust by OCV 9. MD sleep current check*/
#define MD_SLEEP_CURRENT_CHECK

/*7. Qmax variant by current loading.*/
/*#define Q_MAX_BY_CURRENT*/

#define FG_BAT_INT
#define IS_BATTERY_REMOVE_BY_PMIC

/*Beging lenovo mahj2 add */
#define CHAGER_CURRENT_USE_SWITCHIC_METER
#ifdef CHAGER_CURRENT_USE_SWITCHIC_METER
#define CHARGER_IC_RLIM 240
#define CHARGER_IC_KLIM 435
#define  CHARGER_CURRENT_ADC 14 
#endif
/*End lenovo mahj2 add */
//lenovo-sw mahj2 add for multi battery Begin
#define MTK_MULTI_BAT_PROFILE_SUPPORT
//lenovo-sw mahj2 add for multi battery End

#endif
