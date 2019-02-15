/*
 * sunxi Camera register define
 * Author:raymonxiu
*/
#ifndef _SUNXI_CSI0_REG_H_
#define _SUNXI_CSI0_REG_H_

#include <mach/platform.h>

#define  W(addr, val)   writel(val, addr)
#define  R(addr)        readl(addr)
#define  S(addr,bit)	writel(readl(addr)|bit,addr)
#define  C(addr,bit)	writel(readl(addr)&(~bit),addr)

#define CSI0_REGS_BASE        SW_PA_CSI0_IO_BASE /* 0x01c09000 */
#define CSI1_REGS_BASE        SW_PA_CSI1_IO_BASE /* 0X01c1D000 */
#define CSI0_REG_SIZE 				0x1000
#define CSI1_REG_SIZE 				0x1000

//FPGA
#define SW_INTC_IRQNO_CSI0              AW_IRQ_CSI0
#define SW_INTC_IRQNO_CSI1              AW_IRQ_CSI1

#define CSI_REG_EN           (0x00)
#define CSI_REG_CONF         (0x04)
#define CSI_REG_CTRL         (0x08)
#define CSI_REG_SCALE        (0x0C)
#define CSI_REG_BUF_0_A      (0x10)
#define CSI_REG_BUF_0_B      (0x14)
#define CSI_REG_BUF_1_A      (0x18)
#define CSI_REG_BUF_1_B      (0x1C)
#define CSI_REG_BUF_2_A      (0x20)
#define CSI_REG_BUF_2_B      (0x24)
#define CSI_REG_BUF_CTRL     (0x28)
#define CSI_REG_STATUS       (0x2C)
#define CSI_REG_INT_EN       (0x30)
#define CSI_REG_INT_STATUS   (0x34)
#define CSI_REG_RESIZE_H     (0x40)
#define CSI_REG_RESIZE_V     (0x44)
#define CSI_REG_BUF_LENGTH   (0x48)

#endif  /* _CSI_H_ */
