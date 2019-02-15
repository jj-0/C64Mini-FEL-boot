/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hci_sun4i.h
*
* Author 		: yangnaitian
*
* Description 	: Include file for AW1623 HCI Host Controller Driver
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    yangnaitian      2011-5-24            1.0          create this file
*
*************************************************************************************
*/

#ifndef __SW_HCI_SUN4I_H__
#define __SW_HCI_SUN4I_H__

#include <linux/delay.h>
#include <linux/types.h>

#include <linux/io.h>
#include <linux/irq.h>

#define  DMSG_PRINT(stuff...)		printk(stuff)
#define  DMSG_ERR(...)        		(DMSG_PRINT("WRN:L%d(%s):", __LINE__, __FILE__), DMSG_PRINT(__VA_ARGS__))

#if 0
    #define DMSG_DEBUG         		DMSG_PRINT
#else
    #define DMSG_DEBUG(...)
#endif

#if 1
    #define DMSG_INFO         		DMSG_PRINT
#else
    #define DMSG_INFO(...)
#endif

#if	1
    #define DMSG_PANIC        		DMSG_ERR
#else
    #define DMSG_PANIC(...)
#endif


//---------------------------------------------------------------
//  �� ����
//---------------------------------------------------------------
#define  USBC_Readb(reg)	                    (*(volatile unsigned char *)(reg))
#define  USBC_Readw(reg)	                    (*(volatile unsigned short *)(reg))
#define  USBC_Readl(reg)	                    (*(volatile unsigned long *)(reg))

#define  USBC_Writeb(value, reg)                (*(volatile unsigned char *)(reg) = (value))
#define  USBC_Writew(value, reg)	            (*(volatile unsigned short *)(reg) = (value))
#define  USBC_Writel(value, reg)	            (*(volatile unsigned long *)(reg) = (value))

#define  USBC_REG_test_bit_b(bp, reg)         	(USBC_Readb(reg) & (1 << (bp)))
#define  USBC_REG_test_bit_w(bp, reg)   	    (USBC_Readw(reg) & (1 << (bp)))
#define  USBC_REG_test_bit_l(bp, reg)   	    (USBC_Readl(reg) & (1 << (bp)))

#define  USBC_REG_set_bit_b(bp, reg) 			(USBC_Writeb((USBC_Readb(reg) | (1 << (bp))) , (reg)))
#define  USBC_REG_set_bit_w(bp, reg) 	 		(USBC_Writew((USBC_Readw(reg) | (1 << (bp))) , (reg)))
#define  USBC_REG_set_bit_l(bp, reg) 	 		(USBC_Writel((USBC_Readl(reg) | (1 << (bp))) , (reg)))

#define  USBC_REG_clear_bit_b(bp, reg)	 	 	(USBC_Writeb((USBC_Readb(reg) & (~ (1 << (bp)))) , (reg)))
#define  USBC_REG_clear_bit_w(bp, reg)	 	 	(USBC_Writew((USBC_Readw(reg) & (~ (1 << (bp)))) , (reg)))
#define  USBC_REG_clear_bit_l(bp, reg)	 	 	(USBC_Writel((USBC_Readl(reg) & (~ (1 << (bp)))) , (reg)))

//---------------------------------------------------------------
//
//---------------------------------------------------------------
#define SW_SRAM_BASE		0x01c00000
#define SW_SRAM_BASE_LEN	0x100
#define SW_GPIO_BASE		0x01c20800
#define SW_GPIO_BASE_LEN	0x100

//-----------------------------------------------------------------------
//   USB register
//-----------------------------------------------------------------------

#define SW_USB1_BASE				0x01c14000
#define SW_USB2_BASE				0x01c1c000

#define SW_USB_EHCI_BASE_OFFSET		0x00
#define SW_USB_OHCI_BASE_OFFSET		0x400
#define SW_USB_EHCI_LEN      		0x58
#define SW_USB_OHCI_LEN      		0x58

#define SW_USB_PMU_IRQ_ENABLE		0x800

/*
#define SW_USB_EHCI0_BASE     			(SW_USB1_BASE + SW_USB_EHCI_BASE_OFFSET)
#define SW_USB_EHCI0_LEN      			0x58

#define SW_USB_OHCI0_BASE     			(SW_USB1_BASE + SW_USB_OHCI_BASE_OFFSET)
#define SW_USB_OHCI0_LEN      			0x58

#define SW_USB_EHCI1_BASE     			(SW_USB2_BASE + SW_USB_EHCI_BASE_OFFSET)
#define SW_USB_EHCI1_LEN      			0x58

#define SW_USB_OHCI1_BASE     			(SW_USB2_BASE + SW_USB_OHCI_BASE_OFFSET)
#define SW_USB_OHCI1_LEN      			0x58
*/

//-----------------------------------------------------------------------
//   CCMU register
//-----------------------------------------------------------------------

#define SW_CCMU_BASE                		0x01c20000
#define SW_CCMU_BASE_LEN					0x100

#define SW_CCMU_REG_AHB_GATING_REG0     	0x60
#define SW_CCMU_REG_USB_CLK_REG   			0xCC

/* ABH Gating Reg0 */
#define SW_CCMU_BP_AHB_GATING_USBC2       	2
#define SW_CCMU_BP_AHB_GATING_USBC1       	1

