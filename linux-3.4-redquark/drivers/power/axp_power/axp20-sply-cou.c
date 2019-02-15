/*
 * Battery charger driver for KrossPower AXP20X
 *
 * Copyright (C) 2011 KrossPower, Ltd.
 *  Zhang Donglu <zhangdonglu@x-powers.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/mfd/axp-mfd.h>
#include <asm/div64.h>

#include <mach/sys_config.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "axp-cfg.h"
#include "axp-sply.h"

#define DBG_AXP_PSY 1
#if  DBG_AXP_PSY
#define DBG_PSY_MSG(format,args...)   printk("[AXP]"format,##args)
#else
#define DBG_PSY_MSG(format,args...)   do {} while (0)
#endif
static int axp_debug = 0;
static int vbus_curr_limit_debug = 1;
#define	DBG_AXP_APP	1
#if	 DBG_AXP_APP
#define	DBG_APP_MSG(format,args...)	  printk("[AXP-APP]"format,##args)
#else
#define	DBG_APP_MSG(format,args...)	  do {}	while (0)
#endif

#define TIMER5		3
#define BATCAPCORRATE 0
struct delayed_work usbwork;
static int pmu_suspendpwroff_vol = 0;

static int cap_count1 = 0;
static int cap_count2 = 0;
static int cap_count3 = 0;
static int cap_count4 = 0;


static int pmu_batdeten = 0;
struct axp_adc_res adc;
static int count_rdc = 0;
static int count_dis = 0;
static int pmu_earlysuspend_chgcur = 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend	axp_early_suspend;
int	early_suspend_flag = 0;
#endif

int pmu_usbvolnew = 0;
int pmu_usbcurnew = 0;
int axp_usbcurflag = 0;
int axp_usbvolflag = 0;
static int flag_cou = 0;

static int change_flag = 0;
static int pmu_usb_delayed_work_inited = 0;

void Cou_Count_Clear(struct	axp_charger	*charger);
void Set_Rest_Cap(struct axp_charger *charger, int rest_cap);
int	Get_Bat_Coulomb_Count(struct axp_charger *charger);
extern int use_cou;

int axp_usb_det(void)
{
    uint8_t ret;
    
    if(axp_charger==NULL)
    {
        return 0;
    }
    if(axp_charger->master==NULL)
        return 0;

    axp_read(axp_charger->master, AXP20_CHARGE_STATUS, &ret);
    if(ret & 0x10)
        return 1;
    else
        return 0;
}
EXPORT_SYMBOL_GPL(axp_usb_det);

int axp_usbvol(void)
{
	axp_usbvolflag = 1;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol);

int axp_usbcur(void)
{
	axp_usbcurflag = 1;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur);

int axp_usbvol_restore(void)
{
	axp_usbvolflag = 0;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol_restore);

int axp_usbcur_restore(void)
{
	axp_usbcurflag = 0;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur_restore);

static ssize_t axpdebug_store(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
	if(buf[0] == '1'){
		axp_debug = 1; 
	}
	else{
		axp_debug = 0;         
	}        
	return count;
}

static ssize_t axpdebug_show(struct class *class, 
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "bat-debug value is %d\n", axp_debug);
}

static ssize_t vbuslimit_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	if(buf[0] == '1'){
	   vbus_curr_limit_debug = 1;
    }
    else{
	   vbus_curr_limit_debug = 0;
    } 
	return count;
}

static ssize_t vbuslimit_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "vbus curr limit value is %d\n", vbus_curr_limit_debug);
}

static struct class_attribute axppower_class_attrs[] = {
    __ATTR(vbuslimit,S_IRUGO|S_IWUSR,vbuslimit_show,vbuslimit_store),
	__ATTR(axpdebug,S_IRUGO|S_IWUSR,axpdebug_show,axpdebug_store),
	__ATTR_NULL
};
static struct class axppower_class = {
	.name = "axppower",
	.class_attrs = axppower_class_attrs,
};

int ADC_Freq_Get(struct axp_charger *charger)
{
	uint8_t  temp;
	int rValue = 25;

	axp_read(charger->master, AXP20_ADC_CONTROL3,&temp);
	temp &= 0xc0;
	switch(temp >> 6)
	{
		case 0:
			rValue = 25;
			break;
		case 1:
			rValue = 50;
			break;
		case 2:
			rValue = 100;
			break;
		case 3:
			rValue = 200;
			break;
		default:
			break;
	}
	return rValue;
}

static inline int axp20_vbat_to_mV(uint16_t reg)
{
	return ((int)((( reg >> 8) << 4 ) | (reg & 0x000F))) * 1100 / 1000;
}

static inline int axp20_vdc_to_mV(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 1700 / 1000;
}


static inline int axp20_ibat_to_mA(uint16_t reg)
{
	return ((int)(((reg >> 8) << 5 ) | (reg & 0x001F))) * 500 / 1000;
}

static inline int axp20_icharge_to_mA(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 500 / 1000;
}

static inline int axp20_iac_to_mA(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 625 / 1000;
}

static inline int axp20_iusb_to_mA(uint16_t reg)
{
	return ((int)(((reg >> 8) << 4 ) | (reg & 0x000F))) * 375 / 1000;
}


static inline void axp_read_adc(struct axp_charger *charger,
  struct axp_adc_res *adc)
{
	uint8_t tmp[8];

	axp_reads(charger->master,AXP20_VACH_RES,8,tmp);
	adc->vac_res = ((uint16_t) tmp[0] << 8 )| tmp[1];
	adc->iac_res = ((uint16_t) tmp[2] << 8 )| tmp[3];
	adc->vusb_res = ((uint16_t) tmp[4] << 8 )| tmp[5];
	adc->iusb_res = ((uint16_t) tmp[6] << 8 )| tmp[7];
	axp_reads(charger->master,AXP20_VBATH_RES,6,tmp);
	adc->vbat_res = ((uint16_t) tmp[0] << 8 )| tmp[1];
	adc->ichar_res = ((uint16_t) tmp[2] << 8 )| tmp[3];
	adc->idischar_res = ((uint16_t) tmp[4] << 8 )| tmp[5];
}

/*set CPU-VDD,AVCC,DRAM-VDD,LDO3,LDO4 power up first ,after 4ms Core_VDD,VCC-3V3 power up*/
static void axp_set_startup_sequence(struct axp_charger *charger)
{
	axp_write(charger->master,0xF4,0x06); //open REGF2/5 Lock
	axp_write(charger->master,0xF2,0x04); //open REG10x Lock
	axp_write(charger->master,0xFF,0x01);
	axp_write(charger->master,0x03,0x00); //set EXTEN power up at step 2 and 4ms step by step
					     // highest 2bit depend the startup time,00-1ms,01-4ms,10-16ms,11-32ms
	axp_write(charger->master,0x04,0x00); //set Core-VDD power up at step 2
	axp_write(charger->master,0xFF,0x00);
	axp_write(charger->master,0xF4,0x00); //Close all Lock
	if(axp_debug){
		uint8_t val;
		axp_reads(charger->master,0xF4,1,&val);
		DBG_PSY_MSG("axp209 REGF4  = %x\n",val);
		axp_reads(charger->master,0xF2,1,&val);
		DBG_PSY_MSG("axp209 REGF2  = %x\n",val);
		axp_write(charger->master,0xFF,0x01);
		axp_reads(charger->master,0xF2,1,&val);
		DBG_PSY_MSG("first axp209 REGFF  = %x\n",val);
		axp_reads(charger->master,0x03,1,&val);
		DBG_PSY_MSG("axp209 REG103  = %x\n",val);
		axp_reads(charger->master,0x04,1,&val);
		DBG_PSY_MSG("axp209 REG104  = %x\n",val);
		axp_write(charger->master,0xFF,0x01);
		axp_reads(charger->master,0xF2,1,&val);
		DBG_PSY_MSG("after axp209 REGFF  = %x\n",val);
		}
}

static void axp_charger_update_state(struct axp_charger *charger)
{
	uint8_t val[2];
	uint16_t tmp;
	axp_reads(charger->master,AXP20_CHARGE_STATUS,2,val);
	tmp = (val[1] << 8 )+ val[0];
	charger->is_on = (val[1] & AXP20_IN_CHARGE) ? 1 : 0;
	charger->fault = val[1];
	charger->bat_det = (tmp & AXP20_STATUS_BATEN)?1:0;
	charger->ac_det = (tmp & AXP20_STATUS_ACEN)?1:0;
	charger->usb_det = (tmp & AXP20_STATUS_USBEN)?1:0;
	charger->usb_valid = (tmp & AXP20_STATUS_USBVA)?1:0;
	charger->ac_valid = (tmp & AXP20_STATUS_ACVA)?1:0;
	charger->ext_valid = charger->ac_valid | charger->usb_valid;
	charger->bat_current_direction = (tmp & AXP20_STATUS_BATCURDIR)?1:0;
	charger->in_short = (tmp& AXP20_STATUS_ACUSBSH)?1:0;
	charger->batery_active = (tmp & AXP20_STATUS_BATINACT)?1:0;
	charger->low_charge_current = (tmp & AXP20_STATUS_CHACURLOEXP)?1:0;
	charger->int_over_temp = (tmp & AXP20_STATUS_ICTEMOV)?1:0;
    
	axp_read(charger->master,AXP20_CHARGE_CONTROL1,val);
	charger->charge_on = ((val[0] >> 7) & 0x01);

}

static void axp_charger_update(struct axp_charger *charger)
{
	uint16_t tmp;
	uint8_t val[2];
	//struct axp_adc_res adc;
	charger->adc = &adc;
	axp_read_adc(charger, &adc);
	tmp = charger->adc->vbat_res;
	charger->vbat = axp20_vbat_to_mV(tmp);
	//tmp = charger->adc->ichar_res + charger->adc->idischar_res;
	charger->ibat = ABS(axp20_icharge_to_mA(charger->adc->ichar_res)-axp20_ibat_to_mA(charger->adc->idischar_res));
	tmp = charger->adc->vac_res;
	charger->vac = axp20_vdc_to_mV(tmp);
	tmp = charger->adc->iac_res;
	charger->iac = axp20_iac_to_mA(tmp);
	tmp = charger->adc->vusb_res;
	charger->vusb = axp20_vdc_to_mV(tmp);
	tmp = charger->adc->iusb_res;
	charger->iusb = axp20_iusb_to_mA(tmp);
	axp_reads(charger->master,AXP20_INTTEMP,2,val);
	//DBG_PSY_MSG("TEMPERATURE:val1=0x%x,val2=0x%x\n",val[1],val[0]);
	tmp = (val[0] << 4 ) + (val[1] & 0x0F);
	charger->ic_temp = (int) tmp  - 1447;
	if(!charger->ext_valid) {
		charger->disvbat =  charger->vbat;
		charger->disibat =  charger->ibat;
	}else
	{
        charger->disvbat =  charger->vbat;
		charger->disibat =  axp20_ibat_to_mA(charger->adc->idischar_res);
    }
}

#if defined  (CONFIG_AXP_CHARGEINIT)
static void axp_set_charge(struct axp_charger *charger)
{
	uint8_t val=0x00;
	uint8_t tmp=0x00;
	if(charger->chgvol < 4150000)
		val &= ~(3 << 5);
	else if (charger->chgvol<4200000){
		val &= ~(3 << 5);
		val |= 1 << 5;
	}
	else if (charger->chgvol<4360000){
		val &= ~(3 << 5);
		val |= 1 << 6;
	}
	else
		val |= 3 << 5;

	if(charger->chgcur == 0)
		charger->chgen = 0;

	if(charger->chgcur< 300000)
		charger->chgcur = 300000;
	else if(charger->chgcur > 1800000)
		charger->chgcur = 1800000;

	val |= (charger->chgcur - 200001) / 100000 ;
	if(charger ->chgend == 10){
		val &= ~(1 << 4);
	}
	else {
		val |= 1 << 4;
	}
	val &= 0x7F;
	val |= charger->chgen << 7;
	if(charger->chgpretime < 30)
		charger->chgpretime = 30;
	if(charger->chgcsttime < 360)
		charger->chgcsttime = 360;

	tmp = ((((charger->chgpretime - 40) / 10) << 6)  \
		| ((charger->chgcsttime - 360) / 120));
	axp_write(charger->master, AXP20_CHARGE_CONTROL1,val);
	axp_update(charger->master, AXP20_CHARGE_CONTROL2,tmp,0xC2);
}
#else
static void axp_set_charge(struct axp_charger *charger)
{

}
#endif

