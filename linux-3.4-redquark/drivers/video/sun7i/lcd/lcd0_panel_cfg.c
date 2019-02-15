
#include "lcd_panel_cfg.h"

//delete this line if you want to use the lcd para define in sys_config1.fex
//#define LCD_PARA_USE_CONFIG

static void LCD_power_on(__u32 sel);
static void LCD_power_off(__u32 sel);
static void LCD_bl_open(__u32 sel);
static void LCD_bl_close(__u32 sel);


static void LCD_panel_init(__u32 sel);
static void LCD_panel_exit(__u32 sel);
void lp079x01_init(void);
void lp079x01_exit(void);
#define spi_csx_set(v)	(LCD_GPIO_write(0, 3, v))       //PA05,pio3
#define spi_sck_set(v)  (LCD_GPIO_write(0, 0, v))	//PA06,pio0
#define spi_sdi_set(v)  (LCD_GPIO_write(0, 1, v))	//PA07,pio1

#define lcd_panel_rst(v)(LCD_GPIO_write(0, 2, v))       //PH24
#define lcd_2828_rst(v) (LCD_GPIO_write(0, 4, v))	//PH23
#define lcd_2828_pd(v)  (LCD_GPIO_write(0, 5, v))	//PH22


#ifdef LCD_PARA_USE_CONFIG
static __u8 g_gamma_tbl[][2] = 
{
        //{input value, corrected value}
        {0, 0},
        {15, 15},
        {30, 30},
        {45, 45},
        {60, 60},
        {75, 75},
        {90, 90},
        {105, 105},
        {120, 120},
        {135, 135},
        {150, 150},
        {165, 165},
        {180, 180},
        {195, 195},
        {210, 210},
        {225, 225},
        {240, 240},
        {255, 255},
};

static void LCD_cfg_panel_info(__panel_para_t * info)
{
        __u32 i = 0, j=0;

        memset(info,0,sizeof(__panel_para_t));

        info->lcd_x             = 800;
        info->lcd_y             = 480;
        info->lcd_dclk_freq     = 33;       //MHz

        info->lcd_pwm_not_used  = 0;
        info->lcd_pwm_ch        = 0;
        info->lcd_pwm_freq      = 10000;     //Hz
        info->lcd_pwm_pol       = 0;

        info->lcd_if            = 0;        //0:hv(sync+de); 1:8080; 2:ttl; 3:lvds

        info->lcd_hbp           = 215;      //hsync back porch
        info->lcd_ht            = 1055;     //hsync total cycle
        info->lcd_hspw          = 0;        //hsync plus width
        info->lcd_vbp           = 34;       //vsync back porch
        info->lcd_vt            = 2 * 525;  //vysnc total cycle *2
        info->lcd_vspw          = 0;        //vysnc plus width

        info->lcd_hv_if         = 0;        //0:hv parallel 1:hv serial 
        info->lcd_hv_smode      = 0;        //0:RGB888 1:CCIR656
        info->lcd_hv_s888_if    = 0;        //serial RGB format
        info->lcd_hv_syuv_if    = 0;        //serial YUV format

        info->lcd_cpu_if        = 0;        //0:18bit 4:16bit
        info->lcd_frm           = 0;        //0: disable; 1: enable rgb666 dither; 2:enable rgb656 dither

        info->lcd_lvds_ch       = 0;        //0:single channel; 1:dual channel
        info->lcd_lvds_mode     = 0;        //0:NS mode; 1:JEIDA mode
        info->lcd_lvds_bitwidth = 0;        //0:24bit; 1:18bit
        info->lcd_lvds_io_cross = 0;        //0:normal; 1:pn cross

        info->lcd_io_cfg0       = 0x10000000;

        info->lcd_gamma_correction_en = 0;
        if(info->lcd_gamma_correction_en)
        {
                __u32 items = sizeof(g_gamma_tbl)/2;

        for(i=0; i<items-1; i++)
        {
                __u32 num = g_gamma_tbl[i+1][0] - g_gamma_tbl[i][0];

                //__inf("handling{%d,%d}\n", g_gamma_tbl[i][0], g_gamma_tbl[i][1]);
                for(j=0; j<num; j++)
                {
                        __u32 value = 0;

                        value = g_gamma_tbl[i][1] + ((g_gamma_tbl[i+1][1] - g_gamma_tbl[i][1]) * j)/num;
                        info->lcd_gamma_tbl[g_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
                        //__inf("----gamma %d, %d\n", g_gamma_tbl[i][0] + j, value);
                }
        }
        info->lcd_gamma_tbl[255] = (g_gamma_tbl[items-1][1]<<16) + (g_gamma_tbl[items-1][1]<<8) + g_gamma_tbl[items-1][1];
        //__inf("----gamma 255, %d\n", g_gamma_tbl[items-1][1]);
        }
}
#endif

void spi_24bit_3wire(__u32 tx)
{
	__u8 i;

	spi_csx_set(0);

	for(i=0;i<24;i++)
	{
		LCD_delay_us(1);
		spi_sck_set(0);
		LCD_delay_us(1);
		if(tx & 0x800000)
			spi_sdi_set(1);
		else
			spi_sdi_set(0);
		LCD_delay_us(1);
		spi_sck_set(1);
		LCD_delay_us(1);
		tx <<= 1;
	}
	spi_sdi_set(1);
	LCD_delay_us(1);
	spi_csx_set(1);
	LCD_delay_us(3);
}

static __s32 LCD_open_flow(__u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 50);           //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 50);         //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, TCON_open,	200);           //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);             //open lcd backlight, and delay 0ms
	return 0;
}

