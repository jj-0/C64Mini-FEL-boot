#ifndef _LCDC_TVE_H_
#define _LCDC_TVE_H_

#include "../../bsp_display.h"

#define LCDC_VBI_LCD_EN 0x80000000
#define LCDC_VBI_HD_EN 0x40000000
#define LCDC_LTI_LCD_EN 0x20000000
#define LCDC_LTI_HD_EN 0x10000000
#define LCDC_VBI_LCD 0x00008000
#define LCDC_VBI_HD 0x00004000
#define LCDC_LTI_LCD_FLAG 0x00002000
#define LCDC_LTI_HD_FLAG 0x00001000
                    
typedef enum
{
        LCDC_SRC_BE0 	= 0, 
        LCDC_SRC_BE1 	= 1,
        LCDC_SRC_DMA 	= 2,
        LCDC_SRC_BLACK 	= 3,
        LCDC_SRC_WHITE 	= 4,
        LCDC_SRC_BLUE 	= 5,
}__lcdc_src_t;  

typedef enum
{
        LCDC_LCDIF_HV 	        = 0, 
        LCDC_LCDIF_CPU 	        = 1,
        LCDC_LCDIF_TTL 	        = 2,
        LCDC_LCDIF_LVDS	        = 3,
        LCDC_LCDIF_HV2DSI       = 4,
}__lcdc_lcdif_t; 
  

typedef enum
{
        LCDC_FRM_RGB888 = 0, 
        LCDC_FRM_RGB666 = 1,
        LCDC_FRM_RGB656 = 2,
}__lcdc_frm_t;

typedef struct
{
        __bool  b_interlace;        //1=b_interlace, 0=progressive
        __bool  b_rgb_internal_hd;  //used when TV and VGA output, 0:YUV, 1:RGB
        __bool  b_rgb_remap_io;     //used when LCD and HDMI output, 0:YUV, 1:RGB
        __bool  b_remap_if;         //used when LCD and HDMI output, 0:LCD, 1:HDMI
        __u16   src_x;              //tcon1 source width in pixels
        __u16   src_y;              //tcon1 source height in pixels
        __u16   scl_x;              //tcon1 scale output width size
        __u16   scl_y;              //tcon1 scale output height size
        __u16   out_x;              //tcon1 output width in pixels
        __u16   out_y;              //tcon1 output height in pixels
        __u16   ht;                 //tcon1 horizontal total time 
        __u16   hbp;                //tcon1 back porch
        __u16   vt;                 //tcon1 vertical total time 
        __u16   vbp;                //tcon1 vertical back porch
        __u16   vspw;               //tcon1 vertical sync pulse width in pixels
        __u16   hspw;               //tcon1 horizontal sync pulse width
        __u32   io_pol;             //tcon1 io polarity, 0=normal, 1=inverse
        __u32   io_out;             //tcon1 io output enable, 0=enable output, 1=disable output, be careful!
        __u8    start_delay;
}__tcon1_cfg_t;


#define TVE_D0ActFlags  (0x01)
#define TVE_D1ActFlags  (0x01<<1)
#define TVE_D2ActFlags  (0x01<<2)
#define TVE_D3ActFlags  (0x01<<3)

typedef enum
{
        TVE_MODE_NTSC = 0, 
        TVE_MODE_PAL,
        TVE_MODE_480I,
        TVE_MODE_576I,
        TVE_MODE_480P,
        TVE_MODE_576P,
        TVE_MODE_720P_50HZ,
        TVE_MODE_720P_60HZ,
        TVE_MODE_1080I_50HZ,
        TVE_MODE_1080I_60HZ,
        TVE_MODE_1080P_50HZ,
        TVE_MODE_1080P_60HZ,
        TVE_MODE_VGA
}__tve_mode_t;

typedef enum tag_TVE_DAC
{
        DAC1 = 1, //bit0
        DAC2 = 2, //bit1
        DAC3 = 4  //bit2
}__tve_dac_t;

typedef enum tag_TVE_SRC
{
        CVBS            = 0, 
        SVIDEO_Y        = 1,
        SVIDEO_C        = 2,
        COMPONENT_Y     = 4,
        COMPONENT_PB    = 5,
        COMPONENT_PR    = 6,
        VGA_R           = 4,
        VGA_G           = 5,
        VGA_B           = 6  
}__tve_src_t;