static enum power_supply_property axp_battery_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	//POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	//POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CAPACITY,
	//POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	//POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property axp_ac_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static enum power_supply_property axp_usb_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static void axp_battery_check_status(struct axp_charger *charger,
            union power_supply_propval *val)
{
	if (charger->bat_det) {
		if (charger->ext_valid){
			if( charger->rest_vol == 100)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else if(charger->charge_on)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
	}
	else
		val->intval = POWER_SUPPLY_STATUS_FULL;
}

static void axp_battery_check_health(struct axp_charger *charger,
            union power_supply_propval *val)
{
	if (charger->fault & AXP20_FAULT_LOG_BATINACT)
		val->intval = POWER_SUPPLY_HEALTH_DEAD;
	else if (charger->fault & AXP20_FAULT_LOG_OVER_TEMP)
		val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (charger->fault & AXP20_FAULT_LOG_COLD)
		val->intval = POWER_SUPPLY_HEALTH_COLD;
	else
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
}

static int axp_battery_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
	struct axp_charger *charger;
	int ret = 0;
	charger = container_of(psy, struct axp_charger, batt);

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			axp_battery_check_status(charger, val);
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			axp_battery_check_health(charger, val);
			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = charger->battery_info->technology;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
			val->intval = charger->battery_info->voltage_max_design;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
			val->intval = charger->battery_info->voltage_min_design;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			//val->intval = charger->ocv * 1000;
			val->intval = charger->vbat * 1000;
			break;
		case POWER_SUPPLY_PROP_CURRENT_NOW:
			val->intval = charger->ibat * 1000;
			break;
		case POWER_SUPPLY_PROP_MODEL_NAME:
			val->strval = charger->batt.name;
			break;
		/*
		case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		case POWER_SUPPLY_PROP_CHARGE_FULL:
			val->intval = charger->battery_info->charge_full_design;
		break;
		*/
		case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
			val->intval = charger->battery_info->energy_full_design;
			//DBG_PSY_MSG("POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:%d\n",val->intval);
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			if(charger->rest_vol > 100){
				val->intval	= 100;
			} else if (charger->rest_vol < 0){
				val->intval	= 0;
			} else {
				val->intval	= charger->rest_vol;
			}
			break;
		/*
		case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
			if(charger->bat_det && !(charger->is_on) && !(charger->ext_valid))
				val->intval = charger->rest_time;
			else
				val->intval = 0;
			break;
		case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
			if(charger->bat_det && charger->is_on)
				val->intval = charger->rest_time;
			else
				val->intval = 0;
			break;
		*/
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = (!charger->is_on)&&(charger->bat_det) && (! charger->ext_valid);
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = charger->bat_det;
			break;
		case POWER_SUPPLY_PROP_TEMP:
			//val->intval = charger->ic_temp - 200;
			val->intval =  300;
			break;
		default:
			ret = -EINVAL;
			break;
		}
	return ret;
}

static int axp_ac_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
	struct axp_charger *charger;
	int ret = 0;
	charger = container_of(psy, struct axp_charger, ac);

	switch(psp){
		case POWER_SUPPLY_PROP_MODEL_NAME:
			val->strval = charger->ac.name;
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = charger->ac_det;
			break;
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = charger->ac_valid;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = charger->vac * 1000;
			break;
		case POWER_SUPPLY_PROP_CURRENT_NOW:
			val->intval = charger->iac * 1000;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int axp_usb_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
	struct axp_charger *charger;
	int ret = 0;
	charger = container_of(psy, struct axp_charger, usb);

	switch(psp){
		case POWER_SUPPLY_PROP_MODEL_NAME:
			val->strval = charger->usb.name;
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = charger->usb_det;
			break;
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = charger->usb_valid;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = charger->vusb * 1000;
			break;
		case POWER_SUPPLY_PROP_CURRENT_NOW:
			val->intval = charger->iusb * 1000;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static void axp_change(struct axp_charger *charger)
{
	uint8_t val,tmp;
	int var;
	DBG_PSY_MSG("battery state change\n");
	axp_charger_update_state(charger);
	axp_charger_update(charger);
	printk("charger->usb_valid = %d\n",charger->usb_valid);
	if(!charger->usb_valid){
       printk("pmu_usbcurnew=%d mA\n",pmu_usbcurnew);
		printk("set usb vol-lim to %d mV, cur-lim to %d mA\n",pmu_usbvolnew,pmu_usbcurnew);
		cancel_delayed_work_sync(&usbwork);
		//reset usb-pc after usb removed 
		if(pmu_usbcurnew){
          
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			var = pmu_usbcurnew * 1000;
			if(var >= 900000)
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
			else if ((var >= 500000)&& (var < 900000)){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			}
			else if ((var >= 100000)&& (var < 500000)){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
			}
			else
				printk("set usb limit current error,%d mA\n",pmu_usbcur);	
		}
		else
			axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
			
		if(pmu_usbvolnew){
			axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
			var = pmu_usbvolnew * 1000;
			if(var >= 4000000 && var <=4700000){
				tmp = (var - 4000000)/100000;
			    axp_read(charger->master, AXP20_CHARGE_VBUS,&val);
			    val &= 0xC7;
			    val |= tmp << 3;
			    axp_write(charger->master, AXP20_CHARGE_VBUS,val);
			}
			else
				printk("set usb limit voltage error,%d mV\n",pmu_usbvol);	
		}
		else
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
	} else {
	    if(pmu_usb_delayed_work_inited)
	    {
		    schedule_delayed_work(&usbwork, msecs_to_jiffies(4 * 1000));
	    }
	}
	flag_state_change = 1;
	power_supply_changed(&charger->batt);
}

static void axp_presslong(struct axp_charger *charger)
{
	DBG_PSY_MSG("press long\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
	ssleep(2);
	DBG_PSY_MSG("press long up\n");
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

static void axp_pressshort(struct axp_charger *charger)
{
	DBG_PSY_MSG("press short\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
	msleep(100);
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

static void axp_keyup(struct axp_charger *charger)
{
	DBG_PSY_MSG("power key up\n");
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
	axp_write(charger->master, POWER20_DATA_BUFFERC,0xe);
}

static void axp_keydown(struct axp_charger *charger)
{
	DBG_PSY_MSG("power key down\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
	axp_write(charger->master, POWER20_DATA_BUFFERC,0x00);
}
static void axp_capchange(struct axp_charger *charger)
{
	uint8_t val;

	DBG_PSY_MSG("battery change\n");
	ssleep(2);
	axp_charger_update_state(charger);
	axp_charger_update(charger);
	axp_read(charger->master, AXP20_CAP,&val);
	charger->rest_vol = (int) (val & 0x7F);

	if((charger->bat_det == 0) || (charger->rest_vol == 127)){
		charger->rest_vol = 100;
	}

	Cou_Count_Clear(charger);
	axp_write(charger->master, AXP20_DATA_BUFFER0,0x0);
	power_supply_changed(&charger->batt);
}

static void axp_close(struct axp_charger *charger)
{
	int	saved_cap;
	/*������ʾ���ʣ������*/
	charger->rest_vol =	BATCAPCORRATE;
	/*���ÿ�����ѯʣ��������*/
	axp_write(charger->master,AXP20_DATA_BUFFER1,0x80|BATCAPCORRATE);
	/*����ؼ�*/
	Cou_Count_Clear(charger);
	/*���û���ʣ������*/
	saved_cap =	BATCAPCORRATE;
	Set_Rest_Cap(charger,saved_cap);
	/*���õ��������У����־λ*/
	axp_set_bits(charger->master,AXP20_DATA_BUFFER0,0x20);
	/*���õ����������ʼУ���ٷֱ�*/
	axp_write(charger->master, AXP20_DATA_BUFFER5, BATCAPCORRATE);
	DBG_PSY_MSG("\n==================event in close==============\n");
	power_supply_changed(&charger->batt);
}


static int axp_battery_event(struct notifier_block *nb, unsigned long event,
        void *data)
{
	struct axp_charger *charger =
	container_of(nb, struct axp_charger, nb);
    uint8_t w[9];

	w[0] = (uint8_t) ((event) & 0xFF);
	w[1] = POWER20_INTSTS2;
	w[2] = (uint8_t) ((event >> 8) & 0xFF);
	w[3] = POWER20_INTSTS3;
	w[4] = (uint8_t) ((event >> 16) & 0xFF);
	w[5] = POWER20_INTSTS4;
	w[6] = (uint8_t) ((event >> 24) & 0xFF);
	w[7] = POWER20_INTSTS5;
	w[8] = (uint8_t) ((event) & 0xFF);
//	w[8] = (uint8_t) (((uint64_t) event >> 32) & 0xFF);
	if((bool)data==0){
		if(event & (AXP20_IRQ_BATIN|AXP20_IRQ_BATRE)) {
			axp_capchange(charger);
		}

		if(event & (AXP20_IRQ_ACIN|AXP20_IRQ_USBIN|AXP20_IRQ_ACOV|AXP20_IRQ_USBOV|AXP20_IRQ_CHAOV
					|AXP20_IRQ_CHAST|AXP20_IRQ_TEMOV|AXP20_IRQ_TEMLO)) {
			axp_change(charger);
		}

		if(event & (AXP20_IRQ_ACRE|AXP20_IRQ_USBRE)) {
			axp_change(charger);
		}

		if(event & AXP20_IRQ_PEKLO) {
			axp_presslong(charger);
		}

		if(event & AXP20_IRQ_PEKSH) {
			axp_pressshort(charger);
		}
	}
	else{

		if((event) & AXP20_IRQ_PEKFE>>32) {
			axp_keydown(charger);
		}

		if((event) & AXP20_IRQ_PEKRE>>32) {
			axp_keyup(charger);
		}
	}
	
	axp_writes(charger->master,POWER20_INTSTS1,9,w);
	DBG_PSY_MSG("%s, %d, event = 0x%x \n", __func__, __LINE__, (unsigned int)event);

	return 0;
}

static char *supply_list[] = {
	"battery",
};



static void axp_battery_setup_psy(struct axp_charger *charger)
{
	struct power_supply *batt = &charger->batt;
	struct power_supply *ac = &charger->ac;
	struct power_supply *usb = &charger->usb;
	struct power_supply_info *info = charger->battery_info;

	batt->name = "battery";
	batt->use_for_apm = info->use_for_apm;
	batt->type = POWER_SUPPLY_TYPE_BATTERY;
	batt->get_property = axp_battery_get_property;

	batt->properties = axp_battery_props;
	batt->num_properties = ARRAY_SIZE(axp_battery_props);

	ac->name = "ac";
	ac->type = POWER_SUPPLY_TYPE_MAINS;
	ac->get_property = axp_ac_get_property;

	ac->supplied_to = supply_list,
	ac->num_supplicants = ARRAY_SIZE(supply_list),

	ac->properties = axp_ac_props;
	ac->num_properties = ARRAY_SIZE(axp_ac_props);

	usb->name = "usb";
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->get_property = axp_usb_get_property;

	usb->supplied_to = supply_list,
	usb->num_supplicants = ARRAY_SIZE(supply_list),

	usb->properties = axp_usb_props;
	usb->num_properties = ARRAY_SIZE(axp_usb_props);
};

#if defined  (CONFIG_AXP_CHARGEINIT)
static int axp_battery_adc_set(struct axp_charger *charger)
{
	int ret ;
	uint8_t val;

	/*enable adc and set adc */
	val= AXP20_ADC_BATVOL_ENABLE | AXP20_ADC_BATCUR_ENABLE
	| AXP20_ADC_DCINCUR_ENABLE | AXP20_ADC_DCINVOL_ENABLE
	| AXP20_ADC_USBVOL_ENABLE | AXP20_ADC_USBCUR_ENABLE;

	ret = axp_update(charger->master, AXP20_ADC_CONTROL1, val , val);
	if (ret)
		return ret;
	ret =	axp_update(charger->master,	AXP20_COULOMB_CONTROL, AXP20_COULOMB_ENABLE	, AXP20_COULOMB_ENABLE);
	if (ret)
		return ret;
	ret = axp_read(charger->master, AXP20_ADC_CONTROL3, &val);
	switch (charger->sample_time/25){
		case 1: val &= ~(3 << 6);break;
		case 2: val &= ~(3 << 6);val |= 1 << 6;break;
		case 4: val &= ~(3 << 6);val |= 2 << 6;break;
		case 8: val |= 3 << 6;break;
		default: break;
	}
	ret = axp_write(charger->master, AXP20_ADC_CONTROL3, val);
	if (ret)
		return ret;
	return 0;
}
#else
static int axp_battery_adc_set(struct axp_charger *charger)
{
	return 0;
}
#endif

static int axp_battery_first_init(struct axp_charger *charger)
{
	int ret;
	uint8_t val;
	axp_set_charge(charger);
	ret = axp_battery_adc_set(charger);
	if(ret)
		return ret;

	ret = axp_read(charger->master, AXP20_ADC_CONTROL3, &val);
	switch ((val >> 6) & 0x03){
		case 0: charger->sample_time = 25;break;
		case 1: charger->sample_time = 50;break;
		case 2: charger->sample_time = 100;break;
		case 3: charger->sample_time = 200;break;
		default:break;
	}
	return ret;
}

static int axp_get_rdc(struct axp_charger *charger)
{
	int temp;
	int rdc;
	uint8_t v[2];
	axp_reads(charger->master,0xba,2,v);
	rdc = (((v[0] & 0x1F) << 8) | v[1]) * 10742 / 10000;
	DBG_PSY_MSG("===========================calculate rdc \n");

	if(!charger->bat_det){
		charger->disvbat = 0;
		charger->disibat = 0;
		return rdc;
	}
	if( charger->ext_valid){
		axp_charger_update(charger);
		if( axp20_ibat_to_mA(charger->adc->idischar_res) == 0 && axp20_icharge_to_mA(charger->adc->ichar_res) > 200){
		} else {
			DBG_PSY_MSG("%s->%d\n",__FUNCTION__,__LINE__);
			charger->disvbat = 0;
			charger->disibat = 0;
			return rdc;
		}
		DBG_PSY_MSG("CHARGING:      charger->vbat = %d,   charger->ibat = %d\n",charger->vbat,charger->ibat);
		DBG_PSY_MSG("DISCHARGING:charger->disvbat = %d,charger->disibat = %d\n",charger->disvbat,charger->disibat);
		axp_charger_update_state(charger);
		if(!charger->bat_current_direction){
			charger->disvbat = 0;
			charger->disibat = 0;
			return rdc;
		}
		if(charger->disvbat == 0){
			charger->disvbat = 0;
			charger->disibat = 0;
			return rdc;
		} else {
			temp = 1000 * ABS(charger->vbat - charger->disvbat) / ABS(charger->ibat + charger->disibat);
			DBG_PSY_MSG("CALRDC:temp = %d\n",temp);
			charger->disvbat = 0;
			charger->disibat = 0;
			if((temp < 75) || (temp > pmu_battery_rdc * 3)){
				return rdc;
			} else {
				axp_set_bits(charger->master,0x04,0x08);
				return temp;
			}
		}
	} else {
		charger->disvbat = 0;
		charger->disibat = 0;
		return rdc;
	}
}

static ssize_t chgen_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master, AXP20_CHARGE_CONTROL1, &val);
	charger->chgen  = val >> 7;
	return sprintf(buf, "%d\n",charger->chgen);
}

static ssize_t chgen_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	var = simple_strtoul(buf, NULL, 10);
	if(var){
		charger->chgen = 1;
		axp_set_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
	} else {
		charger->chgen = 0;
		axp_clr_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
	}
	return count;
}

static ssize_t chgmicrovol_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master, AXP20_CHARGE_CONTROL1, &val);
	switch ((val >> 5) & 0x03){
		case 0: charger->chgvol = 4100000;break;
		case 1: charger->chgvol = 4150000;break;
		case 2: charger->chgvol = 4200000;break;
		case 3: charger->chgvol = 4360000;break;
	}
	return sprintf(buf, "%d\n",charger->chgvol);
}

static ssize_t chgmicrovol_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	uint8_t tmp, val;
	var = simple_strtoul(buf, NULL, 10);
	switch(var){
		case 4100000:tmp = 0;break;
		case 4150000:tmp = 1;break;
		case 4200000:tmp = 2;break;
		case 4360000:tmp = 3;break;
		default:  tmp = 4;break;
	}
	if(tmp < 4){
		charger->chgvol = var;
		axp_read(charger->master, AXP20_CHARGE_CONTROL1, &val);
		val &= 0x9F;
		val |= tmp << 5;
		axp_write(charger->master, AXP20_CHARGE_CONTROL1, val);
	}
	return count;
}

static ssize_t chgintmicrocur_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master, AXP20_CHARGE_CONTROL1, &val);
	charger->chgcur = (val & 0x0F) * 100000 +300000;
	return sprintf(buf, "%d\n",charger->chgcur);
}

