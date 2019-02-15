
#ifndef __DISP_DISPLAY_H__
#define __DISP_DISPLAY_H__

#include "disp_display_i.h"
#include "disp_layer.h"
#include "disp_scaler.h"
#include "disp_video.h"

#define IMAGE_USED              0x00000004
#define IMAGE_USED_MASK         (~(IMAGE_USED))
#define YUV_CH_USED             0x00000010
#define YUV_CH_USED_MASK        (~(YUV_CH_USED))
#define HWC_USED                0x00000040
#define HWC_USED_MASK           (~(HWC_USED))
#define LCDC_TCON0_USED         0x00000080
#define LCDC_TCON0_USED_MASK    (~(LCDC_TCON0_USED))
#define LCDC_TCON1_USED         0x00000100
#define LCDC_TCON1_USED_MASK    (~(LCDC_TCON1_USED))
#define SCALER_USED             0x00000200
#define SCALER_USED_MASK        (~(SCALER_USED))

#define LCD_ON      0x00010000
#define LCD_OFF     (~(LCD_ON))
#define TV_ON       0x00020000
#define TV_OFF      (~(TV_ON))
#define HDMI_ON     0x00040000
#define HDMI_OFF    (~(HDMI_ON))
#define VGA_ON      0x00080000
#define VGA_OFF     (~(VGA_ON))

#define VIDEO_PLL0_USED	0x00100000
#define VIDEO_PLL0_USED_MASK (~(VIDEO_PLL0_USED))
#define VIDEO_PLL1_USED 0x00200000
#define VIDEO_PLL1_USED_MASK (~(VIDEO_PLL1_USED))

#define IMAGE_OUTPUT_LCDC            0x00000001
#define IMAGE_OUTPUT_SCALER         0x00000002
#define IMAGE_OUTPUT_LCDC_AND_SCALER 0x00000003

#define DE_FLICKER_USED 0x01000000
#define DE_FLICKER_USED_MASK (~(DE_FLICKER_USED))
#define DE_FLICKER_REQUIRED 0x02000000
#define DE_FLICKER_REQUIRED_MASK (~(DE_FLICKER_REQUIRED))

typedef struct
{
    __bool                  lcd_used;

    __bool                  lcd_bl_en_used;
    disp_gpio_set_t         lcd_bl_en;

    __bool                  lcd_power_used;
    disp_gpio_set_t         lcd_power;

    __bool                  lcd_pwm_used;
    disp_gpio_set_t         lcd_pwm;
    __u32                   lcd_pwm_ch;

    __bool                  lcd_gpio_used[6];  //index4: scl;  index5: sda
    disp_gpio_set_t         lcd_gpio[6];       //index4: scl; index5: sda

    __bool                  lcd_io_used[28];
    disp_gpio_set_t         lcd_io[28];

    __u32                   backlight_bright;
    __u32                   backlight_dimming;//IEP-drc backlight dimming rate: 0 -256 (256: no dimming; 0: the most dimming)
    __u32                   backlight_max_limit;

    __u32                   lcd_bright;
    __u32                   lcd_contrast;
    __u32                   lcd_saturation;
    __u32                   lcd_hue;
}__disp_lcd_cfg_t;

typedef struct
{    
    __u32                   status; /*display engine,lcd,tv,vga,hdmi status*/
    __u32                   lcdc_status;//tcon0 used, tcon1 used
    __bool                  have_cfg_reg;
    __u32                   cache_flag;
    __u32                   cfg_cnt;

    __u32                   screen_width;
    __u32                   screen_height;
    __disp_color_t          bk_color;
    __disp_colorkey_t       color_key;
    __u32                   bright;
    __u32                   contrast;
    __u32                   saturation;
    __u32                   hue;
    __bool                  enhance_en;
    __u32                   max_layers;
    __layer_man_t           layer_manage[4];
    __u32                   de_flicker_status;

    __u32                   image_output_type;//see macro definition IMAGE_OUTPUT_XXX above, it can be lcd only /lcd+scaler/ scaler only
    __u32                   out_scaler_index;
    __u32                   hdmi_index;//0: internal hdmi; 1:external hdmi(if exit)
    __bool                  hdmi_hpd;//0:unplug;  1:plugin
    __bool                  b_out_interlace;
    __disp_output_type_t    output_type;//sw status
    __disp_vga_mode_t       vga_mode;
    __disp_tv_mode_t        tv_mode;
    __disp_tv_mode_t        hdmi_mode;
    __u32                   hdmi_hdcp_en;
    __disp_tv_dac_source    dac_source[4];
    __bool                  hdmi_used;
    __disp_out_csc_type_t   output_csc_type;
    __s32                   (*LCD_CPUIF_XY_Swap)(__s32 mode);
    void                    (*LCD_CPUIF_ISR)(void);
    __u32	                pll_use_status;	//lcdc0/lcdc1 using which video pll(0 or 1)

    __u32                   lcd_bright;
    __disp_color_range_t    out_color_range;
    __u32                   out_csc;

    __disp_lcd_cfg_t        lcd_cfg;
    __hdle                  gpio_hdl[6];

    __bool                  vsync_event_en;
    __bool                  dvi_enable;
}__disp_screen_t;

typedef struct
{
    __bool  enable;
    __u32   freq;
    __u32   pre_scal;
    __u32   active_state;
    __u32   duty_ns;
    __u32   period_ns;
    __u32   entire_cycle;
    __u32   active_cycle;
}__disp_pwm_t;

typedef struct
{
    __disp_bsp_init_para    init_para;//para from driver
    __disp_screen_t         screen[2];
    __disp_scaler_t         scaler[2];
    __disp_pwm_t            pwm[2];
    __u32                   print_level;
    __s32                   trd_parallax[2];
__disp_capture_screen_para_t capture_para[2];
}__disp_dev_t;

extern __disp_dev_t gdisp;

#endif
