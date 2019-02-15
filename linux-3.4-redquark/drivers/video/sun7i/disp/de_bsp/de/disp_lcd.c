#include "disp_lcd.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_de.h"
#include "disp_clk.h"

static __lcd_flow_t         open_flow[2];
static __lcd_flow_t         close_flow[2];
__panel_para_t              gpanel_info[2];
static __lcd_panel_fun_t    lcd_panel_fun[2];
extern __bool hdmi_request_close_flag;

void LCD_get_reg_bases(__reg_bases_t *para)
{
	para->base_lcdc0 = gdisp.init_para.base_lcdc0;
	para->base_lcdc1 = gdisp.init_para.base_lcdc1;
	para->base_pioc = gdisp.init_para.base_pioc;
	para->base_ccmu = gdisp.init_para.base_ccmu;
	para->base_pwm  = gdisp.init_para.base_pwm;
}

void Lcd_Panel_Parameter_Check(__u32 sel)
{
	__panel_para_t* info;
	__u32 cycle_num = 1;
	__u32 Lcd_Panel_Err_Flag = 0;
	__u32 Lcd_Panel_Wrn_Flag = 0;
	__u32 Disp_Driver_Bug_Flag = 0;

	__u32 lcd_fclk_frq;
	__u32 lcd_clk_div;
	
	info = &(gpanel_info[sel]);
	
	if((info->lcd_if== LCD_IF_HV||info->lcd_if== LCD_IF_HV2DSI) && info->lcd_hv_if==1 && info->lcd_hv_smode==0)
		cycle_num = 3;
	else if((info->lcd_if== LCD_IF_HV||info->lcd_if== LCD_IF_HV2DSI) && info->lcd_hv_if==1 && info->lcd_hv_smode==1)
		cycle_num = 2;
	else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==1)
		cycle_num = 3;
	else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==2)
		cycle_num = 2;
	else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==3)
		cycle_num = 2;
	else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==5)
		cycle_num = 2;
	else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==6)
		cycle_num = 3;
	else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==7)
		cycle_num = 2;
	else
		cycle_num = 1;	

	if(info->lcd_hbp > info->lcd_hspw)
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT0;
	}

	if(info->lcd_vbp > info->lcd_vspw)
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT1;
	}

	if(info->lcd_ht >= (info->lcd_hbp+info->lcd_x*cycle_num+4))
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT2;
	}

	if((info->lcd_vt/2) >= (info->lcd_vbp+info->lcd_y+2))
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT3;
	}

	lcd_clk_div = tcon0_get_dclk_div(sel);
	if(lcd_clk_div >= 6)
	{
		;
	}
	else if((lcd_clk_div ==5) || (lcd_clk_div ==4) || (lcd_clk_div ==2))		
	{
		if((info->lcd_io_cfg0 != 0x00000000) && (info->lcd_io_cfg0 != 0x04000000))
		{
			Lcd_Panel_Err_Flag |= BIT10;
		}
	}
	else
	{
		Disp_Driver_Bug_Flag |= 1;	
	}


	if((info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==0)	
	 ||(info->lcd_if== LCD_IF_LVDS && info->lcd_lvds_bitwidth==1))
	{
		if(info->lcd_frm != 1)
			Lcd_Panel_Wrn_Flag |= BIT0;		
	}
	else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==4)
	{
		if(info->lcd_frm != 2)
			Lcd_Panel_Wrn_Flag |= BIT1;					
	}

	lcd_fclk_frq = (info->lcd_dclk_freq * 1000*1000)/((info->lcd_vt/2) * info->lcd_ht);
	if(lcd_fclk_frq<50 || lcd_fclk_frq>70)
	{
		Lcd_Panel_Wrn_Flag |= BIT2;	
	}

	if(Lcd_Panel_Err_Flag != 0 || Lcd_Panel_Wrn_Flag != 0)
	{
		if(Lcd_Panel_Err_Flag != 0)
		{
			__u32 i;
			for(i=0;i<200;i++)
			{
				OSAL_PRINTF("*** Lcd in danger...\n");	
			}
		}
		
		OSAL_PRINTF("*****************************************************************\n");	
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("*** LCD Panel Parameter Check\n");
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("***             by dulianping\n");				
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("*****************************************************************\n");
		
		OSAL_PRINTF("*** \n");	
		OSAL_PRINTF("*** Interface:");
		if((info->lcd_if== LCD_IF_HV||info->lcd_if== LCD_IF_HV2DSI) && info->lcd_hv_if==0)
			{OSAL_PRINTF("*** Parallel HV Panel\n");}
		else if((info->lcd_if== LCD_IF_HV||info->lcd_if== LCD_IF_HV2DSI) && info->lcd_hv_if==1)
			{OSAL_PRINTF("*** Serial HV Panel\n");}
		else if((info->lcd_if== LCD_IF_HV||info->lcd_if== LCD_IF_HV2DSI) && info->lcd_hv_if==2)
			{OSAL_PRINTF("*** Serial YUV Panel\n");}
		else if(info->lcd_if== LCD_IF_LVDS && info->lcd_lvds_bitwidth==0)
			{OSAL_PRINTF("*** 24Bit LVDS Panel\n");}
		else if(info->lcd_if== LCD_IF_LVDS && info->lcd_lvds_bitwidth==1)
			{OSAL_PRINTF("*** 18Bit LVDS Panel\n");}	
		else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==0)
			{OSAL_PRINTF("*** 18Bit CPU Panel\n");}	
		else if(info->lcd_if== LCD_IF_CPU && info->lcd_cpu_if==4)
			{OSAL_PRINTF("*** 16Bit CPU Panel\n");}
		else
		{
			OSAL_PRINTF("\n");
			OSAL_PRINTF("*** lcd_if:     %d\n",info->lcd_if);
			OSAL_PRINTF("*** lcd_hv_if:  %d\n",info->lcd_hv_if);
			OSAL_PRINTF("*** lcd_cpu_if: %d\n",info->lcd_cpu_if);
		}
	
		if(info->lcd_frm==0)
			{OSAL_PRINTF("*** Lcd Frm Disable\n");}
		else if(info->lcd_frm==1)
			{OSAL_PRINTF("*** Lcd Frm to RGB666\n");}
		else if(info->lcd_frm==2)
			{OSAL_PRINTF("*** Lcd Frm to RGB565\n");}	
	
		OSAL_PRINTF("*** \n");
		OSAL_PRINTF("*** Timing:\n");	
		OSAL_PRINTF("*** lcd_x:      %d\n",info->lcd_x);
		OSAL_PRINTF("*** lcd_y:      %d\n",info->lcd_y);
		OSAL_PRINTF("*** lcd_ht:     %d\n",info->lcd_ht);
		OSAL_PRINTF("*** lcd_hbp:    %d\n",info->lcd_hbp);
		OSAL_PRINTF("*** lcd_vt:     %d\n",info->lcd_vt);
		OSAL_PRINTF("*** lcd_vbp:    %d\n",info->lcd_vbp);
		OSAL_PRINTF("*** lcd_hspw:   %d\n",info->lcd_hspw);			
		OSAL_PRINTF("*** lcd_vspw:   %d\n",info->lcd_vspw);		
		OSAL_PRINTF("*** lcd_frame_frq:  %dHz\n",lcd_fclk_frq);	
		
		OSAL_PRINTF("*** \n");	
		if(Lcd_Panel_Err_Flag & BIT0)
			{OSAL_PRINTF("*** Err01: Violate \"lcd_hbp > lcd_hspw\"\n");}
		if(Lcd_Panel_Err_Flag & BIT1)
			{OSAL_PRINTF("*** Err02: Violate \"lcd_vbp > lcd_vspw\"\n");}
		if(Lcd_Panel_Err_Flag & BIT2)
			{OSAL_PRINTF("*** Err03: Violate \"lcd_ht >= (lcd_hbp+lcd_x*%d+4)\"\n", cycle_num);}
		if(Lcd_Panel_Err_Flag & BIT3)								
			{OSAL_PRINTF("*** Err04: Violate \"(lcd_vt/2) >= (lcd_vbp+lcd_y+2)\"\n");}
		if(Lcd_Panel_Err_Flag & BIT10)			
			{OSAL_PRINTF("*** Err10: Violate \"lcd_io_cfg0\",use \"0x00000000\" or \"0x04000000\"");}
		if(Lcd_Panel_Wrn_Flag & BIT0)			
			{OSAL_PRINTF("*** WRN01: Recommend \"lcd_frm = 1\"\n");}
		if(Lcd_Panel_Wrn_Flag & BIT1)
			{OSAL_PRINTF("*** WRN02: Recommend \"lcd_frm = 2\"\n");}		
		if(Lcd_Panel_Wrn_Flag & BIT2)							
			{OSAL_PRINTF("*** WRN03: Recommend \"lcd_dclk_frq = %d\"\n",((info->lcd_vt/2) * info->lcd_ht)*60/(1000*1000));}
		OSAL_PRINTF("*** \n");	

    	        if(Lcd_Panel_Err_Flag != 0)
    	        {	
                        __u32 image_base_addr;
                        __u32 reg_value = 0;

                        image_base_addr = DE_Get_Reg_Base(sel);

                        sys_put_wvalue(image_base_addr+0x804,0xffff00ff);//set background color

                        reg_value = sys_get_wvalue(image_base_addr+0x800);
                        sys_put_wvalue(image_base_addr+0x800,reg_value & 0xfffff0ff);//close all layer

#ifdef __LINUX_OSAL__
                        LCD_delay_ms(2000);
                        sys_put_wvalue(image_base_addr+0x804,0x00000000);//set background color
                        sys_put_wvalue(image_base_addr+0x800,reg_value);//open layer
#endif
                        OSAL_PRINTF("*** Try new parameters,you can make it pass!\n");
    	        }
                OSAL_PRINTF("*** LCD Panel Parameter Check End\n");
                OSAL_PRINTF("*****************************************************************\n");
	}
}