static ssize_t chgintmicrocur_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	uint8_t val,tmp;
	var = simple_strtoul(buf, NULL, 10);
	if(var >= 300000 && var <= 1800000){
		tmp = (var -200001)/100000;
		charger->chgcur = tmp *100000 + 300000;
		axp_read(charger->master, AXP20_CHARGE_CONTROL1, &val);
		val &= 0xF0;
		val |= tmp;
		axp_write(charger->master, AXP20_CHARGE_CONTROL1, val);
	}
	return count;
}

static ssize_t chgendcur_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master, AXP20_CHARGE_CONTROL1, &val);
	charger->chgend = ((val >> 4)& 0x01)? 15 : 10;
	return sprintf(buf, "%d\n",charger->chgend);
}

static ssize_t chgendcur_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	var = simple_strtoul(buf, NULL, 10);
	if(var == 10 ){
		charger->chgend = var;
		axp_clr_bits(charger->master ,AXP20_CHARGE_CONTROL1,0x10);
	} else if (var == 15){
		charger->chgend = var;
		axp_set_bits(charger->master ,AXP20_CHARGE_CONTROL1,0x10);
	}
	return count;
}

static ssize_t chgpretimemin_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master,AXP20_CHARGE_CONTROL2, &val);
	charger->chgpretime = (val >> 6) * 10 +40;
	return sprintf(buf, "%d\n",charger->chgpretime);
}

static ssize_t chgpretimemin_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	uint8_t tmp,val;
	var = simple_strtoul(buf, NULL, 10);
	if(var >= 40 && var <= 70){
		tmp = (var - 40)/10;
		charger->chgpretime = tmp * 10 + 40;
		axp_read(charger->master,AXP20_CHARGE_CONTROL2,&val);
		val &= 0x3F;
		val |= (tmp << 6);
		axp_write(charger->master,AXP20_CHARGE_CONTROL2,val);
	}
	return count;
}

static ssize_t chgcsttimemin_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master,AXP20_CHARGE_CONTROL2, &val);
	charger->chgcsttime = (val & 0x03) *120 + 360;
	return sprintf(buf, "%d\n",charger->chgcsttime);
}

static ssize_t chgcsttimemin_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	uint8_t tmp,val;
	var = simple_strtoul(buf, NULL, 10);
	if(var >= 360 && var <= 720){
		tmp = (var - 360)/120;
		charger->chgcsttime = tmp * 120 + 360;
		axp_read(charger->master,AXP20_CHARGE_CONTROL2,&val);
    	val &= 0xFC;
		val |= tmp;
		axp_write(charger->master,AXP20_CHARGE_CONTROL2,val);
	}
	return count;
}

static ssize_t adcfreq_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master, AXP20_ADC_CONTROL3, &val);
	switch ((val >> 6) & 0x03){
		case 0: charger->sample_time = 25;break;
		case 1: charger->sample_time = 50;break;
		case 2: charger->sample_time = 100;break;
		case 3: charger->sample_time = 200;break;
		default:break;
	}
	return sprintf(buf, "%d\n",charger->sample_time);
}

static ssize_t adcfreq_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	uint8_t val;
	var = simple_strtoul(buf, NULL, 10);
	axp_read(charger->master, AXP20_ADC_CONTROL3, &val);
	switch (var/25){
		case 1: val &= ~(3 << 6);charger->sample_time = 25;break;
		case 2: val &= ~(3 << 6);val |= 1 << 6;charger->sample_time = 50;break;
		case 4: val &= ~(3 << 6);val |= 2 << 6;charger->sample_time = 100;break;
		case 8: val |= 3 << 6;charger->sample_time = 200;break;
		default: break;
	}
	axp_write(charger->master, AXP20_ADC_CONTROL3, val);
	return count;
}


static ssize_t vholden_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master,AXP20_CHARGE_VBUS, &val);
	val = (val>>6) & 0x01;
	return sprintf(buf, "%d\n",val);
}

static ssize_t vholden_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	var = simple_strtoul(buf, NULL, 10);
	if(var)
		axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
	else
		axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);

	return count;
}

static ssize_t vhold_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	int vhold;
	axp_read(charger->master,AXP20_CHARGE_VBUS, &val);
	vhold = ((val >> 3) & 0x07) * 100000 + 4000000;
	return sprintf(buf, "%d\n",vhold);
}

static ssize_t vhold_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	uint8_t val,tmp;
	var = simple_strtoul(buf, NULL, 10);
	if(var >= 4000000 && var <=4700000){
		tmp = (var - 4000000)/100000;
		//printk("tmp = 0x%x\n",tmp);
		axp_read(charger->master, AXP20_CHARGE_VBUS,&val);
		val &= 0xC7;
		val |= tmp << 3;
		//printk("val = 0x%x\n",val);
		axp_write(charger->master, AXP20_CHARGE_VBUS,val);
	}
	return count;
}

static ssize_t iholden_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;
	axp_read(charger->master,AXP20_CHARGE_VBUS, &val);
	return sprintf(buf, "%d\n",((val & 0x03) == 0x03)?0:1);
}

static ssize_t iholden_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	var = simple_strtoul(buf, NULL, 10);
	if(var)
		axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
	else
		axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);

	return count;
}

static ssize_t ihold_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val,tmp;
	int ihold;
	axp_read(charger->master,AXP20_CHARGE_VBUS, &val);
	tmp = (val) & 0x03;
	switch(tmp){
		case 0: ihold = 900000;break;
		case 1: ihold = 500000;break;
		case 2: ihold = 100000;break;
		default: ihold = 0;break;
	}
	return sprintf(buf, "%d\n",ihold);
}

static ssize_t ihold_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;
	var = simple_strtoul(buf, NULL, 10);
	if(var >= 900000) {
		axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
	} else if ((var >= 500000) && (var < 900000)) {
		axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
		axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
	} else if ((var >= 100000) && (var < 500000)) {
		axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
		axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
	} else {
		printk("input error\n");	
	}
	return count;
}

static struct device_attribute axp_charger_attrs[] = {
	AXP_CHG_ATTR(chgen),
	AXP_CHG_ATTR(chgmicrovol),
	AXP_CHG_ATTR(chgintmicrocur),
	AXP_CHG_ATTR(chgendcur),
	AXP_CHG_ATTR(chgpretimemin),
	AXP_CHG_ATTR(chgcsttimemin),
	AXP_CHG_ATTR(adcfreq),
	AXP_CHG_ATTR(vholden),
	AXP_CHG_ATTR(vhold),
	AXP_CHG_ATTR(iholden),
	AXP_CHG_ATTR(ihold),
};