__s32   LCDC_set_reg_base(__u32 sel, __u32 address);
__u32   LCDC_get_reg_base(__u32 sel);
__s32   LCDC_init(__u32 sel);
__s32   LCDC_exit(__u32 sel);
void    LCDC_open(__u32 sel);
void    LCDC_close(__u32 sel);
__s32   LCDC_set_int_line(__u32 sel, __u32 tcon_index,__u32 num);
__s32   LCDC_clear_int(__u32 sel, __u32 irqsrc);
__s32   LCDC_get_timing(__u32 sel,__u32 index,__disp_tcon_timing_t* tt);
__s32   LCDC_enable_int(__u32 sel, __u32 irqsrc);
__s32   LCDC_disable_int(__u32 sel, __u32 irqsrc);
__u32   LCDC_query_int(__u32 sel);
__s32   LCDC_set_start_delay(__u32 sel, __u32 tcon_index, __u8 delay);
__s32   LCDC_get_start_delay(__u32 sel, __u32 tcon_index);
__u32   LCDC_get_cur_line(__u32 sel, __u32 tcon_index);
__s32 	LCDC_enable_output(__u32 sel);
__s32 	LCDC_disable_output(__u32 sel);
__s32 	LCDC_set_output(__u32 sel, __bool value);

void    LCD_CPU_WR(__u32 sel, __u32 index, __u32 data);
void    LCD_CPU_WR_INDEX(__u32 sel, __u32 index);
void    LCD_CPU_WR_DATA(__u32 sel, __u32 data);
void    LCD_CPU_RD(__u32 sel, __u32 index, __u32 *data);
void    LCD_CPU_AUTO_FLUSH(__u32 sel, __u8 en);
void    LCD_CPU_DMA_FLUSH(__u32 sel, __u8 en);
void    LCD_XY_SWAP(__u32 sel);
__s32	LCD_LVDS_open(__u32 sel);
__s32	LCD_LVDS_close(__u32 sel);

__s32 	tcon0_open(__u32 sel);
__s32 	tcon0_close(__u32 sel);
__u32   tcon0_cfg(__u32 sel, __panel_para_t *cfg);
__s32   tcon0_get_width(__u32 sel);
__s32   tcon0_get_height(__u32 sel);
__s32   tcon0_set_dclk_div(__u32 sel, __u8 div);
__s32   tcon0_src_select(__u32 sel, __lcdc_src_t src);
__u32   tcon0_get_dclk_div(__u32 sel);

__s32 	tcon1_open(__u32 sel);
__s32 	tcon1_close(__u32 sel);
__u32   tcon1_cfg(__u32 sel, __tcon1_cfg_t *cfg);
__u32   tcon1_cfg_ex(__u32 sel, __panel_para_t * info);
__s32 	tcon1_set_hdmi_mode(__u32 sel,  __disp_tv_mode_t mode);
__s32 	tcon1_set_tv_mode(__u32 sel, __disp_tv_mode_t mode);
__s32   tcon1_set_vga_mode(__u32 sel,  __disp_vga_mode_t mode);
__s32   tcon1_src_select(__u32 sel, __lcdc_src_t src);
__bool  tcon1_in_valid_regn(__u32 sel, __u32 juststd);
__s32   tcon1_get_width(__u32 sel);
__s32   tcon1_get_height(__u32 sel);
__s32   tcon1_set_gamma_table(__u32 sel, __u32 address,__u32 size);
__s32   tcon1_set_gamma_Enable(__u32 sel, __bool enable);

__u8 	tcon_mux_init(void);
__u8    tcon_set_hdmi_src(__u8 src);
__u8    tcon_set_tv_src(__u32 tv_index, __u8 src);

__s32   TVE_set_reg_base(__u32 sel,__u32 address);
__u32   TVE_get_reg_base(__u32 sel);
__s32   TVE_init(__u32 sel);
__s32   TVE_exit(__u32 sel);
__s32   TVE_open(__u32 sel);
__s32   TVE_close(__u32 sel);
__s32   TVE_set_vga_mode(__u32 sel);
__s32   TVE_set_tv_mode(__u32 sel, __u8 mode);
__u8    TVE_query_interface(__u32 sel,__u8 index);
__u8    TVE_query_int(__u32 sel);
__u8    TVE_clear_int (__u32 sel);
__u8    TVE_dac_int_enable(__u32 sel,__u8 index);
__u8    TVE_dac_int_disable(__u32 sel,__u8 index);
__u8    TVE_dac_autocheck_enable(__u32 sel,__u8 index);
__u8    TVE_dac_autocheck_disable(__u32 sel,__u8 index);
__u8    TVE_dac_enable(__u32 sel,__u8 index);
__u8    TVE_dac_disable(__u32 sel,__u8 index);
__u8    TVE_dac_set_de_bounce(__u32 sel,__u8 index,__u32 times);
__u8    TVE_dac_get_de_bounce(__u32 sel,__u8 index);
__s32   TVE_dac_set_source(__u32 sel,__u32 index,__u32 source);
__s32   TVE_dac_get_source(__u32 sel,__u32 index);
__s32   TVE_get_dac_status(__u32 index);
__u8 	TVE_csc_init(__u32 sel,__u8 type);
__s32   TVE_dac_sel(__u32 sel,__u32 dac, __u32 index);


#endif