/* usb clock reg */
#define SW_CCMU_BP_USB_CLK_GATING_USBPHY	8
#define SW_CCMU_BP_USB_CLK_GATING_OHCI1		7
#define SW_CCMU_BP_USB_CLK_GATING_OHCI0		6
#define SW_CCMU_BP_USB_CLK_48M_SEL			4
#define SW_CCMU_BP_USB_CLK_USBPHY2_RST		2
#define SW_CCMU_BP_USB_CLK_USBPHY1_RST		1
#define SW_CCMU_BP_USB_CLK_USBPHY0_RST		0

//-----------------------------------------------------------------------
//   interrupt register
//-----------------------------------------------------------------------
#define SW_INT_SRC_EHCI0            		39
#define SW_INT_SRC_OHCI0                    64
#define SW_INT_SRC_EHCI1            		40
#define SW_INT_SRC_OHCI1                    65

//-----------------------------------------------------------------------
//   SDRAM Control register
//-----------------------------------------------------------------------

//#define SW_HCI0_PASS_BY_BASE     0x01c14800
//#define SW_HCI0_PASS_BY_BASE_LEN 4

#define SW_SDRAM_BASE               		0x01c01000
#define SW_SDRAM_BASE_LEN					0x100

#define SW_SDRAM_REG_HPCR_USB1				(0x250 + ((1 << 2) * 4))
#define SW_SDRAM_REG_HPCR_USB2				(0x250 + ((1 << 2) * 5))

/* HPCR */
#define SW_SDRAM_BP_HPCR_READ_CNT_EN		31
#define SW_SDRAM_BP_HPCR_RWRITE_CNT_EN		30
#define SW_SDRAM_BP_HPCR_COMMAND_NUM		8
#define SW_SDRAM_BP_HPCR_WAIT_STATE			4
#define SW_SDRAM_BP_HPCR_PRIORITY_LEVEL		2
#define SW_SDRAM_BP_HPCR_ACCESS_EN			0

enum sw_usbc_type{
	SW_USB_UNKOWN = 0,
	SW_USB_EHCI,
	SW_USB_OHCI,
};

struct sw_hci_hcd{
	__u32 usbc_no;						/* usb controller number */
	__u32 irq_no;						/* interrupt number 	*/
	char hci_name[32];                  /* hci name             */

	struct resource	*usb_base_res;   	/* USB  resources 		*/
	struct resource	*usb_base_req;   	/* USB  resources 		*/
	void __iomem	*usb_vbase;			/* USB  base address 	*/

	void __iomem	* ehci_base;
	__u32 ehci_reg_length;
	void __iomem	* ohci_base;
	__u32 ohci_reg_length;

	struct resource	*sram_base_res;   	/* SRAM resources 		*/
	struct resource	*sram_base_req;   	/* SRAM resources 		*/
	void __iomem	*sram_vbase;		/* SRAM base address 	*/
	__u32 sram_reg_start;
	__u32 sram_reg_length;

	struct resource	*clock_base_res;   	/* clock resources 		*/
	struct resource	*clock_base_req;   	/* clock resources 		*/
	void __iomem	*clock_vbase;		/* clock base address 	*/
	__u32 clock_reg_start;
	__u32 clock_reg_length;

	struct resource	*gpio_base_res;   	/* gpio resources 		*/
	struct resource	*gpio_base_req;   	/* gpio resources 		*/
	void __iomem	*gpio_vbase;		/* gpio base address 	*/
	__u32 gpio_reg_start;
	__u32 gpio_reg_length;

	struct resource	*sdram_base_res;   	/* sdram resources 		*/
	struct resource	*sdram_base_req;   	/* sdram resources 		*/
	void __iomem	*sdram_vbase;		/* sdram base address 	*/
	__u32 sdram_reg_start;
	__u32 sdram_reg_length;

	struct platform_device *pdev;
	struct usb_hcd *hcd;

	struct clk	*sie_clk;				/* SIE clock handle 	*/
	struct clk	*phy_gate;				/* PHY clock handle 	*/
	struct clk	*phy_reset;				/* PHY reset handle 	*/
	struct clk	*ohci_gate;			    /* ohci clock handle 	*/
	__u32 clk_is_open;					/* is usb clock open 	*/


	u32 drv_vbus_Handle;
	u32 drv_vbus_1_Handle;
	script_item_u drv_vbus_gpio_set;
	script_item_u restrict_gpio_set;
	u32 drv_vbus_gpio_valid;
	u32 usb_restrict_valid;
	__u32 power_flag;                   /* flag. �Ƿ񹩵�       */

    __u32 used;                         /* flag. �������Ƿ�ʹ�� */
	__u32 probe;                        /* ��������ʼ�� */
	__u32 host_init_state;				/* usb �������ĳ�ʼ��״̬��0 : ������. 1 : ���� */
	__u32 usb_restrict_flag;
	__u32 usbc_type;                    /* usb controller type  */

	int (* open_clock)(struct sw_hci_hcd *sw_hci, u32 ohci);
	int (* close_clock)(struct sw_hci_hcd *sw_hci, u32 ohci);
    void (* set_power)(struct sw_hci_hcd *sw_hci, int is_on);
	void (* port_configure)(struct sw_hci_hcd *sw_hci, u32 enable);
	void (* usb_passby)(struct sw_hci_hcd *sw_hci, u32 enable);
};



#endif   //__SW_HCI_SUN4I_H__