#if defined CONFIG_HAS_EARLYSUSPEND
static void axp_earlysuspend(struct early_suspend *h)
{
	uint8_t tmp;
	volatile int val;
	DBG_PSY_MSG("======early suspend=======\n");

#if defined (CONFIG_AXP_CHGCHANGE)
	early_suspend_flag = 1;
	if(pmu_earlysuspend_chgcur == 0)
		axp_clr_bits(axp_charger->master,AXP20_CHARGE_CONTROL1,0x80);
	else
		axp_set_bits(axp_charger->master,AXP20_CHARGE_CONTROL1,0x80);

	if(pmu_earlysuspend_chgcur >= 300000 && pmu_earlysuspend_chgcur <= 1800000) {
		tmp = (pmu_earlysuspend_chgcur -200001)/100000;
		axp_update(axp_charger->master, AXP20_CHARGE_CONTROL1, tmp,0x0F);
	}
	DBG_APP_MSG("pmu_earlysuspend_chgcur = %d\n",pmu_earlysuspend_chgcur);
#endif
	/*set suspendpoweroff level*/
	DBG_APP_MSG("[earlysuspend]pmu_suspendpwroff_vol = %d\n",pmu_suspendpwroff_vol);
	if(pmu_suspendpwroff_vol >= 2867200 && pmu_suspendpwroff_vol <= 4200000) {
		val = (pmu_suspendpwroff_vol - 2867200) / 5600;	
	}
	DBG_APP_MSG("[earlysuspend]pmu_suspendpwroff_vol val = 0x%x\n",val);
	axp_write(axp_charger->master, AXP20_APS_WARNING1,(val-0x35));
	axp_write(axp_charger->master, AXP20_APS_WARNING2,(val-0x58));
}

static void axp_lateresume(struct early_suspend *h)
{
	uint8_t tmp;
	DBG_PSY_MSG("======late resume=======\n");

#if defined (CONFIG_AXP_CHGCHANGE)
	early_suspend_flag = 0;
	if(pmu_resume_chgcur == 0)
		axp_clr_bits(axp_charger->master,AXP20_CHARGE_CONTROL1,0x80);
	else
		axp_set_bits(axp_charger->master,AXP20_CHARGE_CONTROL1,0x80);

	if(pmu_resume_chgcur >= 300000 && pmu_resume_chgcur <= 1800000){
		tmp = (pmu_resume_chgcur -200001)/100000;
		axp_update(axp_charger->master, AXP20_CHARGE_CONTROL1, tmp,0x0F);
	}
	DBG_APP_MSG("pmu_resume_chgcur = %d\n",pmu_resume_chgcur);
#endif
}
#endif

