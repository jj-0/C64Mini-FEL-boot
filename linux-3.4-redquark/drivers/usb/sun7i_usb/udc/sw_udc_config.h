/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc_config.h
*
* Author 		: javen
*
* Description 	:
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-3-3            1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_UDC_CONFIG_H__
#define  __SW_UDC_CONFIG_H__


#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>

#include  "../include/sw_usb_config.h"

#define  SW_UDC_DOUBLE_FIFO       /* ˫ FIFO          */
#define  SW_UDC_DMA               /* DMA ����         */
#define  SW_UDC_HS_TO_FS          /* ֧�ָ�����תȫ�� */


//---------------------------------------------------------------
//  ����
//---------------------------------------------------------------

/* sw udc ���Դ�ӡ */
#if	0
    #define DMSG_DBG_UDC     			DMSG_PRINT
#else
    #define DMSG_DBG_UDC(...)
#endif


#endif   //__SW_UDC_CONFIG_H__

