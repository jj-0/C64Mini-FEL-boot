/*
 * drivers\media\video\sun7i\sun7i_cedar.c
 * (C) Copyright 2007-2011
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <asm/system.h>
#include <asm/siginfo.h>
#include <asm/signal.h>
#include <mach/system.h>
#include <mach/clock.h>
#include <mach/memory.h>
#include "sun7i_cedar.h"
#include <linux/pm.h>

#define DRV_VERSION "0.01alpha"

#define CHIP_VERSION_F51

#undef USE_CEDAR_ENGINE

#ifndef CEDARDEV_MAJOR
#define CEDARDEV_MAJOR (150)
#endif
#ifndef CEDARDEV_MINOR
#define CEDARDEV_MINOR (0)
#endif

//#define CEDAR_DEBUG

#define CONFIG_SW_SYSMEM_RESERVED_BASE 0x43000000
#define CONFIG_SW_SYSMEM_RESERVED_SIZE 75776

#define VE_CLK_HIGH_WATER  (500)//400MHz
#define VE_CLK_LOW_WATER   (100) //160MHz

int g_dev_major = CEDARDEV_MAJOR;
int g_dev_minor = CEDARDEV_MINOR;
module_param(g_dev_major, int, S_IRUGO);//S_IRUGO represent that g_dev_major can be read,but canot be write
module_param(g_dev_minor, int, S_IRUGO);

#define VE_IRQ_NO AW_IRQ_VE

struct clk *ve_moduleclk = NULL;
struct clk *ve_pll4clk = NULL;
struct clk *ahb_veclk = NULL;
struct clk *dram_veclk = NULL;
struct clk *avs_moduleclk = NULL;
struct clk *hosc_clk = NULL;

static unsigned long pll4clk_rate = 300000000;

extern unsigned long ve_start;
extern unsigned long ve_size;
extern int flush_clean_user_range(long start, long end);
struct iomap_para{
	volatile char* regs_macc;
	#ifdef CHIP_VERSION_F51
	volatile char* regs_avs;
	#else
	volatile char* regs_ccmu;
	#endif
};

static DECLARE_WAIT_QUEUE_HEAD(wait_ve);
struct cedar_dev {
	struct cdev cdev;                       /* char device struct                  */
	struct device *dev;                     /* ptr to class device struct          */
	struct class  *class;                   /* class for auto create device node   */

	struct semaphore sem;                   /* mutual exclusion semaphore          */

	wait_queue_head_t wq;                   /* wait queue for poll ops             */

	struct iomap_para iomap_addrs;          /* io remap addrs                      */

	struct timer_list cedar_engine_timer;
	struct timer_list cedar_engine_timer_rel;

	u32 irq;                                /* cedar video engine irq number       */
	u32 irq_flag;                           /* flag of video engine irq generated  */
	u32 irq_value;                          /* value of video engine irq           */
	u32 irq_has_enable;
	u32 ref_count;
};
struct cedar_dev *cedar_devp;

u32 int_sta=0,int_value;

/*
 * Video engine interrupt service routine
 * To wake up ve wait queue
 */
static irqreturn_t VideoEngineInterupt(int irq, void *dev)
{
	unsigned int ve_int_ctrl_reg;
	volatile int val;
	int modual_sel;
	struct iomap_para addrs = cedar_devp->iomap_addrs;

	modual_sel = readl(addrs.regs_macc + 0);
	modual_sel &= 0xf;

	/* estimate Which video format */
	switch (modual_sel) {
	case 0: //mpeg124
		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x100 + 0x14);
		break;
	case 1: //h264
		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x200 + 0x20);
		break;
	case 2: //vc1
		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x300 + 0x24);
		break;
	case 3: //rmvb
		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x400 + 0x14);
		break;
	case 0xa: //isp
		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0xa00 + 0x08);
		break;
	case 0xb: //avc enc
		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0xb00 + 0x14);
		break;
	default:
		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x100 + 0x14);
		printk("macc modual sel not defined!\n");
		break;
	}

	//disable interrupt
	if(modual_sel == 0) {
		val = readl(ve_int_ctrl_reg);
		writel(val & (~0x7c), ve_int_ctrl_reg);
	} else {
		val = readl(ve_int_ctrl_reg);
		writel(val & (~0xf), ve_int_ctrl_reg);
	}

	cedar_devp->irq_value = 1; //hx modify 2011-8-1 16:08:47
	cedar_devp->irq_flag = 1;
	//any interrupt will wake up wait queue
	wake_up_interruptible(&wait_ve); //ioctl
	return IRQ_HANDLED;
}

/*
 * poll operateion for wait for ve irq
 */