int axp_charger_create_attrs(struct power_supply *psy)
{
	int j,ret;
	for (j = 0; j < ARRAY_SIZE(axp_charger_attrs); j++) {
		ret = device_create_file(psy->dev,
			&axp_charger_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(psy->dev,
			&axp_charger_attrs[j]);
succeed:
	return ret;
}

void Set_Rest_Cap(struct axp_charger *charger, int rest_cap)
{
	uint8_t	val;
	if(rest_cap	>= 0)
		val	= rest_cap & 0x7F;
	else
		val	= ABS(rest_cap)	| 0x80;
	axp_write(charger->master, AXP20_DATA_BUFFER4, val);
    printk("****************Set_Rset_Cap=%d***********\n",rest_cap);
}

void Get_Rest_Cap(struct axp_charger *charger, int *rest_cap)
{
	uint8_t	val;
	axp_read(charger->master, AXP20_DATA_BUFFER4, &val);
	if((val	& 0x80)	>> 7)
		*rest_cap =	(int) (0 - (val	& 0x7F));
	else
		*rest_cap =	(int) (val & 0x7F);
}

void Cou_Count_Clear(struct	axp_charger	*charger)
{
	uint8_t	temp = 0xff;
	axp_read(charger->master, AXP20_COULOMB_CONTROL, &temp);
	temp |=	0x20;
	temp &=	0xbf;
	axp_write(charger->master, AXP20_COULOMB_CONTROL, temp);
	temp |=	0x80;
	temp &=	0xbf;
	axp_write(charger->master, AXP20_COULOMB_CONTROL, temp);
 
}

void Buffer_Cou_Set(struct axp_charger *charger,uint16_t Cou_Counter)
{
	uint8_t	temp[3]	= {0,AXP20_DATA_BUFFER3,0};
	Cou_Counter	|= 0x8000;
	temp[0]	= ((Cou_Counter	& 0xff00) >> 8);
	temp[2]	= (Cou_Counter & 0x00ff);
	axp_writes(charger->master,AXP20_DATA_BUFFER2,3,temp);
}

uint16_t Get_Buffer_Cou(struct axp_charger *charger)
{
	uint8_t	temp[2];
	uint16_t rValue;
	axp_reads(charger->master, AXP20_DATA_BUFFER2,2,temp);
	rValue = ((temp[0] << 8) + temp[1]);
	if(rValue &	0x8000)
		return ((rValue	& 0x7fff));
	else
		return pmu_battery_cap;
}

int Get_Bat_Coulomb_Count(struct axp_charger *charger)
{
	uint8_t  temp[8];
	int64_t  rValue1,rValue2,rValue;
	int Cur_CoulombCounter_tmp,m;

	axp_reads(charger->master, AXP20_CCHAR3_RES,8,temp);
	rValue1 = ((temp[0] << 24) + (temp[1] << 16) + (temp[2] << 8) + temp[3]);
	rValue2 = ((temp[4] << 24) + (temp[5] << 16) + (temp[6] << 8) + temp[7]);
	if(axp_debug){
		DBG_PSY_MSG("%s->%d -     CHARGINGOULB:[0]=0x%x,[1]=0x%x,[2]=0x%x,[3]=0x%x\n",__FUNCTION__,__LINE__,temp[0],temp[1],temp[2],temp[3]);
		DBG_PSY_MSG("%s->%d - DISCHARGINGCLOUB:[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x\n",__FUNCTION__,__LINE__,temp[4],temp[5],temp[6],temp[7]);
	}
	rValue = (ABS(rValue1 - rValue2)) * 4369;
	m = ADC_Freq_Get(charger) * 480;
	do_div(rValue,m);
	if(rValue1 >= rValue2)
		Cur_CoulombCounter_tmp = (int)rValue;
	else
		Cur_CoulombCounter_tmp = (int)(0 - rValue);
	return Cur_CoulombCounter_tmp;				//unit mAh
}

static void axp_charging_monitor(struct work_struct *work)
{
	struct axp_charger *charger;
	uint8_t	val;
	uint8_t v[5];
	int	pre_rest_vol;
	int rdc;
	int rt_rest_vol;
	uint16_t tmp;
	int Cur_CoulombCounter;
	int	saved_cap;
	int	Cou_Correction_Flag;
	int	flag_notfristin;
	uint8_t data_mm[12];
	int mm;

	charger = container_of(work, struct axp_charger, work.work);
	axp_reads(charger->master,AXP20_IC_TYPE,2,v);
	flag_notfristin	= (v[1]	>> 6) &	0x1;
	Cou_Correction_Flag	= (v[1]	>> 5) &	0x1;
	pre_rest_vol = charger->rest_vol;
	axp_charger_update_state(charger);
	axp_charger_update(charger);

	if(charger->is_on && axp20_icharge_to_mA(charger->adc->ichar_res) > 200 && charger->vbat > 3500 && charger->disvbat != 0){
		if((((v[1] >> 7) == 0) || (((v[1] >> 3) & 0x1) == 0)) && count_rdc >= 3){
			axp_set_bits(charger->master,AXP20_CAP,0x80);
			axp_clr_bits(charger->master,0xBA,0x80);
			rdc = (axp_get_rdc(charger) * 10000 + 5371) / 10742;
			tmp = (uint16_t) rdc;
			axp_write(charger->master,0xBB,tmp & 0x00FF);
			axp_update(charger->master, 0xBA, (tmp >> 8), 0x1F);
			axp_clr_bits(charger->master,AXP20_CAP,0x80);
			axp_set_bits(charger->master,0x04,0x80);
			if(axp_debug){
				DBG_PSY_MSG("=rdc = %d\n",rdc * 10742 / 10000);
			}
			count_rdc = 0;
		} else if((((v[1] >> 7) == 0) || (((v[1] >> 3) & 0x1) == 0)) && count_rdc < 3){
			count_rdc ++;  
		} else {
			count_rdc = 0; 
		}
	} else {
		count_rdc = 0;
	}

	axp_read(charger->master, AXP20_CAP,&val);
	rt_rest_vol	= (int)	(val & 0x7F);
	if((charger->bat_det ==	0) || (rt_rest_vol == 127) ){
		rt_rest_vol	= 100;
	}
	if(axp_debug){
		DBG_PSY_MSG("rt_rest_vol = %d\n",rt_rest_vol);
	}
	
	if(change_flag){
		DBG_PSY_MSG("-----------in change_flag-----------\n");
		change_flag	= 0;
		if(ABS(rt_rest_vol - charger->ocv_rest_vol)	> 5){
			DBG_PSY_MSG("-----------correct	rdc-----------\n");
			axp_clr_bits(charger->master,0x04,0x08);
			//ssleep(1);
			axp_read(charger->master, AXP20_CAP,&val);
			rt_rest_vol	= (int)	(val & 0x7F);
			if((charger->bat_det ==	0) || (rt_rest_vol == 127) ){
				rt_rest_vol	= 100;
			}
		}
	}
	
	if(flag_state_change){
		rt_rest_vol	= charger->ocv_rest_vol;
		flag_state_change ++;
		if(flag_state_change >=4){
			flag_state_change =	0;
			change_flag	= 1;
		}
		if(axp_debug)
			DBG_PSY_MSG("==flag_state_change =	%d==(when >= 4 corrent rdc)=\n",flag_state_change);
	}
	
	charger->ocv_rest_vol =	rt_rest_vol;

	Get_Rest_Cap(charger,&saved_cap);
		
	if(	flag_notfristin	){
		Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
		if(axp_debug){
			DBG_PSY_MSG("Cur_CoulombCounter	= %d\n", Cur_CoulombCounter);
		}
		charger->rest_vol =	100	* (Cur_CoulombCounter) / bat_cap + saved_cap;
		if(Cur_CoulombCounter < 0){
			charger->rest_vol = charger->rest_vol - 1;
		}
		/*
		if((charger->rest_vol <= (BATCAPCORRATE))&&(rt_rest_vol <= (BATCAPCORRATE + 1)) &&(!charger->ext_valid) && (!flag_cou)){
			Cou_Correction_Flag	= 1;
			flag_cou = 1;
			Set_Rest_Cap(charger,BATCAPCORRATE);
			axp_write(charger->master, AXP20_DATA_BUFFER5, rt_rest_vol);
			Cou_Count_Clear(charger);
			axp_set_bits(charger->master,AXP20_DATA_BUFFER0,0x20);
			printk("\n ============Capacity	Calibration	Start============ \n");
			printk("\n ============	  Rest	Capacity  =	 %d	 ============ \n",rt_rest_vol);
		}
		*/
		if(charger->rest_vol > BATCAPCORRATE){
			flag_cou = 0;
		}
		
		if(axp_debug){
			DBG_PSY_MSG("Before modify:charger->rest_vol = %d\n",charger->rest_vol);
		}
		
		if(charger->rest_vol > 100){
			charger->rest_vol =	100;
		} else if (charger->rest_vol < 0){
			charger->rest_vol =	0;
		}

		/*���ʣ������У��������С��95��ocv�ٷֱȴ���95�����ڳ��ʱ�����ʣ�������Լ�*/
		if((rt_rest_vol	> 94) && (charger->rest_vol	< 95) && (charger->bat_current_direction ==	1)) {
			if(cap_count1 >= TIMER5) {
				DBG_PSY_MSG("Correct1:rt_rest_vol = %d,charger->rest_vol =%d +1\n",rt_rest_vol,charger->rest_vol);
				charger->rest_vol ++;
				saved_cap ++;
				Set_Rest_Cap(charger,saved_cap);
				cap_count1 = 0;
			} else {
				cap_count1++;	
			}
		} else {
			cap_count1 = 0;	
		}

		/*���ʣ������У��������С��100��ocv����4.1V���ⲿ������ڣ��ŵ����Ϊ0,��δ�ڳ��ʱ�����ʣ�������Լӣ�ֱ��100*/
		if((charger->vbat >= 4100) && (charger->rest_vol < 100) && (charger->bat_current_direction == 0)	&& (charger->ext_valid)&&(charger->charge_on) && (!charger->disibat)) {
			if(cap_count2 >= TIMER5) {
				DBG_PSY_MSG("Correct2:fix 100,charger->rest_vol = %d -> 100%%\n",charger->rest_vol);
				charger->rest_vol ++;
				saved_cap ++;
				Set_Rest_Cap(charger,saved_cap);
				cap_count2 = 0;
			} else {
				cap_count2++;	
			}
		} else {
			cap_count2 = 0;	
		}

		/*���ʣ������У��������С�ڣ����������У���ٷ�+5����ocv�ٷֱȴ��ڣ����������У���ٷֱ�+4�������ڷŵ�ʱ�����ʣ���������ֲ��䣬��ocv�ٷֱȽ�����*/
		if((rt_rest_vol	> (BATCAPCORRATE + 4)) &&	(charger->rest_vol < (BATCAPCORRATE+ 5)) && charger->bat_current_direction == 0) {
			DBG_PSY_MSG("Correct3:discharging:(rt_rest_vol > %d)&&(charger->rest_vol < %d)\n",BATCAPCORRATE+4,BATCAPCORRATE+5);
			if(pre_rest_vol	> charger->rest_vol) {
				Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
				saved_cap =	pre_rest_vol - 100 * (Cur_CoulombCounter) /	bat_cap;
				Set_Rest_Cap(charger,saved_cap);
			}
			charger->rest_vol =	 pre_rest_vol;
		}

		/*���ʣ������У����������ڣ����������У���ٷ�+12����ocv�ٷֱ�С�ڣ����������У���ٷֱ�+13�������ڷŵ�ʱ�����ʣ�������Լ�*/
		if((rt_rest_vol	< (BATCAPCORRATE + 14)) &&	(charger->rest_vol > (BATCAPCORRATE + 13)) && (charger->bat_current_direction == 0)) {
			if(cap_count4 >= TIMER5) {
				DBG_PSY_MSG("Correct5:discharging:(rt_rest_vol < %d)&&(charger->rest_vol > %d)\n",BATCAPCORRATE+13,BATCAPCORRATE+12);
				charger->rest_vol --;
				saved_cap --;
				Set_Rest_Cap(charger,saved_cap);
				cap_count4 = 0;
			} else {
				cap_count4++;
			}
		} else {
			cap_count4 = 0;	
		}
		
		/*���ʣ������У����������ڣ����������У���ٷ�+4����ocv�ٷֱ�С�ڣ����������У���ٷֱ�+5�������ڷŵ�ʱ�����ʣ�������Լ�*/
		if((rt_rest_vol	< (BATCAPCORRATE + 6)) &&	(charger->rest_vol > (BATCAPCORRATE + 5)) && (charger->bat_current_direction == 0)) {
			if(cap_count3 >= TIMER5) {
				DBG_PSY_MSG("Correct4:discharging:(rt_rest_vol < %d)&&(charger->rest_vol > %d)\n",BATCAPCORRATE+5,BATCAPCORRATE+4);
				charger->rest_vol --;
				saved_cap --;
				Set_Rest_Cap(charger,saved_cap);
				cap_count3 = 0;
			} else {
				cap_count3++;
			}
		} else {
			cap_count3 = 0;	
		}
		
		/*���ʣ������У����������ڵ���100���һ��ڳ��ʱ�����ʣ�������ٷֱ�����Ϊ99*/
		if((charger->bat_current_direction == 1) &&	(charger->rest_vol >= 100)) {
			DBG_PSY_MSG("Correct5:fix 99:charger->rest_vol = %d\n",charger->rest_vol);
			charger->rest_vol =	99;
			Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
			saved_cap =	99 - 100 * (Cur_CoulombCounter)	/ bat_cap;
			Set_Rest_Cap(charger,saved_cap);
		}
		
		/*���������У��*/
		if(Cou_Correction_Flag && (charger->bat_current_direction  == 0) &&	(charger->rest_vol == 100))	{
			axp_read(charger->master, AXP20_DATA_BUFFER5, &val);
			Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
			bat_cap	= ABS(Cur_CoulombCounter) / (100 - val) * 100;
			Buffer_Cou_Set(charger,bat_cap);
			Cou_Correction_Flag	= 0;
			axp_clr_bits(charger->master,AXP20_DATA_BUFFER0,0x20);
			printk("\n ===========Capacity calibration finished============	\n");
			printk("\n ============	  Battery capacity =  %d   ============	\n",bat_cap);
			saved_cap =	100	- 100 *	(Cur_CoulombCounter) / bat_cap;
			Set_Rest_Cap(charger,saved_cap);
		}
	} else {
		charger->rest_vol =	rt_rest_vol;
		DBG_PSY_MSG("init charger->rest_vol = %d\n",charger->rest_vol);
		/* full	*/
		if(charger->vbat >=	4100 &&	!charger->is_on	&& charger->ext_valid &&(charger->charge_on)&&(!charger->disibat)) {
			charger->rest_vol =	100;
		}
		/* charging*/
		if(charger->is_on && charger->rest_vol == 100){
			charger->rest_vol =	99;
		}
		Set_Rest_Cap(charger,charger->rest_vol);
		Cou_Count_Clear(charger);
		axp_set_bits(charger->master, AXP20_DATA_BUFFER0, 0x40);
		flag_notfristin	= 1;
	}

	if((count_dis >= 8) && (charger->disvbat != 0)){
		charger->disvbat = 0;
		charger->disibat = 0;
		count_dis = 0;
	}
	if(charger->bat_current_direction == 1){
		count_dis++;
	} else {
		count_dis = 0;
	}
	
	axp_reads(charger->master,0xbc,2,v);
	charger->ocv = ((v[0] << 4)	+ (v[1]	& 0x0f)) * 11 /10 ;
	
	/* ���ʣ������У�� */
	if(!charger->ext_valid){
		if(charger->rest_vol > pre_rest_vol){
			charger->rest_vol = pre_rest_vol;
		}
	}
	
	/* ������ */
	if(ABS(charger->rest_vol - rt_rest_vol) > 40){
		axp_clr_bits(charger->master, AXP20_DATA_BUFFER0, 0x60);
	}
	
	if((rt_rest_vol <= BATCAPCORRATE)&&(charger->rest_vol > (BATCAPCORRATE + 5))){
		axp_clr_bits(charger->master, AXP20_DATA_BUFFER0, 0x60);
	}
	/*set capacity calibration flag*/
	if((charger->rest_vol <= (BATCAPCORRATE))&&(rt_rest_vol <= (BATCAPCORRATE + 1)) &&(!charger->ext_valid) && (!flag_cou)){
		Cou_Correction_Flag	= 1;
		flag_cou = 1;
		Set_Rest_Cap(charger,BATCAPCORRATE);
		axp_write(charger->master, AXP20_DATA_BUFFER5, rt_rest_vol);
		Cou_Count_Clear(charger);
		axp_set_bits(charger->master,AXP20_DATA_BUFFER0,0x20);//Cou_Correction_Flag
		printk("\n ============Capacity	Calibration	Start============ \n");
		printk("\n ============	  Rest	Capacity  =	 %d	 ============ \n",rt_rest_vol);
	}
	if(axp_debug){
		DBG_PSY_MSG("charger->ic_temp = %d\n",charger->ic_temp);
		DBG_PSY_MSG("charger->vbat = %d\n",charger->vbat);
		DBG_PSY_MSG("charger->ibat = %d\n",charger->ibat);
		DBG_PSY_MSG("charger->vusb = %d\n",charger->vusb);
		DBG_PSY_MSG("charger->iusb = %d\n",charger->iusb);
		DBG_PSY_MSG("charger->vac = %d\n",charger->vac);
		DBG_PSY_MSG("charger->iac = %d\n",charger->iac);
		DBG_PSY_MSG("charger->ocv = %d\n",charger->ocv);
		DBG_PSY_MSG("charger->disvbat = %d\n",charger->disvbat);
		DBG_PSY_MSG("charger->disibat = %d\n",charger->disibat);
		DBG_PSY_MSG("charger->ocv_rest_vol = %d\n",charger->ocv_rest_vol);
		DBG_PSY_MSG("charger->rest_vol = %d\n",charger->rest_vol);
		axp_reads(charger->master,0xba,2,v);
		rdc = (((v[0] & 0x1F) << 8) | v[1]) * 10742 / 10000;
		DBG_PSY_MSG("rdc = %d\n",rdc);
		DBG_PSY_MSG("bat_cap = %d\n",bat_cap);
		DBG_PSY_MSG("charger->is_on = %d\n",charger->is_on);
		DBG_PSY_MSG("charger->charge_on = %d\n",charger->charge_on);
		DBG_PSY_MSG("charger->ext_valid = %d\n",charger->ext_valid);
		DBG_PSY_MSG("count_dis = %d\n",count_dis);
		DBG_PSY_MSG("count_rdc = %d\n",count_rdc);
		DBG_PSY_MSG("pmu_init_chgcur           = %d\n",pmu_init_chgcur);
		DBG_PSY_MSG("pmu_earlysuspend_chgcur   = %d\n",pmu_earlysuspend_chgcur);
		DBG_PSY_MSG("pmu_suspend_chgcur        = %d\n",pmu_suspend_chgcur);
		DBG_PSY_MSG("pmu_resume_chgcur         = %d\n",pmu_resume_chgcur);
		DBG_PSY_MSG("pmu_shutdown_chgcur       = %d\n",pmu_shutdown_chgcur);
		axp_reads(charger->master,AXP20_DATA_BUFFER0,12,data_mm);
		for( mm = 0; mm < 12; mm++){
			DBG_PSY_MSG("REG[0x%x] = 0x%x\n",mm+AXP20_DATA_BUFFER0,data_mm[mm]);	
		}
	}
	
	/* if battery volume changed, inform uevent */
	if(charger->rest_vol - pre_rest_vol){
		printk("battery vol change: %d->%d \n", pre_rest_vol, charger->rest_vol);
		pre_rest_vol = charger->rest_vol;
		axp_write(charger->master,AXP20_DATA_BUFFER1,charger->rest_vol | 0x80);
		power_supply_changed(&charger->batt);
	}
	/* reschedule for the next time */
	schedule_delayed_work(&charger->work, charger->interval);
}

static void axp_usb(struct work_struct *work)
{
	int var;
	uint8_t tmp,val;
	struct axp_charger *charger;
	
	charger = axp_charger;
	if(axp_debug)
	{
		printk("[axp_usb]axp_usbcurflag = %d\n",axp_usbcurflag);
	}
	if(axp_usbcurflag&&vbus_curr_limit_debug){
		if(axp_debug)
		{
			printk("set usbcur_pc %d mA\n",pmu_usbcurnew);
		}
		if(pmu_usbcurnew){
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			var = pmu_usbcurnew * 1000;
			if(var >= 900000)
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
			else if ((var >= 500000)&& (var < 900000)){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			}
			else if ((var >= 100000)&& (var < 500000)){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
			}
			else{
				printk("set usb limit current error,%d mA\n",pmu_usbcurnew);	
			} 				
		}
		else
			axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);			
	}else {
		if(axp_debug)
		  {
			  printk("set usbcur %d mA\n",pmu_usbcur);
		  }
		if((pmu_usbcur) && (pmu_usbcur_limit)){
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			var = pmu_usbcur * 1000;
			if(var >= 900000)
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
			else if ((var >= 500000)&& (var < 900000)){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			}
			else if ((var >= 100000)&& (var < 500000)){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
			}
			else
				printk("set usb limit current error,%d mA\n",pmu_usbcur);	
		}
		else
			axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
	}
		
	if(axp_usbvolflag){
		if(axp_debug)
		{
			printk("set usbvol_pc %d mV\n",pmu_usbvolnew);
		}
		if(pmu_usbvolnew){
		    axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
		  	var = pmu_usbvolnew * 1000;
		  	if(var >= 4000000 && var <=4700000){
		    	tmp = (var - 4000000)/100000;
		    	axp_read(charger->master, AXP20_CHARGE_VBUS,&val);
		    	val &= 0xC7;
		    	val |= tmp << 3;
		    	axp_write(charger->master, AXP20_CHARGE_VBUS,val);
		  	}
		  	else
		  		printk("set usb limit voltage error,%d mV\n",pmu_usbvolnew);	
		}
		else
		    axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
	}else {
		if(axp_debug)
		{
			printk("set usbvol %d mV\n",pmu_usbvol);
		}
		if((pmu_usbvol) && (pmu_usbvol_limit)){
		    axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
		  	var = pmu_usbvol * 1000;
		  	if(var >= 4000000 && var <=4700000){
		    	tmp = (var - 4000000)/100000;
		    	axp_read(charger->master, AXP20_CHARGE_VBUS,&val);
		    	val &= 0xC7;
		    	val |= tmp << 3;
		    	axp_write(charger->master, AXP20_CHARGE_VBUS,val);
		  	}
		  	else
		  		printk("set usb limit voltage error,%d mV\n",pmu_usbvol);	
		}
		else
		    axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
	}
}

static int axp_battery_probe(struct platform_device *pdev)
{
	struct axp_charger *charger;
	struct axp_supply_init_data *pdata = pdev->dev.platform_data;
	int ret,var;
	uint8_t val1,val2,tmp,val;
	uint8_t ocv_cap[31],v[2];
	int Cur_CoulombCounter,rdc,saved_cap;
	script_item_u   item_val;
	script_item_value_type_e  item_type;

	powerkeydev = input_allocate_device();
	if (!powerkeydev) {
		kfree(powerkeydev);
		return -ENODEV;
	}
	use_cou = 1;

	powerkeydev->name = pdev->name;
	powerkeydev->phys = "m1kbd/input2";
	powerkeydev->id.bustype = BUS_HOST;
	powerkeydev->id.vendor = 0x0001;
	powerkeydev->id.product = 0x0001;
	powerkeydev->id.version = 0x0100;
	powerkeydev->open = NULL;
	powerkeydev->close = NULL;
	powerkeydev->dev.parent = &pdev->dev;

	set_bit(EV_KEY, powerkeydev->evbit);
	set_bit(EV_REL, powerkeydev->evbit);
	//set_bit(EV_REP, powerkeydev->evbit);
	set_bit(KEY_POWER, powerkeydev->keybit);

	ret = input_register_device(powerkeydev);
	if(ret) {
		printk("Unable to Register the power key\n");
	}

	if (pdata == NULL)
		return -EINVAL;

	if (pdata->chgcur > 1800000 ||
		pdata->chgvol < 4100000 ||
		pdata->chgvol > 4360000){
		printk("charger milliamp is too high or target voltage is over range\n");
		return -EINVAL;
	}

	if (pdata->chgpretime < 40 || pdata->chgpretime >70 ||
		pdata->chgcsttime < 360 || pdata->chgcsttime > 720){
		printk("prechaging time or constant current charging time is over range\n");
		return -EINVAL;
	}

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (charger == NULL)
		return -ENOMEM;

	charger->master = pdev->dev.parent;

	charger->chgcur			= pdata->chgcur;
	charger->chgvol			= pdata->chgvol;
	charger->chgend			= pdata->chgend;
	charger->sample_time	= pdata->sample_time;
	charger->chgen			= pdata->chgen;
	charger->chgpretime		= pdata->chgpretime;
	charger->chgcsttime		= pdata->chgcsttime;
	charger->battery_info	= pdata->battery_info;
	charger->disvbat		= 0;
	charger->disibat		= 0;
	axp_set_startup_sequence(charger);
	ret = axp_battery_first_init(charger);
	if (ret)
		goto err_charger_init;

	charger->nb.notifier_call = axp_battery_event;
	ret = axp_register_notifier(charger->master, &charger->nb, AXP20_NOTIFIER_ON);
	if (ret)
		goto err_notifier;

	axp_battery_setup_psy(charger);
	ret = power_supply_register(&pdev->dev, &charger->batt);
	if (ret)
		goto err_ps_register;

	axp_read(charger->master,AXP20_CHARGE_STATUS,&val);
	if(!((val >> 1) & 0x01)){
		ret = power_supply_register(&pdev->dev, &charger->ac);
		if (ret){
			power_supply_unregister(&charger->batt);
			goto err_ps_register;
		}
	}
	
	ret = power_supply_register(&pdev->dev, &charger->usb);
	if (ret){
		power_supply_unregister(&charger->ac);
		power_supply_unregister(&charger->batt);
		goto err_ps_register;
	}

	ret = axp_charger_create_attrs(&charger->batt);
	if(ret){
		return ret;
	}

	platform_set_drvdata(pdev, charger);

	/* initial restvol*/

	/* usb current and voltage limit */
	
	if((pmu_usbvol) && (pmu_usbvol_limit)) {
		axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
		var = pmu_usbvol * 1000;
		if((var >= 4000000) && (var <=4700000)) {
			tmp = (var - 4000000)/100000;
			axp_read(charger->master, AXP20_CHARGE_VBUS,&val);
			val &= 0xC7;
			val |= tmp << 3;
			axp_write(charger->master, AXP20_CHARGE_VBUS,val);
		} else {
			printk("set usb limit voltage error,%d mV\n",pmu_usbvol);
		}
	} else {
		axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
	}

	if((pmu_usbcur) && (pmu_usbcur_limit)) {
		axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
		var = pmu_usbcur * 1000;
		if(var >= 900000) {
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
		} else if ((var >= 500000) && (var < 900000)) {
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
			axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
		} else if ((var >= 100000) && (var < 500000)) {
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
		} else {
			printk("set usb limit current error,%d mV\n",pmu_usbcur);	
		}
  	} else {
		axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
	}
	
	/* set lowe power warning/shutdown voltage*/
	item_type = script_get_item("pmu_para", "pmu_suspendpwroff_vol", &item_val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != item_type) {
		printk("[AXP]axp driver uning configuration failed(%d)\n", __LINE__);
		pmu_suspendpwroff_vol = 3500;
		printk("[AXP]pmu_suspendpwroff_vol = %d\n",pmu_suspendpwroff_vol);
	}else
	{
        pmu_suspendpwroff_vol = item_val.val;
    }
	pmu_suspendpwroff_vol = pmu_suspendpwroff_vol * 1000;

	if(pmu_suspendpwroff_vol >= 2867200 && pmu_suspendpwroff_vol <= 4200000) {
		val = (pmu_suspendpwroff_vol - 2867200) / 5600;	
	}

	/* 3.5552V--%5 close*/
	axp_write(charger->master, AXP20_APS_WARNING1,val); //�͵���ʾ
	axp_write(charger->master, AXP20_APS_WARNING2,(val - 0x23));//�ػ���ʾ,�ȵ͵���ʾ��196mv
	ocv_cap[0]  = pmu_bat_para1;
	ocv_cap[1]  = 0xC1;
	ocv_cap[2]  = pmu_bat_para2;
	ocv_cap[3]  = 0xC2;
	ocv_cap[4]  = pmu_bat_para3;
	ocv_cap[5]  = 0xC3;
	ocv_cap[6]  = pmu_bat_para4;
	ocv_cap[7]  = 0xC4;
	ocv_cap[8]  = pmu_bat_para5;
	ocv_cap[9]  = 0xC5;
	ocv_cap[10] = pmu_bat_para6;
	ocv_cap[11] = 0xC6;
	ocv_cap[12] = pmu_bat_para7;
	ocv_cap[13] = 0xC7;
	ocv_cap[14] = pmu_bat_para8;
	ocv_cap[15] = 0xC8;
	ocv_cap[16] = pmu_bat_para9;
	ocv_cap[17] = 0xC9;
	ocv_cap[18] = pmu_bat_para10;
	ocv_cap[19] = 0xCA;
	ocv_cap[20] = pmu_bat_para11;
	ocv_cap[21] = 0xCB;
	ocv_cap[22] = pmu_bat_para12;
	ocv_cap[23] = 0xCC;
	ocv_cap[24] = pmu_bat_para13;
	ocv_cap[25] = 0xCD;
	ocv_cap[26] = pmu_bat_para14;
	ocv_cap[27] = 0xCE;
	ocv_cap[28] = pmu_bat_para15;
	ocv_cap[29] = 0xCF;
	ocv_cap[30] = pmu_bat_para16;
	axp_writes(charger->master, 0xC0,31,ocv_cap);

	/* open/close set */
	printk("pmu_pekoff_time = %d\n",pmu_pekoff_time);
	printk("pmu_pekoff_en = %d\n",pmu_pekoff_en);
	printk("pmu_peklong_time = %d\n",pmu_peklong_time);
	printk("pmu_pekon_time = %d\n",pmu_pekon_time);
	printk("pmu_pwrok_time = %d\n",pmu_pwrok_time);
	printk("pmu_pwrnoe_time = %d\n",pmu_pwrnoe_time);
	printk("pmu_intotp_en = %d\n",pmu_intotp_en);

	/* n_oe delay time set */
	if (pmu_pwrnoe_time < 1000)
		pmu_pwrnoe_time = 128;
	if (pmu_pwrnoe_time > 3000)
		pmu_pwrnoe_time = 3000;
	axp_read(charger->master,POWER20_OFF_CTL,&val);
	val &= 0xfc;
	val |= ((pmu_pwrnoe_time) / 1000);
	axp_write(charger->master,POWER20_OFF_CTL,val);
	DBG_PSY_MSG("POWER20_OFF_CTL:%d-->0x%x\n",__LINE__,val);

	/* pek open time set */
	axp_read(charger->master,POWER20_PEK_SET,&val);
	if (pmu_pekon_time < 1000) {
		val &= 0x3f;
	} else if(pmu_pekon_time < 2000) {
		val &= 0x3f;
		val |= 0x80;
	} else if(pmu_pekon_time < 3000) {
		val &= 0x3f;
		val |= 0xc0;
	} else {
		val &= 0x3f;
		val |= 0x40;
	}
	axp_write(charger->master,POWER20_PEK_SET,val);
	printk("POWER20_PEK_SET:%d-->0x%x\n",__LINE__,val);

	/* pek long time set*/
	if(pmu_peklong_time < 1000)
		pmu_peklong_time = 1000;
	if(pmu_peklong_time > 2500)
		pmu_peklong_time = 2500;
	axp_read(charger->master,POWER20_PEK_SET,&val);
	val &= 0xcf;
	val |= (((pmu_peklong_time - 1000) / 500) << 4);
	axp_write(charger->master,POWER20_PEK_SET,val);
	printk("POWER20_PEK_SET:%d-->0x%x\n",__LINE__,val);

	/* pek en set*/
	if(pmu_pekoff_en)
		pmu_pekoff_en = 1;
	axp_read(charger->master,POWER20_PEK_SET,&val);
	val &= 0xf7;
	val |= (pmu_pekoff_en << 3);
	axp_write(charger->master,POWER20_PEK_SET,val);
	printk("POWER20_PEK_SET:%d-->0x%x\n",__LINE__,val);

	/* pek delay set */
	if(pmu_pwrok_time <= 8)
		pmu_pwrok_time = 0;
	else
		pmu_pwrok_time = 1;
	axp_read(charger->master,POWER20_PEK_SET,&val);
	val &= 0xfb;
	val |= pmu_pwrok_time << 2;
	axp_write(charger->master,POWER20_PEK_SET,val);
	printk("POWER20_PEK_SET:%d-->0x%x\n",__LINE__,val);

	/* pek off time set */
	if(pmu_pekoff_time < 4000)
		pmu_pekoff_time = 4000;
	if(pmu_pekoff_time > 10000)
		pmu_pekoff_time =10000;
	pmu_pekoff_time = (pmu_pekoff_time - 4000) / 2000 ;
	axp_read(charger->master,POWER20_PEK_SET,&val);
	val &= 0xfc;
	val |= pmu_pekoff_time ;
	axp_write(charger->master,POWER20_PEK_SET,val);
	printk("POWER20_PEK_SET:%d-->0x%x\n",__LINE__,val);

	/* enable overtemperture off */
	if(pmu_intotp_en)
		pmu_intotp_en = 1;
	axp_read(charger->master,POWER20_HOTOVER_CTL,&val);
	val &= 0xfb;
	val |= pmu_intotp_en << 2;
	axp_write(charger->master,POWER20_HOTOVER_CTL,val);
	printk("POWER20_HOTOVER_CTL:%d-->0x%x\n",__LINE__,val);
	
	/* disable */
	axp_set_bits(charger->master,AXP20_CAP,0x80); //disbale ocv calculation
	axp_clr_bits(charger->master,0xBA,0x80); //disable rdc calculation
	axp_reads(charger->master,0xbA,2,v);
	rdc = (((v[0] & 0x1F) << 8) | v[1]) * 10742 / 10000;
	axp_read(charger->master,AXP20_DATA_BUFFER0,&val1);
	if((((val1 >> 7) & 0x1) == 0)||(rdc > pmu_battery_rdc * 3)) {
		rdc = (pmu_battery_rdc * 10000 + 5371) / 10742;
		axp_write(charger->master,0xBB,rdc & 0x00FF);
		axp_update(charger->master, 0xBA, (rdc >> 8), 0x1F);
	}
	axp_clr_bits(charger->master,AXP20_CAP,0x80);//enable ocv calculation

	axp_set_bits(charger->master,0x8F,0x88);
	axp_clr_bits(charger->master,0x81,0x04);

	axp_charger_update_state(charger);

	axp_read(charger->master, AXP20_CAP,&val2);
	charger->ocv_rest_vol =	((val2 & 0x7F)==127)? 100:(val2	& 0x7F);

	axp_read(charger->master,AXP20_DATA_BUFFER1,&val1);
	DBG_PSY_MSG("last_rest_vol = %d, now_rest_vol	= %d\n",(val1 &	0x7F),charger->ocv_rest_vol);

	bat_cap =	Get_Buffer_Cou(charger);

	axp_read(charger->master,AXP20_DATA_BUFFER0,&val);
	Get_Rest_Cap(charger,&saved_cap);
	if((val >> 6)& 0x01){
		Cur_CoulombCounter = (Get_Bat_Coulomb_Count(charger));
		DBG_PSY_MSG("Cur_CoulombCounter	= %d\n",Cur_CoulombCounter);
		charger->rest_vol =	100* Cur_CoulombCounter	/ bat_cap +	saved_cap;
		if(Cur_CoulombCounter < 0){
			charger->rest_vol = charger->rest_vol - 1;
		}
		if(	charger->rest_vol >	100	){
			charger->rest_vol =	100;
			if(Cur_CoulombCounter < bat_cap){
				saved_cap = 100 - 100 * (Cur_CoulombCounter) / bat_cap;
				Set_Rest_Cap(charger,saved_cap);
			}
			else{
				bat_cap = Cur_CoulombCounter * 100 /(100 - saved_cap);
				Buffer_Cou_Set(charger,bat_cap);
			}
		} else if(charger->rest_vol < 0){
			charger->rest_vol =	0;
			Set_Rest_Cap(charger,0);
			Cou_Count_Clear(charger);
		}
		
		if((charger->vbat > 4100) && (!charger->is_on) && (charger->ext_valid) && (charger->charge_on)&&(!charger->disibat)){
			charger->rest_vol = 100;
			saved_cap = 100 - 100 * (Cur_CoulombCounter) / bat_cap;
			Set_Rest_Cap(charger,saved_cap);
  		}
  		/* �Ľ��������� */
  		if((saved_cap <= BATCAPCORRATE ) && (charger->rest_vol <= (BATCAPCORRATE + 1)) && (!(charger->ext_valid))){
  			charger->rest_vol = BATCAPCORRATE + 1;
  			saved_cap = BATCAPCORRATE + 1;
			Set_Rest_Cap(charger,saved_cap);
  		}
	}
	else
		charger->rest_vol =	(val2 &	0x7F);

	if((charger->bat_det ==	0) || (charger->rest_vol ==	127)){
		charger->rest_vol =	100;
	}
	
	DBG_PSY_MSG("charger->rest_vol = %d\n",charger->rest_vol);
	charger->interval	= msecs_to_jiffies(10 *	1000);
	INIT_DELAYED_WORK(&charger->work,	axp_charging_monitor);
	schedule_delayed_work(&charger->work,	charger->interval);
	
	/* set usb cur-vol limit*/
	INIT_DELAYED_WORK(&usbwork, axp_usb);
    pmu_usb_delayed_work_inited = 1;
	if(charger->usb_valid){
		schedule_delayed_work(&usbwork, msecs_to_jiffies(7 * 1000));
	}

	item_type = script_get_item("pmu_para", "pmu_earlysuspend_chgcur", &item_val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != item_type) {
		printk("axp driver uning configuration failed(%d)\n", __LINE__);
		pmu_earlysuspend_chgcur = pmu_suspend_chgcur / 1000;
		printk("pmu_earlysuspend_chgcur = %d\n",pmu_earlysuspend_chgcur);
	}else
	{
        pmu_earlysuspend_chgcur=item_val.val;
    }
	pmu_earlysuspend_chgcur = pmu_earlysuspend_chgcur * 1000;
  
	item_type = script_get_item("pmu_para", "pmu_batdeten", &item_val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != item_type) {
		printk("axp driver uning configuration failed(%d)\n", __LINE__);
		pmu_batdeten = 1;
		printk("pmu_batdeten = %d\n",pmu_batdeten);
	}else
	{
        pmu_batdeten = item_val.val;
    }
	if(!pmu_batdeten)
		axp_clr_bits(charger->master,0x32,0x40);
	else
		axp_set_bits(charger->master,0x32,0x40);
  	
	/*axp usb-pc limite*/
	item_type = script_get_item("pmu_para", "pmu_usbvol_pc", &item_val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != item_type) {
		printk("axp driver uning configuration failed-pmu_usbvol_pc\n");
		pmu_usbvolnew = 4000;
		printk("pmu_usbvolnew = %d\n",pmu_usbvolnew);
	} else {
		pmu_usbvolnew = item_val.val;
		if(pmu_usbvolnew){
			axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
			var = pmu_usbvolnew * 1000;
			if(var >= 4000000 && var <=4700000){
				tmp = (var - 4000000)/100000;
				axp_read(charger->master, AXP20_CHARGE_VBUS,&val);
				val &= 0xC7;
				val |= tmp << 3;
				axp_write(charger->master, AXP20_CHARGE_VBUS,val);
			}
		} else {
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x40);
		}
		
	}
  
	item_type = script_get_item("pmu_para", "pmu_usbcur_pc", &item_val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != item_type) {
		printk("axp driver uning configuration failed-pmu_usbcurnew\n");
		pmu_usbcurnew = 500;
		printk("pmu_usbcurnew = %d\n",pmu_usbcurnew);
	} else {
		pmu_usbcurnew = item_val.val;
		if(pmu_usbcurnew){
			axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			var = pmu_usbcurnew * 1000;
			if(var == 900000)
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
			else if (var == 500000){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
			}
			else if (var == 100000){
				axp_clr_bits(charger->master, AXP20_CHARGE_VBUS, 0x01);
				axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x02);
			}
		} else {
    		axp_set_bits(charger->master, AXP20_CHARGE_VBUS, 0x03);
		}
		
	}

	
	
	axp_charger = charger;