static __s32 LCD_close_flow(__u32 sel)
{	
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);           //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, TCON_close,	50);            //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 50);         //close lcd power, and delay 500ms
	return 0;
}

static void LCD_power_on(__u32 sel)
{
        LCD_POWER_EN(sel, 1);//config lcd_power pin to open lcd power
}

static void LCD_power_off(__u32 sel)
{
        LCD_POWER_EN(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(__u32 sel)
{
        LCD_PWM_EN(sel, 1);//open pwm module
        LCD_BL_EN(sel, 1);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(__u32 sel)
{
        LCD_BL_EN(sel, 0);//config lcd_bl_en pin to close lcd backlight
        LCD_PWM_EN(sel, 0);//close pwm module
}

static void LCD_panel_init(__u32 sel)
{
        __panel_para_t *info = kmalloc(sizeof(__panel_para_t), GFP_KERNEL | __GFP_ZERO);
        lcd_get_panel_para(sel, info);
        if(info->lcd_if == LCD_IF_HV2DSI)
        {
                lp079x01_init();
        }
        kfree(info);
        return;
}

static void LCD_panel_exit(__u32 sel)
{
        __panel_para_t *info = kmalloc(sizeof(__panel_para_t), GFP_KERNEL | __GFP_ZERO);
        lcd_get_panel_para(sel, info);
        if(info->lcd_if == LCD_IF_HV2DSI)
        {
                lp079x01_exit();
        }
        kfree(info);
        return;
}

//sel: 0:lcd0; 1:lcd1
static __s32 LCD_user_defined_func(__u32 sel, __u32 para1, __u32 para2, __u32 para3)
{
        return 0;
}

void LCD_get_panel_funs_0(__lcd_panel_fun_t * fun)
{
#ifdef LCD_PARA_USE_CONFIG
        fun->cfg_panel_info = LCD_cfg_panel_info;//delete this line if you want to use the lcd para define in sys_config1.fex
#endif
        fun->cfg_open_flow = LCD_open_flow;
        fun->cfg_close_flow = LCD_close_flow;
        fun->lcd_user_defined_func = LCD_user_defined_func;
}

void lp079x01_init(void)
{
        lcd_2828_rst(0);
        lcd_panel_rst(0);
        lcd_2828_pd(0);
        LCD_delay_ms(20);
        lcd_2828_rst(1);
        lcd_panel_rst(1);
        LCD_delay_ms(50);

	spi_24bit_3wire(0x7000B1);  //VSA=50, HAS=64
	spi_24bit_3wire(0x723240);

	spi_24bit_3wire(0x7000B2); //VBP=30+50, HBP=56+64
	spi_24bit_3wire(0x725078);

	spi_24bit_3wire(0x7000B3); //VFP=36, HFP=60
	spi_24bit_3wire(0x72243C);

	spi_24bit_3wire(0x7000B4); //HACT=768
	spi_24bit_3wire(0x720300);

	spi_24bit_3wire(0x7000B5); //VACT=1240
	spi_24bit_3wire(0x720400);

	spi_24bit_3wire(0x7000B6);
	//todo, cfg to 18bpp packed
	spi_24bit_3wire(0x72000B); //0x720009:burst mode, 18bpp packed
														 //0x72000A:burst mode, 18bpp loosely packed
							   						 //0x72000B:burst mode, 24bpp 
							   						 
	spi_24bit_3wire(0x7000DE); //no of lane=4
	spi_24bit_3wire(0x720003);

	spi_24bit_3wire(0x7000D6); //RGB order and packet number in blanking period
	spi_24bit_3wire(0x720005);

	spi_24bit_3wire(0x7000B9); //disable PLL
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x7000BA); //lane speed=560
	spi_24bit_3wire(0x72C015); //may modify according to requirement, 500Mbps to  560Mbps, (n+1)*12M
	
	spi_24bit_3wire(0x7000BB); //LP clock
	spi_24bit_3wire(0x720008);

	spi_24bit_3wire(0x7000B9); //enable PPL
	spi_24bit_3wire(0x720001);

	spi_24bit_3wire(0x7000c4); //enable BTA
	spi_24bit_3wire(0x720001);

	spi_24bit_3wire(0x7000B7); //enter LP mode
	spi_24bit_3wire(0x720342);

	spi_24bit_3wire(0x7000B8); //VC
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x7000BC); //set packet size
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x700011); //sleep out cmd

	LCD_delay_ms(200);
	spi_24bit_3wire(0x700029); //display on

	LCD_delay_ms(200);
	spi_24bit_3wire(0x7000B7); //video mode on
	spi_24bit_3wire(0x72030b);
}

void lp079x01_exit(void)
{
        spi_24bit_3wire(0x7000B7); //enter LP mode
        spi_24bit_3wire(0x720342);
        LCD_delay_ms(50);
        spi_24bit_3wire(0x700028); //display off
        LCD_delay_ms(10);
        spi_24bit_3wire(0x700010); //sleep in cmd
        LCD_delay_ms(20);
        lcd_2828_rst(0);
}

EXPORT_SYMBOL(LCD_get_panel_funs_0);

