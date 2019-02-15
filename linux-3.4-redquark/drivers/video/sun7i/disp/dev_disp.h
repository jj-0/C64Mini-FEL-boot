#ifndef __DEV_DISP_H__
#define __DEV_DISP_H__

#include "drv_disp_i.h"

#define FB_RESERVED_MEM

struct info_mm {
	void *info_base;	/* Virtual address */
	unsigned long mem_start;	/* Start of frame buffer mem */
					/* (physical address) */
	__u32 mem_len;			/* Length of frame buffer mem */
};



typedef struct
{
	struct device   *       dev;
	
	__u32                   base_image0;
	__u32                   base_image1;
	__u32                   base_scaler0;
	__u32                   base_scaler1;
	__u32                   base_lcdc0;
	__u32                   base_lcdc1;
	__u32                   base_tvec0;
	__u32                   base_tvec1;
	__u32                   base_hdmi;
	__u32                   base_ccmu;
	__u32                   base_sdram;
        __u32                   base_pioc;
	__u32                   base_pwm;
	
        __disp_init_t           disp_init;
    
        __bool                  fb_enable[FB_MAX];
        __fb_mode_t             fb_mode[FB_MAX];
        __u32                   layer_hdl[FB_MAX][2];//[fb_id][0]:screen0 layer handle;[fb_id][1]:screen1 layer handle 
        struct fb_info *        fbinfo[FB_MAX];
        __disp_fb_create_para_t fb_para[FB_MAX];
	wait_queue_head_t       wait[2];
	unsigned long           wait_count[2];
        struct work_struct      resume_work[2];
        //add by heyihang.Jan 28, 2013
        struct work_struct      vsync_work[2];
        ktime_t                 vsync_timestamp[2];
}fb_info_t;

typedef struct
{
        __u32                   mid;
        __u32                   used;
        __u32                   status;
        __u32                   exit_mode;//0:clean all  1:disable interrupt
        __bool                  b_cache[2];
	__bool	                b_lcd_open[2];
        struct timer_list       disp_timer[2];
}__disp_drv_t;


struct alloc_struct_t
{
        __u32 address;                      //?????ڴ??ĵ?ַ
        __u32 size;                         //???????ڴ???С???û?ʵ?ʵõ????ڴ???С
        __u32 o_size;                       //?û????????ڴ???С
        struct alloc_struct_t *next;
};

int disp_open(struct inode *inode, struct file *file);
int disp_release(struct inode *inode, struct file *file);
ssize_t disp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t disp_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int disp_mmap(struct file *file, struct vm_area_struct * vma);
long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

__s32 disp_create_heap(__u32 pHeapHead, __u32 pHeapHeadPhy, __u32 nHeapSize);
void *disp_malloc(__u32 num_bytes, __u32 *phy_addr);
void  disp_free(void *virt_addr, void* phy_addr);

extern __s32 FB_set_3Dui_layer(__u32 sel, __bool en, __s32 left_off, __s32 right_off);
extern __s32 FB_close_hw_layer(__u32 sel, __u32 mask, __bool is_close);
extern __s32 Display_Fb_Request(__u32 fb_id, __disp_fb_create_para_t *fb_para);
extern __s32 Display_Fb_Release(__u32 fb_id);
extern __s32 Display_Fb_get_para(__u32 fb_id, __disp_fb_create_para_t *fb_para);
extern __s32 Display_get_disp_init_para(__disp_init_t * init_para);

extern __s32 DRV_disp_int_process(__u32 sel);

//add by heyihang.Jan 28, 2013
extern __s32 DRV_disp_vsync_event(__u32 sel);

extern __s32 DRV_DISP_Init(void);
extern __s32 DRV_DISP_Exit(void);

extern fb_info_t g_fbi;

extern __disp_drv_t    g_disp_drv;

extern __s32 DRV_lcd_open(__u32 sel);
extern __s32 DRV_lcd_close(__u32 sel);
extern __s32 Fb_Init(__u32 from);
extern __s32 Fb_Exit(void);

#endif