#ifdef CONFIG_HAS_EARLYSUSPEND
	axp_early_suspend.suspend = axp_earlysuspend;
	axp_early_suspend.resume = axp_lateresume;
	axp_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	register_early_suspend(&axp_early_suspend);
#endif

	/* ���Խӿ�ע�� */
	class_register(&axppower_class);

	return ret;

err_ps_register:
	axp_unregister_notifier(charger->master, &charger->nb, AXP20_NOTIFIER_ON);

err_notifier:
	cancel_delayed_work_sync(&charger->work);

err_charger_init:
	kfree(charger);
	input_unregister_device(powerkeydev);
	kfree(powerkeydev);

	return ret;
}

static int axp_battery_remove(struct platform_device *dev)
{
	struct axp_charger *charger = platform_get_drvdata(dev);

	if(main_task){
		kthread_stop(main_task);
		main_task = NULL;
	}

	axp_unregister_notifier(charger->master, &charger->nb, AXP20_NOTIFIER_ON);
	cancel_delayed_work_sync(&charger->work);
	power_supply_unregister(&charger->usb);
	power_supply_unregister(&charger->ac);
	power_supply_unregister(&charger->batt);
	kfree(axp_charger);
	kfree(charger);
	input_unregister_device(powerkeydev);
	kfree(powerkeydev);

	return 0;
}