unsigned int cedardev_poll(struct file *filp, struct poll_table_struct *wait)
{
	int mask = 0;
	struct cedar_dev *devp = filp->private_data;

	poll_wait(filp, &devp->wq, wait);
	if (devp->irq_flag == 1) {
		devp->irq_flag = 0;
		mask |= POLLIN | POLLRDNORM;
	}
	return mask;
}

static int clk_status = 0;
static LIST_HEAD(run_task_list);
static LIST_HEAD(del_task_list);
static spinlock_t cedar_spin_lock;
#define CEDAR_RUN_LIST_NONULL	-1
#define CEDAR_NONBLOCK_TASK  0      //������
#define CEDAR_BLOCK_TASK 1
#define CLK_REL_TIME 10000	//10��
#define TIMER_CIRCLE 50		//50����
#define TASK_INIT      0x00
#define TASK_TIMEOUT   0x55
#define TASK_RELEASE   0xaa
#define SIG_CEDAR		35

int enable_cedar_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	if (clk_status == 1)
		goto out;
	clk_status = 1;

	if(0 != clk_enable(ahb_veclk)){
		pr_err("%s(%d) err: enable ahb_veclk failed\n", __func__, __LINE__);
		goto out;
	}
	if(0 != clk_enable(ve_moduleclk)){
		pr_err("%s(%d) err: enable ve_moduleclk failed\n", __func__, __LINE__);
		goto out3;
	}
	if(0 != clk_enable(dram_veclk)){
		pr_err("%s(%d) err: enable dram_veclk failed\n", __func__, __LINE__);
		goto out2;
	}
	if(0 != clk_enable(avs_moduleclk)){
		pr_err("%s(%d) err: enable avs_moduleclk failed\n", __func__, __LINE__);
		goto out1;
	}
	#ifdef CEDAR_DEBUG
	printk("%s,%d\n",__func__,__LINE__);
	#endif
	res = 0;
	goto out;

out1:
	clk_disable(dram_veclk);
out2:
	clk_disable(ve_moduleclk);
out3:
	clk_disable(ahb_veclk);
out:
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
	return res;
}

int disable_cedar_hw_clk(void)
{
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);

	if (clk_status == 0)
		goto out;
	clk_status = 0;

	clk_disable(dram_veclk);
	clk_disable(ve_moduleclk);
	clk_disable(ahb_veclk);
	clk_disable(avs_moduleclk);
	#ifdef CEDAR_DEBUG
	printk("%s,%d\n",__func__,__LINE__);
	#endif
out:
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
	return 0;
}

void cedardev_insert_task(struct cedarv_engine_task* new_task)
{
	struct cedarv_engine_task *task_entry;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);
	if(list_empty(&run_task_list))
		new_task->is_first_task = 1;

	/*
	 * ����run_task_list�������������������ȼ�������ڵ��е��������ȼ��ߣ����ҵ�ǰ���������ǵ�һ�����������
	 * ��ô�ͽ����ȼ��ߵ��������ǰ�棬�����е������ȡ�Ӹߵ��׵����ȼ������Ŷӡ�
	 */
	list_for_each_entry(task_entry, &run_task_list, list) {
		if((task_entry->is_first_task == 0) && (task_entry->running == 0)
			&& (task_entry->t.task_prio < new_task->t.task_prio))
			break;
	}
	list_add(&new_task->list, task_entry->list.prev);

	#ifdef CEDAR_DEBUG
	printk("%s,%d, TASK_ID:",__func__,__LINE__);
	list_for_each_entry(task_entry, &run_task_list, list) {
		printk("%d!", task_entry->t.ID);
	}
	printk("\n");
	#endif

	/* ÿ�β���һ�����񣬾ͽ���ǰ�ļ�ʱ��ʱ������Ϊϵͳ��ǰ��jiffies */
	mod_timer(&cedar_devp->cedar_engine_timer, jiffies + 0);
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

int cedardev_del_task(int task_id)
{
	struct cedarv_engine_task *task_entry;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);
	/*
	 * ����run_task_list����
	 * ����ҵ���Ӧ��id�ţ���ô�ͽ�run_task_list�����е������Ƶ�del_task_list����ı�ͷ��
	 */
	list_for_each_entry(task_entry, &run_task_list, list) {
		if (task_entry->t.ID == task_id && task_entry->status != TASK_RELEASE) {
			task_entry->status = TASK_RELEASE;

			spin_unlock_irqrestore(&cedar_spin_lock, flags);
			mod_timer(&cedar_devp->cedar_engine_timer, jiffies + 0);
			return 0;
		}
	}
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
	/* �Ҳ�����Ӧ ID */
	return -1;
}