__s32 LCD_parse_panel_para(__u32 sel, __panel_para_t * info)
{
        __s32 ret = 0;
        char primary_key[25];
        __s32 value = 0;
        __u32 i = 0;

        if(!BSP_disp_lcd_used(sel))//no need to get panel para if lcd_used eq 0
        return 0;

        sprintf(primary_key, "lcd%d_para", sel);

        memset(info, 0, sizeof(__panel_para_t));

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_x", &value, 1);
        if(ret == 0)
        {
                info->lcd_x = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_y", &value, 1);
        if(ret == 0)
        {
                info->lcd_y = value;
        }

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_width", &value, 1);
	if(ret == 0)
	{
		info->lcd_width = value;
	}

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_height", &value, 1);
	if(ret == 0)
	{
		info->lcd_height = value;
	}

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dclk_freq", &value, 1);
        if(ret == 0)
        {
                info->lcd_dclk_freq = value;
        }

        //add by heyihang.Jan 27, 2013
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_not_used", &value, 1);
        if(ret == 0)
        {
                info->lcd_pwm_not_used = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_ch", &value, 1);
        if(ret == 0)
        {
                info->lcd_pwm_ch = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_freq", &value, 1);
        if(ret == 0)
        {
                info->lcd_pwm_freq = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_pol", &value, 1);
        if(ret == 0)
        {
                info->lcd_pwm_pol = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_if", &value, 1);
        if(ret == 0)
        {
                info->lcd_if = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hbp", &value, 1);
        if(ret == 0)
        {
                info->lcd_hbp = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_ht", &value, 1);
        if(ret == 0)
        {
                info->lcd_ht = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vbp", &value, 1);
        if(ret == 0)
        {
                info->lcd_vbp = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vt", &value, 1);
        if(ret == 0)
        {
                info->lcd_vt = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vspw", &value, 1);
        if(ret == 0)
        {
                info->lcd_vspw = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hspw", &value, 1);
        if(ret == 0)
        {
                info->lcd_hspw = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_if", &value, 1);
        if(ret == 0)
        {
                info->lcd_hv_if = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_smode", &value, 1);
        if(ret == 0)
        {
                info->lcd_hv_smode = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_s888_if", &value, 1);
        if(ret == 0)
        {
                info->lcd_hv_s888_if = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_syuv_if", &value, 1);
        if(ret == 0)
        {
                info->lcd_hv_syuv_if = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_ch", &value, 1);
        if(ret == 0)
        {
                info->lcd_lvds_ch = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_mode", &value, 1);
        if(ret == 0)
        {
                info->lcd_lvds_mode = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_bitwidth", &value, 1);
        if(ret == 0)
        {
                info->lcd_lvds_bitwidth = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_io_cross", &value, 1);
        if(ret == 0)
        {
                info->lcd_lvds_io_cross = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_cpu_if", &value, 1);
        if(ret == 0)
        {
                info->lcd_cpu_if = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_frm", &value, 1);
        if(ret == 0)
        {
                info->lcd_frm = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_io_cfg0", &value, 1);
        if(ret == 0)
        {
                info->lcd_io_cfg0 = value;
        }

        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_gamma_correction_en", &value, 1);
        if(ret == 0)
        {
                info->lcd_gamma_correction_en = value;
        }

        if(info->lcd_gamma_correction_en)
        {
                for(i=0; i<256; i++)
                {
                        char name[20];
            
                        sprintf(name, "lcd_gamma_tbl_%d", i);
            
                        ret = OSAL_Script_FetchParser_Data(primary_key, name, &value, 1);
                        if(ret < 0)
                        {
                                info->lcd_gamma_tbl[i] = (i<<16) | (i<<8) | i;
                        }
                        else
                        {
                                info->lcd_gamma_tbl[i] = value;
                        }
                }
        }
        return 0;
}

void LCD_get_sys_config(__u32 sel, __disp_lcd_cfg_t *lcd_cfg)
{
        static char io_name[28][20] = {"lcdd0", "lcdd1", "lcdd2", "lcdd3", "lcdd4", "lcdd5", "lcdd6", "lcdd7", "lcdd8", "lcdd9", "lcdd10", "lcdd11", 
                         "lcdd12", "lcdd13", "lcdd14", "lcdd15", "lcdd16", "lcdd17", "lcdd18", "lcdd19", "lcdd20", "lcdd21", "lcdd22",
                         "lcdd23", "lcdclk", "lcdde", "lcdhsync", "lcdvsync"};
        disp_gpio_set_t  *gpio_info;
        int  value = 1;
        char primary_key[20], sub_name[25];
        int i = 0;
        int  ret;

        sprintf(primary_key, "lcd%d_para", sel);

        //lcd_used
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_used", &value, 1);
        if(ret == 0)
        {
                lcd_cfg->lcd_used = value;
        }

        if(lcd_cfg->lcd_used == 0) //no need to get lcd config if lcd_used eq 0
        return ;

        //lcd_bl_en
        lcd_cfg->lcd_bl_en_used = 0;
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_bl_en_used", &value, 1);
        if (ret < 0)
        {
                __wrn("fetch [%s] lcd_bl_en_used fail!! \n", primary_key);
        }
        else if (value == 1)
        {
                gpio_info = &(lcd_cfg->lcd_bl_en);
                ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_bl_en", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
                if(ret == 0)
                {
                        lcd_cfg->lcd_bl_en_used = 1;
                }
        }

        //lcd_power
        lcd_cfg->lcd_power_used= 0;
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_power_used", &value, 1);
        if (ret < 0)
        {
                __wrn("fetch [%s] lcd_power_used fail!! \n", primary_key);
        }
        else if (value == 1)
        {
                gpio_info = &(lcd_cfg->lcd_power);
                ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_power", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
                if(ret == 0)
                {
                        lcd_cfg->lcd_power_used= 1;
                }
        }

        //lcd_pwm
        lcd_cfg->lcd_pwm_used= 0;
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_used", &value, 1);
        if (ret < 0)
        {
                __wrn("fetch [%s] lcd_pwm_used fail!! \n", primary_key);
        }
        else if (value == 1)
        {
                gpio_info = &(lcd_cfg->lcd_pwm);
                ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_pwm", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
                if(ret == 0)
                {
                       lcd_cfg->lcd_pwm_used= 1;
                }
        }

        //lcd_gpio
        for(i=0; i<6; i++)
        {
                sprintf(sub_name, "lcd_gpio_%d", i);

                gpio_info = &(lcd_cfg->lcd_gpio[i]);
                ret = OSAL_Script_FetchParser_Data(primary_key,sub_name, (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
                if(ret == 0)
                {
                        lcd_cfg->lcd_gpio_used[i]= 1;
                }
        }

        //lcd io
        for(i=0; i<28; i++)
        {
                gpio_info = &(lcd_cfg->lcd_io[i]);
                ret = OSAL_Script_FetchParser_Data(primary_key,io_name[i], (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
                if(ret == 0)
                {
                        lcd_cfg->lcd_io_used[i]= 1;
                }
        }

        lcd_cfg->backlight_max_limit = 150;
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_max_limit", &value, 1);
        if(ret == 0)
        {
                lcd_cfg->backlight_max_limit = (value > 255)? 255:value;
        }

        //init_bright
        sprintf(primary_key, "disp_init");
        sprintf(sub_name, "lcd%d_backlight", sel);

        ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
        if(ret < 0)
        {
                lcd_cfg->backlight_bright = 197;
        }
        else
        {
                if(value > 256)
                {
                        value = 256;
                }
                lcd_cfg->backlight_bright = value;
        }
        //add by heyihang.Jan 28, 2013
        gdisp.screen[sel].lcd_bright = lcd_cfg->backlight_bright;

        //bright,constraction,saturation,hue
        sprintf(primary_key, "disp_init");
        sprintf(sub_name, "lcd%d_bright", sel);
        ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
        if(ret < 0)
        {
                lcd_cfg->lcd_bright = 50;
        }
        else
        {
                if(value > 100)
                {
                        value = 100;
                }
                lcd_cfg->lcd_bright = value;
        }

        sprintf(sub_name, "lcd%d_contrast", sel);
        ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
        if(ret < 0)
        {
                lcd_cfg->lcd_contrast = 50;
        }
        else
        {
                if(value > 100)
                {
                        value = 100;
                }
                lcd_cfg->lcd_contrast = value;
        }

        sprintf(sub_name, "lcd%d_saturation", sel);
        ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
        if(ret < 0)
        {
                lcd_cfg->lcd_saturation = 50;
        }
        else
        {
                if(value > 100)
                {
                        value = 100;
                }
                lcd_cfg->lcd_saturation = value;
        }

        sprintf(sub_name, "lcd%d_hue", sel);
        ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
        if(ret < 0)
        {
                lcd_cfg->lcd_hue = 50;
        }
        else
        {
                if(value > 100)
                {
                        value = 100;
                }
                lcd_cfg->lcd_hue = value;
        }
}

void LCD_delay_ms(__u32 ms) 
{
#ifdef __LINUX_OSAL__
        __u32 timeout = ms*HZ/1000;
    
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(timeout);
#endif
#ifdef __BOOT_OSAL__
        wBoot_timer_delay(ms);//assume cpu runs at 1000Mhz,10 clock one cycle
#endif
}


void LCD_delay_us(__u32 us) 
{
#ifdef __LINUX_OSAL__
        udelay(us);
#endif
#ifdef __BOOT_OSAL__
        volatile __u32 time;
    
        for(time = 0; time < (us*700/10);time++);//assume cpu runs at 700Mhz,10 clock one cycle
#endif
}

void LCD_OPEN_FUNC(__u32 sel, LCD_FUNC func, __u32 delay)
{
        open_flow[sel].func[open_flow[sel].func_num].func = func;
        open_flow[sel].func[open_flow[sel].func_num].delay = delay;
        open_flow[sel].func_num++;
}


void LCD_CLOSE_FUNC(__u32 sel, LCD_FUNC func, __u32 delay)
{
        close_flow[sel].func[close_flow[sel].func_num].func = func;
        close_flow[sel].func[close_flow[sel].func_num].delay = delay;
        close_flow[sel].func_num++;
}

void TCON_open(__u32 sel)
{    
        if(gpanel_info[sel].tcon_index == 0)
        {
                tcon0_open(sel);
                gdisp.screen[sel].lcdc_status |= LCDC_TCON0_USED;
        }
        else
        {
                tcon1_open(sel);
                gdisp.screen[sel].lcdc_status |= LCDC_TCON1_USED;
        }

        if(gpanel_info[sel].lcd_if == LCD_IF_LVDS)
        {
                LCD_LVDS_open(sel);
        }
}

void TCON_close(__u32 sel)
{    
        if(gpanel_info[sel].lcd_if == LCD_IF_LVDS)
        {
                LCD_LVDS_close(sel);
        }

        if(gpanel_info[sel].tcon_index == 0)
        {
                tcon0_close(sel);
                gdisp.screen[sel].lcdc_status &= LCDC_TCON0_USED_MASK;
        }
        else
        {
                tcon1_close(sel);
                gdisp.screen[sel].lcdc_status &= LCDC_TCON1_USED_MASK;
        }
}


static __u32 pwm_read_reg(__u32 offset)
{
        __u32 value = 0;

        value = sys_get_wvalue(gdisp.init_para.base_pwm+offset);

        return value;
}

static __s32 pwm_write_reg(__u32 offset, __u32 value)
{
        sys_put_wvalue(gdisp.init_para.base_pwm+offset, value);

        return 0;    
}

__s32 pwm_enable(__u32 channel, __bool b_en)
{
        __u32 tmp = 0;
        __hdle hdl;

        if(gdisp.screen[channel].lcd_cfg.lcd_pwm_used)
        {
                disp_gpio_set_t  gpio_info[1];

                memcpy(gpio_info, &(gdisp.screen[channel].lcd_cfg.lcd_pwm), sizeof(disp_gpio_set_t));

                if(b_en)
                {
                        gpio_info->mul_sel = 2;
                }
                else
                {            
                        gpio_info->mul_sel = 0;
                }
                hdl = OSAL_GPIO_Request(gpio_info, 1);
                OSAL_GPIO_Release(hdl, 2);
        }

        if(channel == 0)
        {
                tmp = pwm_read_reg(0x200);
                if(b_en)
                {
                        tmp |= (1<<4);
                }
                else
                {
                        tmp &= (~(1<<4));
                }
                pwm_write_reg(0x200,tmp);
        }
        else
        {
                tmp = pwm_read_reg(0x200);
                if(b_en)
                {
                        tmp |= (1<<19);
                }
                else
                {
                        tmp &= (~(1<<19));
                }
                pwm_write_reg(0x200,tmp);
        }

        gdisp.pwm[channel].enable = b_en;

        return 0;
}


//channel: pwm channel,0/1
//pwm_info->freq:  pwm freq, in hz
//pwm_info->active_state: 0:low level; 1:high level
__s32 pwm_set_para(__u32 channel, __pwm_info_t * pwm_info)
{
        __u32 pre_scal[11][2] = {{1,0xf}, {120,0}, {180,1}, {240,2}, {360,3}, {480,4}, {12000,8}, {24000,9}, {36000,0xa}, {48000,0xb}, {72000,0xc}};
        __u32 pre_scal_id = 0, entire_cycle = 16, active_cycle = 12;
        __u32 i=0, j=0, tmp=0;
        __u32 freq;

        //modified by heyihang.Jan 27, 2013
        freq = 1000000000 / pwm_info->period_ns;

        if(freq > 366)
        {
                pre_scal_id = 0;
                entire_cycle = 24000000 / freq;
        }
        else
        {
                for(i=1; i<11; i++)
        	{
        	        for(j=16;; j+=16)
        	        {
        	                __u32 pwm_freq = 0;

        	                pwm_freq = 24000000 / (pre_scal[i][0] * j);
        	                if(abs(pwm_freq - freq) < abs(tmp - freq))
        	                {
        	                        tmp = pwm_freq;
        	                        pre_scal_id = i;
        	                        entire_cycle = j;
        	                        DE_INF("pre_scal:%d, entire_cycle:%d, pwm_freq:%d\n", pre_scal[i][0], j, pwm_freq);
        	                        DE_INF("----%d\n", tmp);
        	                }
        	                else if((tmp < freq) && (pwm_freq < tmp))
        	                {
        	                        break;
        	                }
        	        }
                }
        }

        active_cycle = (pwm_info->duty_ns * entire_cycle + (pwm_info->period_ns/2)) / pwm_info->period_ns;

        gdisp.pwm[channel].enable = pwm_info->enable;
        gdisp.pwm[channel].freq = freq;
        gdisp.pwm[channel].pre_scal = pre_scal[pre_scal_id][0];
        gdisp.pwm[channel].active_state = pwm_info->active_state;
        gdisp.pwm[channel].duty_ns = pwm_info->duty_ns;
        gdisp.pwm[channel].period_ns = pwm_info->period_ns;
        gdisp.pwm[channel].entire_cycle = entire_cycle;
        gdisp.pwm[channel].active_cycle = active_cycle;


        if(channel == 0)
        {
                pwm_write_reg(0x204, ((entire_cycle - 1)<< 16) | active_cycle);

                tmp = pwm_read_reg(0x200) & 0xffffff00;
                tmp |= ((1<<6) | (pwm_info->active_state<<5) | pre_scal[pre_scal_id][1]);//bit6:gatting the special clock for pwm0; bit5:pwm0  active state is high level
                pwm_write_reg(0x200,tmp);
        }
        else
        {
                pwm_write_reg(0x208, ((entire_cycle - 1)<< 16) | active_cycle);

                tmp = pwm_read_reg(0x200) & 0xff807fff;
                tmp |= ((1<<21) | (pwm_info->active_state<<20) | (pre_scal[pre_scal_id][1]<<15));//bit21:gatting the special clock for pwm1; bit20:pwm1  active state is high level
                pwm_write_reg(0x200,tmp);
        }

        pwm_enable(channel, pwm_info->enable);

        return 0;
}

__s32 pwm_get_para(__u32 channel, __pwm_info_t * pwm_info)
{
        pwm_info->enable = gdisp.pwm[channel].enable;
        pwm_info->active_state = gdisp.pwm[channel].active_state;
        pwm_info->duty_ns = gdisp.pwm[channel].duty_ns;
        pwm_info->period_ns = gdisp.pwm[channel].period_ns;

        return 0;
}

__s32 pwm_set_duty_ns(__u32 channel, __u32 duty_ns)
{	    
        __u32 active_cycle = 0;
        __u32 tmp;

        active_cycle = (duty_ns * gdisp.pwm[channel].entire_cycle + (gdisp.pwm[channel].period_ns/2)) / gdisp.pwm[channel].period_ns;

        if(channel == 0)
        {
                tmp = pwm_read_reg(0x204);
                pwm_write_reg(0x204,(tmp & 0xffff0000) | active_cycle);
        }
        else
        {
                tmp = pwm_read_reg(0x208);
                pwm_write_reg(0x208,(tmp & 0xffff0000) | active_cycle);
        }

        gdisp.pwm[channel].duty_ns = duty_ns;

        DE_INF("%d,%d,%d,%d\n", duty_ns, gdisp.pwm[channel].period_ns, active_cycle, gdisp.pwm[channel].entire_cycle);
        return 0;
}

__s32 LCD_PWM_EN(__u32 sel, __bool b_en)
{
        if(gdisp.screen[sel].lcd_cfg.lcd_pwm_used)
        {
                disp_gpio_set_t  gpio_info[1];
                __hdle hdl;

                memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_pwm), sizeof(disp_gpio_set_t));

                if(b_en)
                {
                        pwm_enable(gpanel_info[sel].lcd_pwm_ch, b_en);
                }
                else
                {
                        gpio_info->mul_sel = 0;
                        hdl = OSAL_GPIO_Request(gpio_info, 1);
                        OSAL_GPIO_Release(hdl, 2);
                }
        }
        return 0;
}

__s32 LCD_BL_EN(__u32 sel, __bool b_en)
{
        disp_gpio_set_t  gpio_info[1];
        __hdle hdl;

        if(gdisp.screen[sel].lcd_cfg.lcd_bl_en_used)
        {
                memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_bl_en), sizeof(disp_gpio_set_t));

                if(!b_en)
                {
                        gpio_info->data = (gpio_info->data==0)?1:0;
                }

                hdl = OSAL_GPIO_Request(gpio_info, 1);
                OSAL_GPIO_Release(hdl, 2);
        }

        return 0;
}

__s32 LCD_POWER_EN(__u32 sel, __bool b_en)
{
        disp_gpio_set_t  gpio_info[1];
        __hdle hdl;

        if(gdisp.screen[sel].lcd_cfg.lcd_power_used)
        {
                memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_power), sizeof(disp_gpio_set_t));
                if(!b_en)
                {
                        gpio_info->data = (gpio_info->data==0)?1:0;
                }

                hdl = OSAL_GPIO_Request(gpio_info, 1);
                OSAL_GPIO_Release(hdl, 2);
        }
        return 0;
}


__s32 LCD_GPIO_request(__u32 sel, __u32 io_index)
{
        return 0;
}

__s32 LCD_GPIO_release(__u32 sel,__u32 io_index)
{
        return 0;
}

__s32 LCD_GPIO_set_attr(__u32 sel,__u32 io_index, __bool b_output)
{
        char gpio_name[20];

        sprintf(gpio_name, "lcd_gpio_%d", io_index);
        return  OSAL_GPIO_DevSetONEPIN_IO_STATUS(gdisp.screen[sel].gpio_hdl[io_index], b_output, gpio_name);
}

__s32 LCD_GPIO_read(__u32 sel,__u32 io_index)
{
        char gpio_name[20];

        sprintf(gpio_name, "lcd_gpio_%d", io_index);
        return OSAL_GPIO_DevREAD_ONEPIN_DATA(gdisp.screen[sel].gpio_hdl[io_index], gpio_name);
}

__s32 LCD_GPIO_write(__u32 sel,__u32 io_index, __u32 data)
{
        char gpio_name[20];

        sprintf(gpio_name, "lcd_gpio_%d", io_index);
        return OSAL_GPIO_DevWRITE_ONEPIN_DATA(gdisp.screen[sel].gpio_hdl[io_index], data, gpio_name);
}

__s32 LCD_GPIO_init(__u32 sel)
{
        __u32 i = 0;

        for(i=0; i<6; i++)
        {
                gdisp.screen[sel].gpio_hdl[i] = 0;
                if(gdisp.screen[sel].lcd_cfg.lcd_gpio_used[i])
                {
                        disp_gpio_set_t  gpio_info[1]; 
                        memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_gpio[i]), sizeof(disp_gpio_set_t));
                        gdisp.screen[sel].gpio_hdl[i] = OSAL_GPIO_Request(gpio_info, 1);
                }
        }
        return 0;
}

__s32 LCD_GPIO_exit(__u32 sel)
{
        __u32 i = 0;

        for(i=0; i<6; i++)
        {
                if(gdisp.screen[sel].gpio_hdl[i])
                {
                        OSAL_GPIO_Release(gdisp.screen[sel].gpio_hdl[i], 2);
                }
        }
        for(i=0; i<6; i++)
        {
                gdisp.screen[sel].gpio_hdl[i] = 0;
                if(gdisp.screen[sel].lcd_cfg.lcd_gpio_used[i])
                {
                        disp_gpio_set_t  gpio_info[1];
                        memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_gpio[i]), sizeof(disp_gpio_set_t));
                        gpio_info->mul_sel = 0;
                        gdisp.screen[sel].gpio_hdl[i] = OSAL_GPIO_Request(gpio_info, 1);
                        OSAL_GPIO_Release(gdisp.screen[sel].gpio_hdl[i], 2);
                }
        }
        return 0;
}

void LCD_CPU_register_irq(__u32 sel, void (*Lcd_cpuisr_proc) (void))
{
        gdisp.screen[sel].LCD_CPUIF_ISR = Lcd_cpuisr_proc;
}

__s32 Disp_lcdc_pin_cfg(__u32 sel, __disp_output_type_t out_type, __u32 bon)
{   
        if(out_type == DISP_OUTPUT_TYPE_LCD)
        {
                __hdle lcd_pin_hdl;
                int  i;

                for(i=0; i<28; i++)
                {
                        if(gdisp.screen[sel].lcd_cfg.lcd_io_used[i])
                        {
                                disp_gpio_set_t  gpio_info[1];
                
                                memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_io[i]), sizeof(disp_gpio_set_t));
                                if(!bon)
                                {
                                        gpio_info->mul_sel = 0;


                                }
                                else
                                {
                                        if((gpanel_info[sel].lcd_if == LCD_IF_LVDS) && (gpio_info->mul_sel==2))
                                        {
                                                gpio_info->mul_sel = 3;
                                        }

                                }
                                lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
                                OSAL_GPIO_Release(lcd_pin_hdl, 2);
                        }
                }
        }
        else if(out_type == DISP_OUTPUT_TYPE_VGA)
        {
                __u32 reg_start = 0;
                __u32 tmp = 0;

                if(sel == 0)
                {
                        reg_start = gdisp.init_para.base_pioc+0x6c;
                }
                else
                {
                        reg_start = gdisp.init_para.base_pioc+0xfc;
                }

                if(bon)
                {
                        tmp = sys_get_wvalue(reg_start + 0x0c) & 0xffff00ff;
                        sys_put_wvalue(reg_start + 0x0c,tmp | 0x00002200);
                }
                else
                {
                        tmp = sys_get_wvalue(reg_start + 0x0c) & 0xffff00ff;
                        sys_put_wvalue(reg_start + 0x0c,tmp);
                }
        }

        return DIS_SUCCESS;
}


#ifdef __LINUX_OSAL__
__s32 Disp_lcdc_event_proc(int irq, void *parg)
#else
__s32 Disp_lcdc_event_proc(void *parg)
#endif
{
        __u32  lcdc_flags;
        __u32 sel = (__u32)parg;
				if (hdmi_request_close_flag){
					if(gdisp.init_para.hdmi_close)
						gdisp.init_para.hdmi_close();
					else DE_WRN("hdmi_close is NULL\n");
				hdmi_request_close_flag = 0;
				DE_INF("###%s---hdmi_request_close_flag = 1",__func__);
				}

        lcdc_flags=LCDC_query_int(sel);  
        LCDC_clear_int(sel,lcdc_flags);
        if(lcdc_flags & LCDC_VBI_LCD)
        {
                LCD_vbi_event_proc(sel, 0);
        }
        if(lcdc_flags & LCDC_VBI_HD)
        {
                LCD_vbi_event_proc(sel, 1);
        }
        return OSAL_IRQ_RETURN;
}

__s32 Disp_lcdc_init(__u32 sel)
{
        LCD_get_sys_config(sel, &(gdisp.screen[sel].lcd_cfg));
        gdisp.screen[sel].lcd_cfg.backlight_dimming = 256;

        if(gdisp.screen[sel].lcd_cfg.lcd_used)
        {
                if(lcd_panel_fun[sel].cfg_panel_info)
                {
                        lcd_panel_fun[sel].cfg_panel_info(&gpanel_info[sel]);
                }
                else
                {
                        LCD_parse_panel_para(sel, &gpanel_info[sel]);
                }
                gpanel_info[sel].tcon_index = 0;
        }

        lcdc_clk_init(sel);
        if(gpanel_info[sel].lcd_if == LCD_IF_LVDS)
        {
                lvds_clk_init();
        }

        lcdc_clk_on(sel, 0, 0);
        lcdc_clk_on(sel, 0, 1);
        LCDC_init(sel);
        lcdc_clk_off(sel);

        if(gdisp.screen[sel].lcd_cfg.lcd_used)
        {
                //modified by heyihang.Jan 26, 2013
                if(gpanel_info[sel].lcd_pwm_not_used == 0)
                {
                        __pwm_info_t pwm_info;

                        pwm_info.enable = 0;
                        pwm_info.active_state = 1;
                        if(gpanel_info[sel].lcd_pwm_freq != 0)
                        {
                                pwm_info.period_ns = 1000000000 / gpanel_info[sel].lcd_pwm_freq;
                        }
                        else
                        {
                                DE_WRN("lcd%d.lcd_pwm_freq is ZERO\n", sel);
                                pwm_info.period_ns = 1000000000 / 1000;  //default 1khz
                        } 
                        if(gpanel_info[sel].lcd_pwm_pol == 0)
                        {
                                pwm_info.duty_ns = (gdisp.screen[sel].lcd_cfg.backlight_bright * pwm_info.period_ns) / 256;
                        }
                        else
                        {
                                pwm_info.duty_ns = ((256 - gdisp.screen[sel].lcd_cfg.backlight_bright) * pwm_info.period_ns) / 256;
                        }
                        __inf("pwm_info.period_ns: %d, pwm_info.duty_ns: %d \n", pwm_info.period_ns, pwm_info.duty_ns);
                        pwm_set_para(gdisp.screen[sel].lcd_cfg.lcd_pwm_ch, &pwm_info);
                }
        }
        return DIS_SUCCESS;
}


__s32 Disp_lcdc_exit(__u32 sel)
{	
        if(sel == 0)
        {
                OSAL_InterruptDisable(INTC_IRQNO_LCDC0);
                OSAL_UnRegISR(INTC_IRQNO_LCDC0,Disp_lcdc_event_proc,(void*)sel);
        }
        else if(sel == 1)
        {
                OSAL_InterruptDisable(INTC_IRQNO_LCDC1);
                OSAL_UnRegISR(INTC_IRQNO_LCDC1,Disp_lcdc_event_proc,(void*)sel);
        }
        lcdc_clk_exit(sel);

        return DIS_SUCCESS;
}

__s32 Disp_lcdc_reg_isr(__u32 sel)
{
        if(sel == 0)
        {
                OSAL_RegISR(INTC_IRQNO_LCDC0,0,Disp_lcdc_event_proc,(void*)sel,0,0);
#ifndef __LINUX_OSAL__
                OSAL_InterruptEnable(INTC_IRQNO_LCDC0);
#endif
        }
        else
        {
                OSAL_RegISR(INTC_IRQNO_LCDC1,0,Disp_lcdc_event_proc,(void*)sel,0,0);
#ifndef __LINUX_OSAL__
                OSAL_InterruptEnable(INTC_IRQNO_LCDC1);
#endif
        }
        return DIS_SUCCESS;
}
__u32 tv_mode_to_width(__disp_tv_mode_t mode)
{
        __u32 width = 0;

        switch(mode)
        {
                case DISP_TV_MOD_480I:
                case DISP_TV_MOD_576I:
                case DISP_TV_MOD_480P:
                case DISP_TV_MOD_576P:
                case DISP_TV_MOD_PAL:
                case DISP_TV_MOD_NTSC:
                case DISP_TV_MOD_PAL_SVIDEO:
                case DISP_TV_MOD_NTSC_SVIDEO:
                case DISP_TV_MOD_PAL_M:
                case DISP_TV_MOD_PAL_M_SVIDEO:
                case DISP_TV_MOD_PAL_NC:
                case DISP_TV_MOD_PAL_NC_SVIDEO:
                        width = 720;
                        break;
                case DISP_TV_MOD_720P_50HZ:
                case DISP_TV_MOD_720P_60HZ:
                case DISP_TV_MOD_720P_50HZ_3D_FP:
                case DISP_TV_MOD_720P_60HZ_3D_FP:
                        width = 1280;
                        break;
                case DISP_TV_MOD_1080I_50HZ:
                case DISP_TV_MOD_1080I_60HZ:
                case DISP_TV_MOD_1080P_24HZ:
                case DISP_TV_MOD_1080P_50HZ:
                case DISP_TV_MOD_1080P_60HZ:
                case DISP_TV_MOD_1080P_24HZ_3D_FP:
                case DISP_TV_MOD_1080P_25HZ:
                case DISP_TV_MOD_1080P_30HZ:
                        width = 1920;
                        break;
                default:
                        width = 0;
                        break;
        }

        return width;
}


__u32 tv_mode_to_height(__disp_tv_mode_t mode)
{
        __u32 height = 0;

        switch(mode)
        {
                case DISP_TV_MOD_480I:
                case DISP_TV_MOD_480P:
                case DISP_TV_MOD_NTSC:
                case DISP_TV_MOD_NTSC_SVIDEO:
                case DISP_TV_MOD_PAL_M:
                case DISP_TV_MOD_PAL_M_SVIDEO:
                        height = 480;
                        break;
                case DISP_TV_MOD_576I:
                case DISP_TV_MOD_576P:
                case DISP_TV_MOD_PAL:
                case DISP_TV_MOD_PAL_SVIDEO:
                case DISP_TV_MOD_PAL_NC:
                case DISP_TV_MOD_PAL_NC_SVIDEO:
                        height = 576;
                        break;
                case DISP_TV_MOD_720P_50HZ:
                case DISP_TV_MOD_720P_60HZ:
                        height = 720;
                        break;
                case DISP_TV_MOD_1080I_50HZ:
                case DISP_TV_MOD_1080I_60HZ:
                case DISP_TV_MOD_1080P_24HZ:
                case DISP_TV_MOD_1080P_50HZ:
                case DISP_TV_MOD_1080P_60HZ:
                case DISP_TV_MOD_1080P_25HZ:
                case DISP_TV_MOD_1080P_30HZ:
                        height = 1080;
                        break;
                case DISP_TV_MOD_1080P_24HZ_3D_FP:
                        height = 1080*2;
                        break;
                case DISP_TV_MOD_720P_50HZ_3D_FP:
                case DISP_TV_MOD_720P_60HZ_3D_FP:
                        height = 720*2;
                        break;
                default:
                        height = 0;
                        break;
        }

        return height;
}

__u32 vga_mode_to_width(__disp_vga_mode_t mode)
{
        __u32 width = 0;

        switch(mode)
        {
        	case DISP_VGA_H1680_V1050:
        		width = 1680;
                        break;
        	case DISP_VGA_H1440_V900:
        		width = 1440;
                        break;
        	case DISP_VGA_H1360_V768:
        		width = 1360;
                        break;
        	case DISP_VGA_H1280_V1024:
        		width = 1280;
                        break;
        	case DISP_VGA_H1024_V768:
        		width = 1024;
                        break;
        	case DISP_VGA_H800_V600:
        		width = 800;
                        break;
        	case DISP_VGA_H640_V480:
        		width = 640;
                        break;
        	case DISP_VGA_H1440_V900_RB:
        		width = 1440;
                        break;
        	case DISP_VGA_H1680_V1050_RB:
        		width = 1680;
                        break;
        	case DISP_VGA_H1920_V1080_RB:
        	case DISP_VGA_H1920_V1080:
        		width = 1920;
                        break;
                case DISP_VGA_H1280_V720:
                        width = 1280;
                        break;
        	default:
        		width = 0;
                        break;
        }

        return width;
}


__u32 vga_mode_to_height(__disp_vga_mode_t mode)
{
        __u32 height = 0;

        switch(mode)
        {
                case DISP_VGA_H1680_V1050:
                        height = 1050;
                        break;
                case DISP_VGA_H1440_V900:
                        height = 900;
                        break;
                case DISP_VGA_H1360_V768:
                        height = 768;
                        break;
                case DISP_VGA_H1280_V1024:
                        height = 1024;
                        break;
                case DISP_VGA_H1024_V768:
                        height = 768;
                        break;
                case DISP_VGA_H800_V600:
                        height = 600;
                        break;
                case DISP_VGA_H640_V480:
                        height = 480;
                        break;
                case DISP_VGA_H1440_V900_RB:
                        height = 1440;
                        break;
                case DISP_VGA_H1680_V1050_RB:
                        height = 1050;
                        break;
                case DISP_VGA_H1920_V1080_RB:
                case DISP_VGA_H1920_V1080:
                        height = 1080;
                        break;
                case DISP_VGA_H1280_V720:
                        height = 720;
                        break;
                default:
                        height = 0;
                        break;
        }
        
        return height;
}

// return 0: progressive scan mode; return 1: interlace scan mode
__u32 Disp_get_screen_scan_mode(__disp_tv_mode_t tv_mode)
{
	__u32 ret = 0;
	
	switch(tv_mode)
	{
		case DISP_TV_MOD_480I:
		case DISP_TV_MOD_NTSC:
		case DISP_TV_MOD_NTSC_SVIDEO:
		case DISP_TV_MOD_PAL_M:
		case DISP_TV_MOD_PAL_M_SVIDEO:
		case DISP_TV_MOD_576I:
		case DISP_TV_MOD_PAL:
		case DISP_TV_MOD_PAL_SVIDEO:
		case DISP_TV_MOD_PAL_NC:
		case DISP_TV_MOD_PAL_NC_SVIDEO:
		case DISP_TV_MOD_1080I_50HZ:	
		case DISP_TV_MOD_1080I_60HZ:
		        ret = 1;
                        break;
		default:
		        break;
	}
        
	return ret;
}

__s32 BSP_disp_get_screen_width(__u32 sel)
{    
        __u32 width = 0;

        if((gdisp.screen[sel].status & LCD_ON) || (gdisp.screen[sel].status & TV_ON) || (gdisp.screen[sel].status & HDMI_ON) || (gdisp.screen[sel].status & VGA_ON))
        {
                width = DE_BE_get_display_width(sel);
        }
        else
        {
                width = gpanel_info[sel].lcd_x;
        }

        return width;
}
 
__s32 BSP_disp_get_screen_height(__u32 sel)
{    
        __u32 height = 0;

        if((gdisp.screen[sel].status & LCD_ON) || (gdisp.screen[sel].status & TV_ON) || (gdisp.screen[sel].status & HDMI_ON) || (gdisp.screen[sel].status & VGA_ON))
        {
                height = DE_BE_get_display_height(sel);
        }
        else
        {
                height = gpanel_info[sel].lcd_y;
        }

        return height;
}
//get width of screen in mm
__s32 BSP_disp_get_screen_physical_width(__u32 sel)
{
	__u32 width = 0;

	if((gdisp.screen[sel].status & TV_ON) || (gdisp.screen[sel].status & HDMI_ON) || (gdisp.screen[sel].status & VGA_ON))
	{
		width = 0;
	}
	else
	{
		width = gpanel_info[sel].lcd_width;//width of lcd in mm
	}

	return width;
}

//get height of screen in mm
__s32 BSP_disp_get_screen_physical_height(__u32 sel)
{
	__u32 height = 0;

	if((gdisp.screen[sel].status & TV_ON) || (gdisp.screen[sel].status & HDMI_ON) || (gdisp.screen[sel].status & VGA_ON))
	{
		height = 0;
	}
	else
	{
		height = gpanel_info[sel].lcd_height;//height of lcd in mm
	}

	return height;
}

__s32 BSP_disp_get_output_type(__u32 sel)
{
	if(gdisp.screen[sel].status & TV_ON)
	{
	        return (__s32)DISP_OUTPUT_TYPE_TV;
	}

	if(gdisp.screen[sel].status & LCD_ON)
	{
		return (__s32)DISP_OUTPUT_TYPE_LCD;
	}

	if(gdisp.screen[sel].status & HDMI_ON)
	{
		return (__s32)DISP_OUTPUT_TYPE_HDMI;
	}

	if(gdisp.screen[sel].status & VGA_ON)
	{
		return (__s32)DISP_OUTPUT_TYPE_VGA;
	}

	return (__s32)DISP_OUTPUT_TYPE_NONE;
}


__s32 BSP_disp_get_frame_rate(__u32 sel)
{
        __s32 frame_rate = 60;

        if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_LCD)
        {
                frame_rate = (gpanel_info[sel].lcd_dclk_freq * 1000000) / (gpanel_info[sel].lcd_ht * (gpanel_info[sel].lcd_vt / 2)) ;
        }
        else if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_TV)
        {
                switch(gdisp.screen[sel].tv_mode)
                {
                    case DISP_TV_MOD_480I:
                    case DISP_TV_MOD_480P:
                    case DISP_TV_MOD_NTSC:
                    case DISP_TV_MOD_NTSC_SVIDEO:
                    case DISP_TV_MOD_PAL_M:
                    case DISP_TV_MOD_PAL_M_SVIDEO:
                    case DISP_TV_MOD_720P_60HZ:
                    case DISP_TV_MOD_1080I_60HZ:
                    case DISP_TV_MOD_1080P_60HZ:
                                frame_rate = 60;
                                break;
                    case DISP_TV_MOD_576I:
                    case DISP_TV_MOD_576P:
                    case DISP_TV_MOD_PAL:
                    case DISP_TV_MOD_PAL_SVIDEO:
                    case DISP_TV_MOD_PAL_NC:
                    case DISP_TV_MOD_PAL_NC_SVIDEO:
                    case DISP_TV_MOD_720P_50HZ:
                    case DISP_TV_MOD_1080I_50HZ:
                    case DISP_TV_MOD_1080P_50HZ:
                                frame_rate = 50;
                                break;
                    default:
                                break;
                }
        }
        else if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_HDMI)
        {
                switch(gdisp.screen[sel].hdmi_mode)
                {
                    case DISP_TV_MOD_480I:
                    case DISP_TV_MOD_480P:
                    case DISP_TV_MOD_720P_60HZ:
                    case DISP_TV_MOD_1080I_60HZ:
                    case DISP_TV_MOD_1080P_60HZ:
                    case DISP_TV_MOD_720P_60HZ_3D_FP:
                                frame_rate = 60;
                                break;
                    case DISP_TV_MOD_576I:
                    case DISP_TV_MOD_576P:
                    case DISP_TV_MOD_720P_50HZ:
                    case DISP_TV_MOD_1080I_50HZ:
                    case DISP_TV_MOD_1080P_50HZ:
                    case DISP_TV_MOD_720P_50HZ_3D_FP:
                                frame_rate = 50;
                                break;
                    case DISP_TV_MOD_1080P_24HZ:
                    case DISP_TV_MOD_1080P_24HZ_3D_FP:
                                frame_rate = 24;
                                break;
                    case DISP_TV_MOD_1080P_25HZ:
                                frame_rate = 25;
                                break;
                    case DISP_TV_MOD_1080P_30HZ:
                                frame_rate = 30;
                                break;
                    default:
                                break;
                }
        }
        else if(gdisp.screen[sel].output_type & DISP_OUTPUT_TYPE_VGA)
        {
                frame_rate = 60;
        }

        return frame_rate;
}

__s32 BSP_disp_lcd_open_before(__u32 sel)
{
        LCD_GPIO_init(sel);
        lcdc_clk_on(sel, 0, 0);
        Disp_lcdc_reg_isr(sel);
        disp_clk_cfg(sel, DISP_OUTPUT_TYPE_LCD, DIS_NULL);
        lcdc_clk_on(sel, 0, 1);
        LCDC_init(sel);
        if(gpanel_info[sel].lcd_if == LCD_IF_LVDS)
        {
                lvds_clk_on();
        }
        image_clk_on(sel, 0);
        image_clk_on(sel, 1);
        Image_open(sel);//set image normal channel start bit , because every de_clk_off( )will reset this bit
        DE_BE_EnableINT(sel, DE_IMG_REG_LOAD_FINISH);
        Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_LCD, 1);
        if(gpanel_info[sel].tcon_index == 0)
        {
                tcon0_cfg(sel,(__panel_para_t*)&gpanel_info[sel]);
        }
        else
        {
                tcon1_cfg_ex(sel,(__panel_para_t*)&gpanel_info[sel]);
        }
        gdisp.screen[sel].output_csc_type = DISP_OUT_CSC_TYPE_LCD;
        BSP_disp_set_output_csc(sel, gdisp.screen[sel].output_csc_type);
        DE_BE_set_display_size(sel, gpanel_info[sel].lcd_x, gpanel_info[sel].lcd_y);
        DE_BE_Output_Select(sel, sel);

        open_flow[sel].func_num = 0;
        lcd_panel_fun[sel].cfg_open_flow(sel);

        return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_open_after(__u32 sel)
{	    
        //esMEM_SwitchDramWorkMode(DRAM_WORK_MODE_LCD);
        gdisp.screen[sel].b_out_interlace = 0;
        gdisp.screen[sel].status |= LCD_ON;
        gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_LCD;
        Lcd_Panel_Parameter_Check(sel);
#ifdef __LINUX_OSAL__
        Display_set_fb_timming(sel);
#endif
    return DIS_SUCCESS;
}

__lcd_flow_t * BSP_disp_lcd_get_open_flow(__u32 sel)
{    
        return (&open_flow[sel]);
}

__s32 BSP_disp_lcd_close_befor(__u32 sel)
{    
	close_flow[sel].func_num = 0;
	lcd_panel_fun[sel].cfg_close_flow(sel);
	gdisp.screen[sel].status &= LCD_OFF;
	gdisp.screen[sel].output_type = DISP_OUTPUT_TYPE_NONE;
	return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_close_after(__u32 sel)
{    
    Image_close(sel);
    Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_LCD, 0);
    DE_BE_DisableINT(sel, DE_IMG_REG_LOAD_FINISH);
    image_clk_off(sel, 1);
    lcdc_clk_off(sel);
    if(gpanel_info[sel].lcd_if == LCD_IF_LVDS)
    {
        lvds_clk_off();
    }

    gdisp.screen[sel].pll_use_status &= ((gdisp.screen[sel].pll_use_status == VIDEO_PLL0_USED)? VIDEO_PLL0_USED_MASK : VIDEO_PLL1_USED_MASK);
    LCD_GPIO_exit(sel);
    return DIS_SUCCESS;
}

__lcd_flow_t * BSP_disp_lcd_get_close_flow(__u32 sel)
{    
        return (&close_flow[sel]);
}

__s32 BSP_disp_lcd_xy_switch(__u32 sel, __s32 mode)
{       
        if(gdisp.screen[sel].LCD_CPUIF_XY_Swap != NULL)
        {
                LCD_CPU_AUTO_FLUSH(sel,0);
                LCD_XY_SWAP(sel);
                (*gdisp.screen[sel].LCD_CPUIF_XY_Swap)(mode);
                LCD_CPU_AUTO_FLUSH(sel,1);
        }      	 

        return DIS_SUCCESS;
}

//setting:  0,       1,      2,....  14,   15
//pol==0:  0,       2,      3,....  15,   16
//pol==1: 16,    14,    13, ...   1,   0
__s32 BSP_disp_lcd_set_bright(__u32 sel, __u32  bright)
{	    
        __u32 duty_ns;

        if((gpanel_info[sel].lcd_pwm_not_used == 0) && (gdisp.screen[sel].lcd_cfg.lcd_used))
        {
                if(bright != 0)
                {
                        bright += 1;
                }

                if(gpanel_info[sel].lcd_pwm_pol == 0)
                {
                        duty_ns = (bright * gdisp.pwm[gpanel_info[sel].lcd_pwm_ch].period_ns + 128) / 256;
                }
                else
                {
                        duty_ns = ((256 - bright) * gdisp.pwm[gpanel_info[sel].lcd_pwm_ch].period_ns + 128) / 256;
                }
                pwm_set_duty_ns(gpanel_info[sel].lcd_pwm_ch, duty_ns);
        }
        gdisp.screen[sel].lcd_bright = bright;

        return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_get_bright(__u32 sel)
{
        return gdisp.screen[sel].lcd_bright;	
}

__s32 BSP_disp_set_gamma_table(__u32 sel, __u32 *gamtbl_addr,__u32 gamtbl_size)
{       
        if((gamtbl_addr == NULL) || (gamtbl_size>1024))
        {
                DE_WRN("para invalid in BSP_disp_set_gamma_table\n");
                return DIS_FAIL;
        }

        tcon1_set_gamma_table(sel,(__u32)(gamtbl_addr),gamtbl_size);

        memcpy(gpanel_info[sel].lcd_gamma_tbl, gamtbl_addr, gamtbl_size);

        return DIS_SUCCESS;
}

__s32 BSP_disp_gamma_correction_enable(__u32 sel)
{    
	tcon1_set_gamma_Enable(sel,TRUE);

        gpanel_info[sel].lcd_gamma_correction_en = TRUE;
        
	return DIS_SUCCESS;
}

__s32 BSP_disp_gamma_correction_disable(__u32 sel)
{    
	tcon1_set_gamma_Enable(sel,FALSE);

        gpanel_info[sel].lcd_gamma_correction_en = FALSE;
    
	return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_set_src(__u32 sel, __disp_lcdc_src_t src)
{
        switch (src)
        {
                case DISP_LCDC_SRC_DE_CH1:
                        tcon0_src_select(sel, LCDC_SRC_BE0);
                        break;

                case DISP_LCDC_SRC_DE_CH2:
                        tcon0_src_select(sel, LCDC_SRC_BE1);
                        break;

                case DISP_LCDC_SRC_DMA:
                        tcon0_src_select(sel, LCDC_SRC_DMA);
                        break;

                case DISP_LCDC_SRC_WHITE:
                        tcon0_src_select(sel, LCDC_SRC_WHITE);
                        break;
                    
                case DISP_LCDC_SRC_BLACK:
                        tcon0_src_select(sel, LCDC_SRC_BLACK);
                        break;

                default:
                        DE_WRN("not supported lcdc src:%d in BSP_disp_tv_set_src\n", src);
                        return DIS_NOT_SUPPORT;
        }
        return DIS_SUCCESS;
}

__s32 BSP_disp_lcd_user_defined_func(__u32 sel, __u32 para1, __u32 para2, __u32 para3)
{
        return lcd_panel_fun[sel].lcd_user_defined_func(sel, para1, para2, para3);
}

void LCD_set_panel_funs(__lcd_panel_fun_t * lcd0_cfg, __lcd_panel_fun_t * lcd1_cfg)
{
        memset(&lcd_panel_fun[0], 0, sizeof(__lcd_panel_fun_t));
        memset(&lcd_panel_fun[1], 0, sizeof(__lcd_panel_fun_t));

        lcd_panel_fun[0].cfg_panel_info= lcd0_cfg->cfg_panel_info;
        lcd_panel_fun[0].cfg_open_flow = lcd0_cfg->cfg_open_flow;
        lcd_panel_fun[0].cfg_close_flow= lcd0_cfg->cfg_close_flow;
        lcd_panel_fun[0].lcd_user_defined_func = lcd0_cfg->lcd_user_defined_func;
        lcd_panel_fun[1].cfg_panel_info = lcd1_cfg->cfg_panel_info;
        lcd_panel_fun[1].cfg_open_flow = lcd1_cfg->cfg_open_flow;
        lcd_panel_fun[1].cfg_close_flow= lcd1_cfg->cfg_close_flow;
        lcd_panel_fun[1].lcd_user_defined_func = lcd1_cfg->lcd_user_defined_func;
}

__s32 BSP_disp_get_timming(__u32 sel, __disp_tcon_timing_t * tt)
{    
        memset(tt, 0, sizeof(__disp_tcon_timing_t));

        if(gdisp.screen[sel].status & LCD_ON)
        {
                LCDC_get_timing(sel, 0, tt);
                tt->pixel_clk = gpanel_info[sel].lcd_dclk_freq * 1000;
        }
        else if((gdisp.screen[sel].status & TV_ON )|| (gdisp.screen[sel].status & HDMI_ON))
        {
                __disp_tv_mode_t mode = gdisp.screen[sel].tv_mode;;

                LCDC_get_timing(sel, 1, tt);
                tt->pixel_clk = (clk_tab.tv_clk_tab[mode].tve_clk / clk_tab.tv_clk_tab[mode].pre_scale) / 1000;
        }
        else if(gdisp.screen[sel].status & VGA_ON )
        {
                __disp_tv_mode_t mode = gdisp.screen[sel].vga_mode;;

                LCDC_get_timing(sel, 1, tt);
                tt->pixel_clk = (clk_tab.tv_clk_tab[mode].tve_clk / clk_tab.vga_clk_tab[mode].pre_scale) / 1000;
        }
        else
        {
                DE_INF("get timming fail because device is not output !\n");
                return -1;
        }

        return 0;
}

__u32 BSP_disp_get_cur_line(__u32 sel)
{
        __u32 line = 0;

        if(gdisp.screen[sel].status & LCD_ON)
        {
                line = LCDC_get_cur_line(sel, 0);
        }
        else if((gdisp.screen[sel].status & TV_ON )|| (gdisp.screen[sel].status & HDMI_ON) || (gdisp.screen[sel].status & VGA_ON ))
        {
                line = LCDC_get_cur_line(sel, 1);
        }

        return line;
}

__s32 BSP_disp_close_lcd_backlight(__u32 sel)
{
    disp_gpio_set_t  gpio_info[1];
    __hdle hdl;
    int value=0,ret;
    char primary_key[20];

    sprintf(primary_key, "lcd%d_para", sel);
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_used", &value, 1);
    if(value==1)
    {
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_bl_en_used", &value, 1);
        if(value == 0)
        {
            DE_INF("%s.lcd_bl_en is not used\n", primary_key);
        }
        else
        {
            ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_bl_en", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
            if(ret < 0)
            {
                DE_INF("%s.lcd_bl_en not exist\n", primary_key);
            }
            else
            {
                gpio_info->data = (gpio_info->data==0)?1:0;
                hdl = OSAL_GPIO_Request(gpio_info, 1);
                OSAL_GPIO_Release(hdl, 2);
            }
        }

        value = 1;
        ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_used", &value, 1);
        if(value == 0)
        {
            DE_INF("%s.lcd_pwm is not used\n", primary_key);
        }
        else
        {
            ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_pwm", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
            if(ret < 0)
            {
                DE_INF("%s.lcd_pwm not exist\n", primary_key);
            }
            else
            {
                gpio_info->mul_sel = 0;
                hdl = OSAL_GPIO_Request(gpio_info, 1);
                OSAL_GPIO_Release(hdl, 2);
            }
        }
    }
    return 0;
}

__s32 BSP_disp_lcd_used(__u32 sel)
{
        return gdisp.screen[sel].lcd_cfg.lcd_used;
}

__s32 BSP_disp_restore_lcdc_reg(__u32 sel)
{
        LCDC_init(sel);

        if(BSP_disp_lcd_used(sel))
        {
                __pwm_info_t pwm_info;

                pwm_get_para(gdisp.screen[sel].lcd_cfg.lcd_pwm_ch, &pwm_info);

                pwm_info.enable = 0;
                pwm_set_para(gdisp.screen[sel].lcd_cfg.lcd_pwm_ch, &pwm_info);
        }

        return 0;
}

__s32 lcd_get_panel_para(__u32 sel, __panel_para_t * info)
{    
        memcpy(info, &gpanel_info[sel], sizeof(__panel_para_t));    
        return DIS_SUCCESS;
}

#ifdef __LINUX_OSAL__
EXPORT_SYMBOL(LCD_OPEN_FUNC);
EXPORT_SYMBOL(LCD_CLOSE_FUNC);
EXPORT_SYMBOL(LCD_get_reg_bases);
EXPORT_SYMBOL(LCD_delay_ms);
EXPORT_SYMBOL(LCD_delay_us);
EXPORT_SYMBOL(TCON_open);
EXPORT_SYMBOL(TCON_close);
EXPORT_SYMBOL(LCD_PWM_EN);
EXPORT_SYMBOL(LCD_BL_EN);
EXPORT_SYMBOL(LCD_POWER_EN);
EXPORT_SYMBOL(LCD_CPU_register_irq);
EXPORT_SYMBOL(LCD_CPU_WR);
EXPORT_SYMBOL(LCD_CPU_WR_INDEX);
EXPORT_SYMBOL(LCD_CPU_WR_DATA);
EXPORT_SYMBOL(LCD_CPU_AUTO_FLUSH);
EXPORT_SYMBOL(LCD_GPIO_request);
EXPORT_SYMBOL(LCD_GPIO_release);
EXPORT_SYMBOL(LCD_GPIO_set_attr);
EXPORT_SYMBOL(LCD_GPIO_read);
EXPORT_SYMBOL(LCD_GPIO_write);
EXPORT_SYMBOL(LCD_set_panel_funs);
EXPORT_SYMBOL(pwm_set_para);
EXPORT_SYMBOL(pwm_get_para);
EXPORT_SYMBOL(pwm_set_duty_ns);
EXPORT_SYMBOL(pwm_enable);
EXPORT_SYMBOL(lcd_get_panel_para);

#endif