static int axp20_suspend(struct platform_device *dev, pm_message_t state)
{
	uint8_t irq_w[9];
	uint8_t tmp;
	volatile int val;

	struct axp_charger *charger = platform_get_drvdata(dev);
	if (test_bit(KEY_POWER, powerkeydev->key)){
		return -EBUSY;
	}
	cancel_delayed_work_sync(&charger->work);

	/*clear all irqs events*/
	irq_w[0] = 0xff;
	irq_w[1] = POWER20_INTSTS2;
	irq_w[2] = 0xff;
	irq_w[3] = POWER20_INTSTS3;
	irq_w[4] = 0xff;
	irq_w[5] = POWER20_INTSTS4;
	irq_w[6] = 0xff;
	irq_w[7] = POWER20_INTSTS5;
	irq_w[8] = 0x80;
	axp_writes(charger->master, POWER20_INTSTS1, 9, irq_w);

	/* close all irqs*/
	axp_unregister_notifier(charger->master, &charger->nb, AXP20_NOTIFIER_ON);

#if defined (CONFIG_AXP_CHGCHANGE)
	if(pmu_suspend_chgcur == 0)
		axp_clr_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
	else
		axp_set_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);

	if(pmu_suspend_chgcur >= 300000 && pmu_suspend_chgcur <= 1800000){
		tmp = (pmu_suspend_chgcur -200001)/100000;
		charger->chgcur = tmp *100000 + 300000;
		axp_update(charger->master, AXP20_CHARGE_CONTROL1, tmp,0x0F);
	}
	printk("pmu_suspend_chgcur = %d\n", pmu_suspend_chgcur);
#endif
	/* timer */
	axp_write(charger->master, 0x8A, 0x80);
	axp_write(charger->master, 0x8A, 0x7F);
	/*set suspendpoweroff level*/
	DBG_APP_MSG("[suspend]pmu_suspendpwroff_vol = %d\n",pmu_suspendpwroff_vol);
	if(pmu_suspendpwroff_vol >= 2867200 && pmu_suspendpwroff_vol <= 4200000) {
		val = (pmu_suspendpwroff_vol - 2867200) / 5600;	
	}
	DBG_APP_MSG("[suspend]pmu_suspendpwroff_vol val = 0x%x\n",val);
	axp_write(axp_charger->master, AXP20_APS_WARNING1,val);
	axp_write(axp_charger->master, AXP20_APS_WARNING1,(val-0x23));
	return 0;
}