int cedardev_check_delay(int check_prio)
{
	struct cedarv_engine_task *task_entry;
	int timeout_total = 0;
	unsigned long flags;

	/* ��ȡ�ܵĵȴ�ʱ�� */
	spin_lock_irqsave(&cedar_spin_lock, flags);
	list_for_each_entry(task_entry, &run_task_list, list) {
		if((task_entry->t.task_prio >= check_prio) || (task_entry->running == 1)
			|| (task_entry->is_first_task == 1))
			timeout_total = timeout_total + task_entry->t.frametime;
	}

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
#ifdef CEDAR_DEBUG
	printk("%s,%d,%d\n", __func__, __LINE__, timeout_total);
#endif
	return timeout_total;
}

static void cedar_engine_for_timer_rel(unsigned long arg)
{
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);
	if(list_empty(&run_task_list))
		disable_cedar_hw_clk();
	else {
		printk("Warring: cedar engine timeout for clk disable, but task left, something wrong?\n");
		mod_timer(&cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

static void cedar_engine_for_events(unsigned long arg)
{
	struct cedarv_engine_task *task_entry, *task_entry_tmp;
	struct siginfo info;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);
	list_for_each_entry_safe(task_entry, task_entry_tmp, &run_task_list, list) {
		mod_timer(&cedar_devp->cedar_engine_timer_rel, jiffies + msecs_to_jiffies(CLK_REL_TIME));
		if(task_entry->status == TASK_RELEASE || time_after(jiffies, task_entry->t.timeout)) {
			if (task_entry->status == TASK_INIT)
				task_entry->status = TASK_TIMEOUT;
			list_move(&task_entry->list, &del_task_list);
		}
	}

	list_for_each_entry_safe(task_entry, task_entry_tmp, &del_task_list, list) {
		info.si_signo = SIG_CEDAR;
		info.si_code = task_entry->t.ID;
		if (task_entry->status == TASK_TIMEOUT) { /* ��ʾ����timeoutɾ�� */
			info.si_errno = TASK_TIMEOUT;
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		} else if(task_entry->status == TASK_RELEASE) { /* ��ʾ���������������ɾ�� */
			info.si_errno = TASK_RELEASE;
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		}
		list_del(&task_entry->list);
		kfree(task_entry);
	}

	/* ���������е�task */
	if(!list_empty(&run_task_list)){
		task_entry = list_entry(run_task_list.next, struct cedarv_engine_task, list);
		if(task_entry->running == 0){
			task_entry->running = 1;
			info.si_signo = SIG_CEDAR;
			info.si_code = task_entry->t.ID;
			info.si_errno = TASK_INIT; /* �����Ѿ����� */
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		}
		mod_timer(&cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

/*
 * ioctl function
 * including : wait video engine done,
 *             AVS Counter control,
 *             Physical memory control,
 *             module clock/freq control.
 *             cedar engine
 */
long cedardev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long   ret = 0;
	unsigned int v;
	int ve_timeout = 0;
	struct cedar_dev *devp;
#ifdef USE_CEDAR_ENGINE
	int rel_taskid = 0;
	struct __cedarv_task task_ret;
	struct cedarv_engine_task *task_ptr = NULL;
#endif
	unsigned long flags;

	devp = filp->private_data;
	switch (cmd) {
	case IOCTL_ENGINE_REQ:
	#ifdef USE_CEDAR_ENGINE
		if(copy_from_user(&task_ret, (void __user*)arg, sizeof(struct __cedarv_task))){
			printk("IOCTL_ENGINE_REQ copy_from_user fail\n");
			return -EFAULT;
		}
		spin_lock_irqsave(&cedar_spin_lock, flags);
		/* ���taskΪ������״̬�������߿����������� */
		if(!list_empty(&run_task_list) && ( task_ret.block_mode == CEDAR_NONBLOCK_TASK)){
			spin_unlock_irqrestore(&cedar_spin_lock, flags);
			return CEDAR_RUN_LIST_NONULL; /* run_task_list���������񣬷���-1 */
		}
		spin_unlock_irqrestore(&cedar_spin_lock, flags);

		/* ���taskΪ����״̬����task����run_task_list������ */
		task_ptr = kmalloc(sizeof(struct cedarv_engine_task), GFP_KERNEL);
		if(!task_ptr){
			printk("get mem for IOCTL_ENGINE_REQ\n");
			return PTR_ERR(task_ptr);
		}
		task_ptr->task_handle = current;
		task_ptr->t.ID = task_ret.ID;
		task_ptr->t.timeout = jiffies + msecs_to_jiffies(1000*task_ret.timeout); /* ms to jiffies */
		task_ptr->t.frametime = task_ret.frametime;
		task_ptr->t.task_prio = task_ret.task_prio;
		task_ptr->running = 0;
		task_ptr->is_first_task = 0;
		task_ptr->status = TASK_INIT;

		cedardev_insert_task(task_ptr);

		enable_cedar_hw_clk();
		/*
		 * ����run_task_list�����е������ǵ�һ�����񣬷���1�����ǵ�һ�����񷵻�0.
		 * hx modify 2011-7-28 16:59:16������
		 */
		return task_ptr->is_first_task;
	#else
		enable_cedar_hw_clk();
		cedar_devp->ref_count++;
		break;
	#endif

	case IOCTL_ENGINE_REL:
	#ifdef USE_CEDAR_ENGINE
		rel_taskid = (int)arg;
		/*
		 * ���������id�Ž��������ɾ������������ֵ���壺�Ҳ�����ӦID������-1;�ҵ���ӦID������0��
		 */
		ret = cedardev_del_task(rel_taskid);
	#else
		disable_cedar_hw_clk();
		cedar_devp->ref_count--;
	#endif
		return ret;

	case IOCTL_ENGINE_CHECK_DELAY:
	{
		struct cedarv_engine_task_info task_info;
		/*
		 * ���û��ռ��л�ȡҪ��ѯ���������ȼ���ͨ���������ȼ���ͳ����Ҫ�ȴ�����ʱ��total_time.
		 * ������ӿ��У�ͬʱҲ���û������˵�ǰ�����frametime�����������Լ��ٽӿڣ������û��ռ�Ҫ������һ��
		 * �յ�frametimeֵ�����ڵ�ǰtask��frametime��Ҳ�����ö���Ľӿڻ�ȡ��������������
		 * frametime��total_time�ʹ��ڲ�ͬ�ӿ��С��ô�������
		 */
		if(copy_from_user(&task_info, (void __user*)arg, sizeof(struct cedarv_engine_task_info))){
			printk("IOCTL_ENGINE_CHECK_DELAY copy_from_user fail\n");
			return -EFAULT;
		}
		task_info.total_time = cedardev_check_delay(task_info.task_prio); /* task_info.task_prio�Ǵ��ݹ��������ȼ� */
		#ifdef CEDAR_DEBUG
		printk("%s,%d,%d\n", __func__, __LINE__, task_info.total_time);
		#endif
		task_info.frametime = 0;
		spin_lock_irqsave(&cedar_spin_lock, flags);
		if(!list_empty(&run_task_list)){
			/*
			 * ��ȡrun_task_list�����еĵ�һ������Ҳ���ǵ�ǰ���е�����
			 * ͨ����ǰ���е������ȡframetimeʱ��
			 */
			struct cedarv_engine_task *task_entry;
			#ifdef CEDAR_DEBUG
			printk("%s,%d\n",__func__,__LINE__);
			#endif
			task_entry = list_entry(run_task_list.next, struct cedarv_engine_task, list);
			if(task_entry->running == 1)
				task_info.frametime = task_entry->t.frametime;
			#ifdef CEDAR_DEBUG
			printk("%s,%d,%d\n",__func__,__LINE__,task_info.frametime);
			#endif
		}
		spin_unlock_irqrestore(&cedar_spin_lock, flags);
		/*
		 * ���������ȼ���total_time,frametime�������û��ռ䡣�������ȼ������û����õ�ֵ��total_time����Ҫ�ȴ�����ʱ�䣬
		 * frametime�ǵ�ǰ���������ʱ��.��ʵ��ǰ�������Ϣ�������һ���ӿ�ʵ��.������϶Ⱥͽӿڵ���չ��.
		 */
		if (copy_to_user((void *)arg, &task_info, sizeof(struct cedarv_engine_task_info))){
			printk("IOCTL_ENGINE_CHECK_DELAY copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}

	case IOCTL_WAIT_VE:
		//wait_event_interruptible(wait_ve, cedar_devp->irq_flag);
		ve_timeout = (int)arg;
		cedar_devp->irq_value = 0;

		spin_lock_irqsave(&cedar_spin_lock, flags);
		if(cedar_devp->irq_flag)
			cedar_devp->irq_value = 1;
		spin_unlock_irqrestore(&cedar_spin_lock, flags);

		wait_event_interruptible_timeout(wait_ve, cedar_devp->irq_flag, ve_timeout*HZ);
		//printk("%s,%d,ve_timeout:%d,cedar_devp->irq_value:%d\n", __func__, __LINE__, ve_timeout, cedar_devp->irq_value);
		cedar_devp->irq_flag = 0;
		/* ����1����ʾ�жϷ��أ�����0����ʾtimeout���� */
		return cedar_devp->irq_value;

	case IOCTL_ENABLE_VE:
		clk_enable(ve_moduleclk);
		break;

	case IOCTL_DISABLE_VE:
		clk_disable(ve_moduleclk);
		break;

	case IOCTL_RESET_VE:
		clk_disable(dram_veclk);
		//pr_info("%s(%d): IOCTL_RESET_VE to check, liugang added 2013-2-19\n", __func__, __LINE__);
		clk_reset(ve_moduleclk, AW_CCU_CLK_RESET);
		clk_reset(ve_moduleclk, AW_CCU_CLK_NRESET);
		clk_enable(dram_veclk);
		break;

	case IOCTL_SET_VE_FREQ:
	{
		int arg_rate = (int)arg;
#if 0
		if(arg_rate >= 320)
			clk_set_rate(ve_moduleclk, pll4clk_rate/3); //ve_moduleclk rate is 320khz
		else if((arg_rate >= 240) && (arg_rate < 320))
			clk_set_rate(ve_moduleclk, pll4clk_rate/4); //ve_moduleclk rate is 240khz
		else if((arg_rate >= 160) && (arg_rate < 240))
			clk_set_rate(ve_moduleclk, pll4clk_rate/6); //ve_moduleclk rate is 160khz
		else
			printk("IOCTL_SET_VE_FREQ set ve freq error,%s,%d\n", __func__, __LINE__);
#else
		if(arg_rate >= VE_CLK_LOW_WATER &&
				arg_rate <= VE_CLK_HIGH_WATER &&
				clk_get_rate(ve_moduleclk)/1000000 != arg_rate) {
			if(!clk_set_rate(ve_pll4clk, arg_rate*1000000)) {
				pll4clk_rate = clk_get_rate(ve_pll4clk);
				if(clk_set_rate(ve_moduleclk, pll4clk_rate)) {
					printk("set ve clock failed\n");
				}

			} else {
				printk("set pll4 clock failed\n");
			}
		}
		ret = clk_get_rate(ve_moduleclk);
		//printk("pll4 clk %lu, ve clk %ld\n", clk_get_rate(ve_pll4clk), ret);
#endif
		break;
	}

	case IOCTL_GETVALUE_AVS2:
		/* Return AVS1 counter value */
		return readl(cedar_devp->iomap_addrs.regs_avs + 0x88);

	case IOCTL_ADJUST_AVS2:
	{
		int arg_s = (int)arg;
		int temp;

		v = readl(cedar_devp->iomap_addrs.regs_avs + 0x8c);
		temp = v & 0xffff0000;
		temp =temp + temp*arg_s/100;
		temp = temp > (244<<16) ? (244<<16) : temp;
		temp = temp < (234<<16) ? (234<<16) : temp;
		v = (temp & 0xffff0000) | (v&0x0000ffff);
		#ifdef CEDAR_DEBUG
		printk("Kernel AVS ADJUST Print: 0x%x\n", v);
		#endif
		writel(v, cedar_devp->iomap_addrs.regs_avs + 0x8c);
		break;
	}

	case IOCTL_ADJUST_AVS2_ABS:
	{
		int arg_s = (int)arg;
		int v_dst;

		switch(arg_s) {
		case -2:
			v_dst = 234;
			break;
		case -1:
			v_dst = 236;
			break;
		case 1:
			v_dst = 242;
			break;
		case 2:
			v_dst = 244;
			break;
		default:
			v_dst = 239;
			break;
		}

		v = readl(cedar_devp->iomap_addrs.regs_avs + 0x8c);
		v = (v_dst<<16)  | (v&0x0000ffff);
		writel(v, cedar_devp->iomap_addrs.regs_avs + 0x8c);
		break;
	}

	case IOCTL_CONFIG_AVS2:
		/* Set AVS counter divisor */
		v = readl(cedar_devp->iomap_addrs.regs_avs + 0x8c);
		v = 239 << 16 | (v & 0xffff);
		writel(v, cedar_devp->iomap_addrs.regs_avs + 0x8c);

		/* Enable AVS_CNT1 and Pause it */
		v = readl(cedar_devp->iomap_addrs.regs_avs + 0x80);
		v |= 1 << 9 | 1 << 1;
		writel(v, cedar_devp->iomap_addrs.regs_avs + 0x80);

		/* Set AVS_CNT1 init value as zero  */
		writel(0, cedar_devp->iomap_addrs.regs_avs + 0x88);
		break;

	case IOCTL_RESET_AVS2:
		/* Set AVS_CNT1 init value as zero */
		writel(0, cedar_devp->iomap_addrs.regs_avs + 0x88);
		break;

	case IOCTL_PAUSE_AVS2:
		/* Pause AVS_CNT1 */
		v = readl(cedar_devp->iomap_addrs.regs_avs + 0x80);
		v |= 1 << 9;
		writel(v, cedar_devp->iomap_addrs.regs_avs + 0x80);
		break;

	case IOCTL_START_AVS2:
		/* Start AVS_CNT1 : do not pause */
		v = readl(cedar_devp->iomap_addrs.regs_avs + 0x80);
		v &= ~(1 << 9);
		writel(v, cedar_devp->iomap_addrs.regs_avs + 0x80);
		break;

	case IOCTL_GET_ENV_INFO:
	{
		struct cedarv_env_infomation env_info;
		env_info.phymem_start = (unsigned int)phys_to_virt(SW_VE_MEM_BASE); // (0x4d000000);
		env_info.phymem_total_size = 0x05000000;
		env_info.address_macc = (unsigned int)cedar_devp->iomap_addrs.regs_macc;
		if(copy_to_user((char *)arg, &env_info, sizeof(struct cedarv_env_infomation)))
			return -EFAULT;
		break;
	}

	case IOCTL_GET_IC_VER:
		return 0x0A10000B;

	case IOCTL_READ_REG:
	{
		struct cedarv_regop reg_para;
		if(copy_from_user(&reg_para, (void __user*)arg, sizeof(struct cedarv_regop)))
			return -EFAULT;
		return readl(reg_para.addr);
	}

	case IOCTL_WRITE_REG:
	{
		struct cedarv_regop reg_para;
		if(copy_from_user(&reg_para, (void __user*)arg, sizeof(struct cedarv_regop)))
			return -EFAULT;
		writel(reg_para.value, reg_para.addr);
		break;
	}

	case IOCTL_FLUSH_CACHE:
	{
		struct cedarv_cache_range cache_range;
		if(copy_from_user(&cache_range, (void __user*)arg, sizeof(struct cedarv_cache_range))) {
			printk("IOCTL_FLUSH_CACHE copy_from_user fail\n");
			return -EFAULT;
		}
		flush_clean_user_range(cache_range.start, cache_range.end);
		break;
	}

	case IOCTL_SET_REFCOUNT:
		cedar_devp->ref_count = (int)arg;
		break;

	default:
		break;
	}
	return ret;
}

static int cedardev_open(struct inode *inode, struct file *filp)
{
	struct cedar_dev *devp;

	devp = container_of(inode->i_cdev, struct cedar_dev, cdev);
	filp->private_data = devp;
	if (down_interruptible(&devp->sem))
		return -ERESTARTSYS;
	/* init other resource here */
	devp->irq_flag = 0;
	up(&devp->sem);
	nonseekable_open(inode, filp);
	return 0;
}

static int cedardev_release(struct inode *inode, struct file *filp)
{
	struct cedar_dev *devp;

	devp = filp->private_data;
	if (down_interruptible(&devp->sem))
		return -ERESTARTSYS;
	/* release other resource here */
	devp->irq_flag = 1;
	up(&devp->sem);
	return 0;
}

void cedardev_vma_open(struct vm_area_struct *vma)
{
}

void cedardev_vma_close(struct vm_area_struct *vma)
{
}

static struct vm_operations_struct cedardev_remap_vm_ops = {
	.open  = cedardev_vma_open,
	.close = cedardev_vma_close,
};

static int cedardev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long temp_pfn;
	unsigned int  VAddr;
	struct iomap_para addrs;
	unsigned int io_ram = 0;

	VAddr = vma->vm_pgoff << 12;
	addrs = cedar_devp->iomap_addrs;

	if (VAddr == (unsigned int)addrs.regs_macc) {
		temp_pfn = MACC_REGS_BASE >> 12;
		io_ram = 1;
	} else {
		temp_pfn = (__pa(vma->vm_pgoff << 12))>>12;
		io_ram = 0;
	}

	if (io_ram == 0) {
		/* Set reserved and I/O flag for the area. */
		vma->vm_flags |= VM_RESERVED | VM_IO;

		/* Select uncached access. */
		//vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		if (remap_pfn_range(vma, vma->vm_start, temp_pfn,
			vma->vm_end - vma->vm_start, vma->vm_page_prot))
			return -EAGAIN;
	} else {
		/* Set reserved and I/O flag for the area. */
		vma->vm_flags |= VM_RESERVED | VM_IO;
		/* Select uncached access. */
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
				vma->vm_end - vma->vm_start, vma->vm_page_prot))
			return -EAGAIN;
	}
	vma->vm_ops = &cedardev_remap_vm_ops;
	cedardev_vma_open(vma);
	return 0;
}

#ifdef CONFIG_PM
static int snd_sw_cedar_suspend(struct platform_device *pdev,pm_message_t state)
{
	pr_info("%s(%d): enter %s\n", __func__, __LINE__, (NORMAL_STANDBY == standby_type) ?
		"normal standby" : "super standby");
	disable_cedar_hw_clk();
	return 0;
}

static int snd_sw_cedar_resume(struct platform_device *pdev)
{
	pr_info("%s(%d): resume from %s\n", __func__, __LINE__, (NORMAL_STANDBY == standby_type) ?
		"normal standby" : "super standby");
	if(cedar_devp->ref_count == 0)
		return 0;
	enable_cedar_hw_clk();
	return 0;
}
#endif

static struct file_operations cedardev_fops = {
	.owner   = THIS_MODULE,
	.mmap    = cedardev_mmap,
	.poll    = cedardev_poll,
	.open    = cedardev_open,
	.release = cedardev_release,
	.llseek  = no_llseek,
	.unlocked_ioctl = cedardev_ioctl,
};

/* data relating */
static struct platform_device sw_device_cedar = {
	.name = "sun7i-cedar",
};

/* method relating */
static struct platform_driver sw_cedar_driver = {
#ifdef CONFIG_PM
	.suspend	= snd_sw_cedar_suspend,
	.resume		= snd_sw_cedar_resume,
#endif
	.driver		= {
		.name	= "sun7i-cedar",
	},
};

static int __init cedardev_init(void)
{
	int ret = 0;
	int err = 0;
	int devno;
	unsigned int val;
	dev_t dev = 0;

	printk("[hx-cedar dev]: install start!!!\n");
	if((platform_device_register(&sw_device_cedar))<0)
		return err;
	if((err = platform_driver_register(&sw_cedar_driver)) < 0)
		return err;

	/*register or alloc the device number.*/
	if(g_dev_major) {
		dev = MKDEV(g_dev_major, g_dev_minor);
		ret = register_chrdev_region(dev, 1, "cedar_dev");
	} else {
		ret = alloc_chrdev_region(&dev, g_dev_minor, 1, "cedar_dev");
		g_dev_major = MAJOR(dev);
		g_dev_minor = MINOR(dev);
	}
	if(ret < 0) {
		printk(KERN_WARNING "cedar_dev: can't get major %d\n", g_dev_major);
		return ret;
	}

	spin_lock_init(&cedar_spin_lock);
	cedar_devp = kmalloc(sizeof(struct cedar_dev), GFP_KERNEL);
	if(cedar_devp == NULL) {
		printk("malloc mem for cedar device err\n");
		return -ENOMEM;
	}
	memset(cedar_devp, 0, sizeof(struct cedar_dev));
	cedar_devp->irq = VE_IRQ_NO;

	sema_init(&cedar_devp->sem, 1);
	init_waitqueue_head(&cedar_devp->wq);
	memset(&cedar_devp->iomap_addrs, 0, sizeof(struct iomap_para));

	ret = request_irq(VE_IRQ_NO, VideoEngineInterupt, 0, "cedar_dev", NULL);
	if(ret < 0) {
		printk("request irq err\n");
		return -EINVAL;
	}
	/* map for macc io space */
	cedar_devp->iomap_addrs.regs_macc = ioremap(MACC_REGS_BASE, 4096);
	if(!cedar_devp->iomap_addrs.regs_macc)
		printk("cannot map region for macc");
	cedar_devp->iomap_addrs.regs_avs = ioremap(AVS_REGS_BASE, 1024);

	//VE_SRAM mapping to AC320
	val = readl(0xf1c00000);
	val &= 0x80000000;
	writel(val,0xf1c00000);
	//remapping SRAM to MACC for codec test
	val = readl(0xf1c00000);
	val |= 0x7fffffff;
	writel(val,0xf1c00000);

	ve_pll4clk = clk_get(NULL, CLK_SYS_PLL4);
	if(!ve_pll4clk || IS_ERR(ve_pll4clk)){
		printk("%s(%d) err: clk_get %s failed!\n", __func__, __LINE__, CLK_SYS_PLL4);
		return -EFAULT;
	}
	pll4clk_rate = clk_get_rate(ve_pll4clk);
	/* getting ahb clk for ve!(macc) */
	ahb_veclk = clk_get(NULL, CLK_AHB_VE);
	if(!ahb_veclk || IS_ERR(ahb_veclk)){
		printk("%s(%d) err: clk_get %s failed!\n", __func__, __LINE__, CLK_AHB_VE);
		return -EFAULT;
	}
	ve_moduleclk = clk_get(NULL, CLK_MOD_VE);
	if(!ve_moduleclk || IS_ERR(ve_moduleclk)){
		printk("%s(%d) err: clk_get %s failed!\n", __func__, __LINE__, CLK_MOD_VE);
		return -EFAULT;
	}
	if(clk_set_parent(ve_moduleclk, ve_pll4clk)){
		printk("set parent of ve_moduleclk to ve_pll4clk failed!\n");
		return -EFAULT;
	}
	/* default the ve freq to 160M by lys 2011-12-23 15:25:34 */
	clk_set_rate(ve_moduleclk, pll4clk_rate);
	/* geting dram clk for ve! */
	dram_veclk = clk_get(NULL, CLK_DRAM_VE);
	if(!dram_veclk || IS_ERR(dram_veclk)){
		printk("%s(%d) err: clk_get %s failed!\n", __func__, __LINE__, CLK_DRAM_VE);
		return -EFAULT;
	}
	hosc_clk = clk_get(NULL, CLK_SYS_HOSC);
	if(!hosc_clk || IS_ERR(hosc_clk)){
		printk("%s(%d) err: clk_get %s failed!\n", __func__, __LINE__, CLK_SYS_HOSC);
		return -EFAULT;
	}
	avs_moduleclk = clk_get(NULL, CLK_MOD_AVS);
	if(!avs_moduleclk || IS_ERR(avs_moduleclk)){
		printk("%s(%d) err: clk_get %s failed!\n", __func__, __LINE__, CLK_MOD_AVS);
		return -EFAULT;
	}
	if(clk_set_parent(avs_moduleclk, hosc_clk)){
		printk("set parent of avs_moduleclk to hosc_clk failed!\n");
		return -EFAULT;
	}
	clk_reset(ve_moduleclk, AW_CCU_CLK_NRESET);
	/* for clk test */
	#ifdef CEDAR_DEBUG
	printk("PLL4 CLK:0xf1c20018 is:%x\n", *(volatile int *)0xf1c20018);
	printk("AHB CLK:0xf1c20064 is:%x\n", *(volatile int *)0xf1c20064);
	printk("VE CLK:0xf1c2013c is:%x\n", *(volatile int *)0xf1c2013c);
	printk("SDRAM CLK:0xf1c20100 is:%x\n", *(volatile int *)0xf1c20100);
	printk("SRAM:0xf1c00000 is:%x\n", *(volatile int *)0xf1c00000);
	#endif

	/* Create char device */
	devno = MKDEV(g_dev_major, g_dev_minor);
	cdev_init(&cedar_devp->cdev, &cedardev_fops);
	cedar_devp->cdev.owner = THIS_MODULE;
	cedar_devp->cdev.ops = &cedardev_fops;
	ret = cdev_add(&cedar_devp->cdev, devno, 1);
	if(ret)
		printk(KERN_NOTICE "Err:%d add cedardev", ret);
	cedar_devp->class = class_create(THIS_MODULE, "cedar_dev");
	cedar_devp->dev   = device_create(cedar_devp->class, NULL, devno, NULL, "cedar_dev");
	/*
	 * ��cedar drv��ʼ����ʱ�򣬳�ʼ����ʱ�����������ĳ�Ա
	 * �����������run_task_list��ʱ��������ʱ���������ö�ʱ����ʱ��Ϊ��ǰϵͳ��jiffies���ο�cedardev_insert_task
	 */
	setup_timer(&cedar_devp->cedar_engine_timer, cedar_engine_for_events, (unsigned long)cedar_devp);
	setup_timer(&cedar_devp->cedar_engine_timer_rel, cedar_engine_for_timer_rel, (unsigned long)cedar_devp);
	printk("[cedar dev]: install end!!!\n");
	return 0;
}
module_init(cedardev_init);

static void __exit cedardev_exit(void)
{
	dev_t dev;

	dev = MKDEV(g_dev_major, g_dev_minor);
	free_irq(VE_IRQ_NO, NULL);
	iounmap(cedar_devp->iomap_addrs.regs_macc);
	iounmap(cedar_devp->iomap_addrs.regs_avs);
	/* destroy char device */
	if(cedar_devp){
		cdev_del(&cedar_devp->cdev);
		device_destroy(cedar_devp->class, dev);
		class_destroy(cedar_devp->class);
	}
	clk_disable(dram_veclk);
	clk_put(dram_veclk);

	clk_disable(ve_moduleclk);
	clk_put(ve_moduleclk);

	clk_disable(ahb_veclk);
	clk_put(ahb_veclk);

	clk_put(ve_pll4clk);

	clk_disable(avs_moduleclk);
	clk_put(avs_moduleclk);

	unregister_chrdev_region(dev, 1);
	platform_driver_unregister(&sw_cedar_driver);
	if(cedar_devp)
		kfree(cedar_devp);
}
module_exit(cedardev_exit);

MODULE_AUTHOR("Soft-Reuuimlla");
MODULE_DESCRIPTION("User mode CEDAR device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