static int axp20_resume(struct platform_device *dev)
{
	struct axp_charger *charger = platform_get_drvdata(dev);
	int	pre_rest_vol;
	uint8_t	val,tmp;
	int	rt_rest_vol;
	int	Cur_CoulombCounter = 0;
	uint8_t	v[2];
	int	saved_cap;
	int	flag_notfristin;
	int	Cou_Correction_Flag;
	int bat_valtage;
	//*****У�������������ƫ���µ�����ʾ��׼*******//
	axp_read(charger->master, POWER20_INTSTS2, v);
	if(v[0] &= 0x04){
		axp_reads(charger->master, 0xbc, 2,v);
		bat_valtage = ((int)((v[0] << 4) | (v[1] & 0x0F))) * 1100 / 1000;
		if(bat_valtage > 4080){
			axp_read(charger->master, POWER20_DATA_BUFFER1, v);
			Cou_Correction_Flag	= (v[0]>> 5)&0x1;
			if(Cou_Correction_Flag){
				printk("[AXP20-MFD] ----------charger finish need to be corrected-----------\n");
				printk("[AXP20-MFD] ----------correct the coulunb counter-----------\n");
				axp_read(charger->master, AXP20_DATA_BUFFER5, v);
				Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
				bat_cap	= ABS(Cur_CoulombCounter) / (100 - v[0]) * 100;
				Buffer_Cou_Set(charger,bat_cap);
				Cou_Correction_Flag	= 0;
				axp_clr_bits(charger->master,POWER20_DATA_BUFFER1,0x20);
				saved_cap =	100;
				Set_Rest_Cap(charger,saved_cap);
				Cou_Count_Clear(charger);
			}
			else{
				printk("[AXP20-MFD] ----------charger finish need to be corrected-----------\n");
				saved_cap =	100;
				Set_Rest_Cap(charger,saved_cap);
				Cou_Count_Clear(charger);
			}
		}
	}
	//*******************************end**************************************//
	axp_register_notifier(charger->master, &charger->nb, AXP20_NOTIFIER_ON );

	axp_charger_update_state(charger);
	axp_charger_update(charger);
	pre_rest_vol = charger->rest_vol;
	axp_read(charger->master, AXP20_CAP,&val);
	charger->rest_vol =	(int) (val & 0x7F);
	rt_rest_vol	= charger->rest_vol;
	if((charger->bat_det ==	0) || (charger->rest_vol ==	127)){
		charger->rest_vol =	100;
	}
	axp_reads(charger->master,AXP20_IC_TYPE,2,v);
	flag_notfristin	= (v[1]	>> 6) &	0x1;
	Cou_Correction_Flag	= (v[1]	>> 5) &	0x1;
	Get_Rest_Cap(charger, &saved_cap);

	if(flag_notfristin){
		Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
		DBG_PSY_MSG("Cur_CoulombCounter	= %d\n", Cur_CoulombCounter);

		charger->rest_vol =	100	* (Cur_CoulombCounter) / bat_cap + saved_cap;
		DBG_PSY_MSG("Resume:before modify:charger->rest_vol = %d\n",charger->rest_vol);
		if(Cur_CoulombCounter < 0){
			charger->rest_vol = charger->rest_vol - 1;
		}
/******************************�����������ʼPre_rest_cap����ƫ�󣬵��³��ʱ�ÿ��ؼƼ���ĵ�������100%**************************/
		if(charger->rest_vol > 100) {
			charger->rest_vol =	100;
		} else if(charger->rest_vol < 0) {
			charger->rest_vol =	0;
		}
/************************************************************************************************************************************/
		if((charger->rest_vol <= (BATCAPCORRATE))&& (rt_rest_vol <= (BATCAPCORRATE + 1)) &&(!charger->ext_valid)&&(!flag_cou)) {
			Cou_Correction_Flag	= 1;
			flag_cou = 1;
			Set_Rest_Cap(charger,BATCAPCORRATE);
			axp_write(charger->master, AXP20_DATA_BUFFER5, rt_rest_vol);
			Cou_Count_Clear(charger);
			axp_set_bits(charger->master,AXP20_DATA_BUFFER0,0x20);
			printk("\n Resume=Capacity	Calibration	Start============ \n");
			printk("\n Resume=	  Rest	Capacity  =	 %d	 ============ \n",rt_rest_vol);
		}

		if(charger->rest_vol > BATCAPCORRATE){
			flag_cou = 0;
		}

/******************************�����������ʼPre_rest_cap����ƫС�����³������ÿ��ؼƼ���ĵ�������100%****************/
		if((charger->vbat > 4100) && (charger->rest_vol	< 100) && (charger->bat_current_direction == 0x0) && (charger->	ext_valid == 1)&&(charger->charge_on)&&(!charger->disibat)) {
			DBG_PSY_MSG("Resume:charger->rest_vol = %d -> 100\n",charger->rest_vol);
			charger->rest_vol =	100;
			Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
			saved_cap =	100	- 100 *(Cur_CoulombCounter)	/ bat_cap;
			Set_Rest_Cap(charger,saved_cap);
		}

/*****************************************������������ڳ�磬��ʹ����100%��Ҳ��Ϊ99%**********************************/
		if((charger->bat_current_direction == 1) &&	(charger->rest_vol >= 100)) {
			printk("Resume: charging->charger->rest_vol = %d ->99\n",charger->rest_vol);
			charger->rest_vol =	99;
			Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
			saved_cap =	99 - 100 * (Cur_CoulombCounter)	/ bat_cap;
			Set_Rest_Cap(charger,saved_cap);
		}

		if(Cou_Correction_Flag && (charger->bat_current_direction  == 0) &&	(charger->rest_vol == 100)) {
			axp_read(charger->master, AXP20_DATA_BUFFER5, &val);
			Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
			bat_cap	= ABS(Cur_CoulombCounter) /	( 100 -	val) * 100;
			Buffer_Cou_Set(charger,bat_cap);
			Cou_Correction_Flag	= 0;
			axp_clr_bits(charger->master,AXP20_DATA_BUFFER0,0x20);
			printk("\n Resume=Capacity calibration finished============	\n");
			printk("\n Resume=	  Battery capacity =  %d   ============	\n",bat_cap);
			saved_cap =	100	- 100 *	(Cur_CoulombCounter) / bat_cap;
			Set_Rest_Cap(charger,saved_cap);
		}
	} else {
		if(charger->bat_current_direction && (charger->rest_vol	< pre_rest_vol)) {
			charger->rest_vol =	pre_rest_vol;
		}

		DBG_PSY_MSG("val = 0x%x,pre_rest_vol = %d,rest_vol = %d\n",val,pre_rest_vol,charger->rest_vol);

		/* full	*/
		if((charger->vbat) >= 4100 && !charger->bat_current_direction && charger->ext_valid &&(charger->charge_on)&&(!charger->disibat)) {
			charger->rest_vol =	100;
		}

		/* charging*/
		if(charger->bat_current_direction && charger->rest_vol == 100){
			charger->rest_vol =	99;
		}

		DBG_PSY_MSG("\n\n\n@j == BUFFER_LONG - 1 [Pre_rest_cap = %d]\n\n\n\n",charger->rest_vol);
		Set_Rest_Cap(charger,charger->rest_vol);
		Cou_Count_Clear(charger);
		axp_set_bits(charger->master, AXP20_DATA_BUFFER0, 0x40);
		flag_notfristin	= 1;
	}

	axp_read(charger->master, 0x8A,	&val);
	if((val	>> 7) && (pre_rest_vol > charger->rest_vol)	&& !charger->bat_current_direction){
		/*��ʱ��ŵ�����ȡ�����������У��*/
		Cou_Correction_Flag	= 0;
		axp_clr_bits(charger->master,AXP20_DATA_BUFFER0,0x20);
		if(pre_rest_vol	> rt_rest_vol){
			DBG_PSY_MSG("\n	correct	rest vol!!!\n");
			charger-> rest_vol = rt_rest_vol;
            DBG_PSY_MSG("rt_rest_vol=%d\n",rt_rest_vol);
			Cur_CoulombCounter = Get_Bat_Coulomb_Count(charger);
             DBG_PSY_MSG("Cur_CoulombCounter=%d\n",Cur_CoulombCounter);
			saved_cap =	rt_rest_vol	- 100* (Cur_CoulombCounter)	/ bat_cap;
            DBG_PSY_MSG("saved_cap=%d\n",saved_cap);
			if((saved_cap >= -127) && (saved_cap <= 127)){
				Set_Rest_Cap(charger,saved_cap);
			} else {
				saved_cap =	rt_rest_vol;
				Set_Rest_Cap(charger,saved_cap);
				Cou_Count_Clear(charger);
			}
		}
	}
	
	/*�͵�ػ��ӿ�*/
	axp_read(charger->master, 0x0e,&val);
	printk("[AXP]=======================val = 0x%x\n",val);
	if(val){
		axp_close(charger);
		axp_set_bits(charger->master,0x4b,0x03);
	}
	axp_write(charger->master, 0x0e,0x00);
	
	charger->ocv_rest_vol =	rt_rest_vol;
	axp_reads(charger->master,0xbc,2,v);
	charger->ocv = ((v[0] << 4)	+ (v[1]	& 0x0f)) * 11 /10 ;

if(axp_debug){
	DBG_PSY_MSG("charger->ic_temp =	%d\n",charger->ic_temp);
	DBG_PSY_MSG("charger->vbat = %d\n",charger->vbat);
	DBG_PSY_MSG("charger->ibat = %d\n",charger->ibat);
	DBG_PSY_MSG("charger->disvbat =	%d\n",charger->disvbat);
	DBG_PSY_MSG("charger->disibat =	%d\n",charger->disibat);
	DBG_PSY_MSG("charger->vusb = %d\n",charger->vusb);
	DBG_PSY_MSG("charger->iusb = %d\n",charger->iusb);
	DBG_PSY_MSG("charger->vac =	%d\n",charger->vac);
	DBG_PSY_MSG("charger->iac =	%d\n",charger->iac);
	DBG_PSY_MSG("charger->ocv =	%d\n",charger->ocv);
	DBG_PSY_MSG("rt_rest_vol = %d\n",rt_rest_vol);
	DBG_PSY_MSG("bat_cap = %d\n",bat_cap);
	DBG_PSY_MSG("charger->ocv_rest_vol = %d\n",charger->ocv_rest_vol);
	DBG_PSY_MSG("charger->rest_vol = %d\n",charger->rest_vol);
	axp_reads(charger->master,0xba,2,v);
	tmp	= (((v[0] &	0x1F) << 8)	| v[1])	* 10742	/ 10000;
	DBG_PSY_MSG("rdc = %d\n",tmp);
	axp_read(charger->master,0x8,v);
	DBG_PSY_MSG("base restvol =	%d\n",(int)	v);
	DBG_PSY_MSG("charger->is_on	= %d\n",charger->is_on);
	DBG_PSY_MSG("charger->ext_valid	= %d\n",charger->ext_valid);
}

	/* if battery volume changed, inform uevent	*/
	if(charger->rest_vol - pre_rest_vol){
		DBG_PSY_MSG("battery vol change: %d->%d	\n", pre_rest_vol, charger->rest_vol);
		pre_rest_vol = charger->rest_vol;
		axp_write(charger->master,AXP20_DATA_BUFFER1,charger->rest_vol | 0x80);
		
	}
    power_supply_changed(&charger->batt);
#if	defined	(CONFIG_AXP_CHGCHANGE)
	if(pmu_resume_chgcur ==	0)
		axp_clr_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
	else
		axp_set_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);

	if(pmu_resume_chgcur >=	300000 && pmu_resume_chgcur	<= 1800000){
		tmp	= (pmu_resume_chgcur -200001)/100000;
		charger->chgcur	= tmp *100000 +	300000;
		axp_update(charger->master,	AXP20_CHARGE_CONTROL1, tmp,0x0F);
	}
	DBG_APP_MSG("pmu_resume_chgcur = %d\n",pmu_resume_chgcur);
#endif
	charger->disvbat = 0;
	charger->disibat = 0;

	schedule_delayed_work(&charger->work, charger->interval);

	return 0;
}

static void axp20_shutdown(struct platform_device *dev)
{
	uint8_t tmp;
	struct axp_charger *charger = platform_get_drvdata(dev);

	cancel_delayed_work_sync(&charger->work);
	axp_clr_bits(charger->master,AXP20_CAP,0x80);

#if defined (CONFIG_AXP_CHGCHANGE)
	if(pmu_shutdown_chgcur == 0)
		axp_clr_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);
	else
		axp_set_bits(charger->master,AXP20_CHARGE_CONTROL1,0x80);

		printk("pmu_shutdown_chgcur = %d\n", pmu_shutdown_chgcur);

	if(pmu_shutdown_chgcur >= 300000 && pmu_shutdown_chgcur <= 1800000){
		tmp = (pmu_shutdown_chgcur -200001)/100000;
		charger->chgcur = tmp *100000 + 300000;
		axp_update(charger->master, AXP20_CHARGE_CONTROL1, tmp, 0x0F);
	}
#endif
}

static struct platform_driver axp_battery_driver = {
	.driver = {
		.name = "axp20-supplyer",
		.owner  = THIS_MODULE,
	},
	.probe = axp_battery_probe,
	.remove = axp_battery_remove,
	.suspend = axp20_suspend,
	.resume = axp20_resume,
	.shutdown = axp20_shutdown,
};

static int axp_battery_init(void)
{
	return platform_driver_register(&axp_battery_driver);
}

static void axp_battery_exit(void)
{
	platform_driver_unregister(&axp_battery_driver);
}

module_init(axp_battery_init);
module_exit(axp_battery_exit);

MODULE_DESCRIPTION("axp20 battery charger driver");
MODULE_AUTHOR("Donglu Zhang, Krosspower");
MODULE_LICENSE("GPL");
