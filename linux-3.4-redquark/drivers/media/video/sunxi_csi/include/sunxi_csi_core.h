/*
 * sun7i Camera core header file
 * Author:raymonxiu
*/
#ifndef _SUN7I_CSI_CORE_H_
#define _SUN7I_CSI_CORE_H_

#include <linux/types.h>
#include <media/videobuf-core.h>
#include <media/v4l2-device.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <media/v4l2-mediabus.h>//linux-3.0
#include <mach/sys_config.h>

//for internel driver debug
#define DBG_EN   		0 	
//debug level 0~3
#define DBG_LEVEL 	3

//for internel driver debug
#if(DBG_EN==1)		
#define csi_dbg(l,x,arg...) if(l <= DBG_LEVEL) printk("[CSI_DEBUG]"x,##arg)
#else
#define csi_dbg(l,x,arg...) 
#endif

//print when error happens
#define csi_err(x,arg...) printk(KERN_INFO"[CSI_ERR]"x,##arg)

//print unconditional, for important info
#define csi_print(x,arg...) printk(KERN_INFO"[CSI]"x,##arg)

#define MAX_NUM_INPUTS 2

struct csi_subdev_info {
	const char *name;
	struct i2c_board_info board_info;
};

/*
 * input data format
 */
typedef enum tag_CSI_INPUT_FMT
{
    CSI_RAW=0,     /* raw stream  */
    CSI_BAYER,     /* byer rgb242 */
    CSI_CCIR656,   /* ccir656     */
    CSI_YUV422,    /* yuv422      */
    CSI_YUV422_16 = 4, /* yuv422 16 bit */
    CSI_YUV444 = 4,		 /* yuv444 24 bit */
}__csi_input_fmt_t;

/*
 * output data format
 */
typedef enum tag_CSI_OUTPUT_FMT
{
    /* only when input is raw */
    CSI_PASS_THROUTH = 0,                /* raw */

    /* only when input is bayer */
    CSI_PLANAR_RGB242 = 0,               /* planar rgb242 */

    /* only when input is ccir656 */
    CSI_FIELD_PLANAR_YUV422 = 0,         /* parse a field(odd or even) into planar yuv420 */
    CSI_FIELD_PLANAR_YUV420 = 1,         /* parse a field(odd or even) into planar yuv420 */
    CSI_FRAME_PLANAR_YUV420 = 2,
    CSI_FRAME_PLANAR_YUV422 = 3,
    CSI_FIELD_UV_CB_YUV422  = 4,         /* parse and reconstruct evry 2 fields(odd and even) into a frame, format is planar yuv420 */
    CSI_FIELD_UV_CB_YUV420  = 5,
    CSI_FRAME_UV_CB_YUV420  = 6,
    CSI_FRAME_UV_CB_YUV422  = 7,
    CSI_FIELD_MB_YUV422     = 8,
    CSI_FIELD_MB_YUV420     = 9,
    CSI_FRAME_MB_YUV422     = 10,
    CSI_FRAME_MB_YUV420     = 11,
    CSI_INTLC_INTLV_YUV422  = 15,

    /* only when input is yuv422 */
    CSI_PLANAR_YUV422=0,                /* parse yuv422 into planar yuv422 */
    CSI_PLANAR_YUV420=1,                /* parse yuv422 into planar yuv420 */
    CSI_UV_CB_YUV422=4,
    CSI_UV_CB_YUV420=5,
    CSI_MB_YUV422=8,
    CSI_MB_YUV420=9,
    
    /* only when input is yuv444 */
    CSI_FIELD_PLANAR_YUV444 			 = 0,         /* parse a field(odd or even) into planar yuv444 */
    CSI_FIELD_UV_CB_YUV444_YUV422 = 1,         /* parse a field(odd or even) into planar yuv422 */
    CSI_FRAME_PLANAR_YUV444 			 = 2,				
    CSI_FRAME_UV_CB_YUV444_YUV422 = 3,
    
}__csi_output_fmt_t;

/*
 * input field selection, only when input is ccir656
 */
typedef enum tag_CSI_FIELD_SEL
{
    CSI_ODD,    /* odd field */
    CSI_EVEN,   /* even field */
    CSI_EITHER, /* either field */
}__csi_field_sel_t;

/*
 * input data sequence
 */
typedef enum tag_CSI_SEQ
{
    /* only when input is yuv422 */
    CSI_YUYV=0,
    CSI_YVYU,
    CSI_UYVY,
    CSI_VYUY,

    /* only when input is byer */
    CSI_RGRG=0,               /* first line sequence is RGRG... */
    CSI_GRGR,                 /* first line sequence is GRGR... */
    CSI_BGBG,                 /* first line sequence is BGBG... */
    CSI_GBGB,                 /* first line sequence is GBGB... */
}__csi_seq_t;

/*
 * input reference signal polarity
 */
typedef enum tag_CSI_REF
{
    CSI_LOW,    /* active low */
    CSI_HIGH,   /* active high */
}__csi_ref_t;

/*
 * input data valid of the input clock edge type
 */
typedef enum tag_CSI_CLK
{
    CSI_FALLING,    /* active falling */
    CSI_RISING,     /* active rising */
}__csi_clk_t;

/*
 * csi mode configuration
 */
typedef struct tag_CSI_CONF
{
    __csi_input_fmt_t  input_fmt;   /* input data format */
    __csi_output_fmt_t output_fmt;  /* output data format */
    __csi_field_sel_t  field_sel;   /* input field selection */
    __csi_seq_t        seq;         /* input data sequence */
    __csi_ref_t        vref;        /* input vref signal polarity */
    __csi_ref_t        href;        /* input href signal polarity */
    __csi_clk_t        clock;       /* input data valid of the input clock edge type */
}__csi_conf_t;



/*
 * csi buffer
 */

typedef enum tag_CSI_BUF
{
    CSI_BUF_0_A,    /* FIFO for Y address A */  
    CSI_BUF_0_B,    /* FIFO for Y address B */
    CSI_BUF_1_A,    /* FIFO for Cb address A */
    CSI_BUF_1_B,    /* FIFO for Cb address B */
    CSI_BUF_2_A,    /* FIFO for Cr address A */
    CSI_BUF_2_B,    /* FIFO for Cr address B */
}__csi_buf_t;

/*
 * csi capture status
 */
typedef struct tag_CSI_CAPTURE_STATUS
{
    _Bool picture_in_progress;  
    _Bool video_in_progress;    
}__csi_capture_status;


/*
 * csi double buffer
 */
typedef enum tag_CSI_DOUBLE_BUF
{
    CSI_BUF_A,  
    CSI_BUF_B,
}__csi_double_buf_t;

/*
 * csi double buffer status
 */
typedef struct tag_CSI_DOUBLE_BUF_STATUS
{
    _Bool             enable;   /* double buffer enable */
    __csi_double_buf_t cur;     /* current frame selected output type, buffer A or B*/
    __csi_double_buf_t next;    /* next frame output type, buffer A or B */
}__csi_double_buf_status_t;

/*
 * csi interrupt
 */
typedef enum tag_CSI_INT
{
    CSI_INT_CAPTURE_DONE     = 0X1,
    CSI_INT_FRAME_DONE       = 0X2,
    CSI_INT_BUF_0_OVERFLOW   = 0X4,
    CSI_INT_BUF_1_OVERFLOW   = 0X8,
    CSI_INT_BUF_2_OVERFLOW   = 0X10,
    CSI_INT_PROTECTION_ERROR = 0X20,
    CSI_INT_HBLANK_OVERFLOW  = 0X40,
    CSI_INT_VSYNC_TRIG       = 0X80,
}__csi_int_t;

/*
 * csi interrupt status
 */
typedef struct tag_CSI_INT_STATUS
{
    _Bool capture_done;
    _Bool frame_done;
    _Bool buf_0_overflow;
    _Bool buf_1_overflow;
    _Bool buf_2_overflow;
    _Bool protection_error;
    _Bool hblank_overflow;
    _Bool vsync_trig;
}__csi_int_status_t;

typedef enum tag_CSI_IF
{
		CSI_IF_HV8 					= 0,
		CSI_IF_CCIR656_16 	= 1,
		CSI_IF_HV24 				= 2,
		CSI_IF_CCIR656			= 3,
		CSI_IF_CCIR656_2CH	= 4,
		CSI_IF_CCIR656_4CH 	= 5,
}__csi_if_t;

/*
 * csi sub device info
 */
typedef struct tag_CSI_SUBDEV_INFO
{
    int								 mclk;				/* the mclk frequency for sensor module in HZ unit*/
    __csi_ref_t        vref;        /* input vref signal polarity */
    __csi_ref_t        href;        /* input href signal polarity */
    __csi_clk_t        clock;       /* input data valid of the input clock edge type */
    int								 iocfg;				/*0 for csi back , 1 for csi front*/				
    int 							 stby_mode;			
}__csi_subdev_info_t;

struct csi_buf_addr {
	dma_addr_t	y;
	dma_addr_t	cb;
	dma_addr_t	cr;
};

struct csi_fmt {
	u8					name[32];
	__csi_if_t									csi_if;
	enum v4l2_mbus_pixelcode		ccm_fmt;//linux-3.0
	u32   											fourcc;          /* v4l2 format id */
	enum v4l2_field							field;
	__csi_input_fmt_t						input_fmt;	
	__csi_output_fmt_t 					output_fmt;	
	__csi_field_sel_t						csi_field;
	int   				depth;
	u16	  				planes_cnt;
};

struct csi_size{
	u32		csi_width;
	u32		csi_height;
};

/* buffer for one video frame */
struct csi_buffer {
	struct videobuf_buffer vb;
	struct csi_fmt        *fmt;
};

struct csi_dmaqueue {
	struct list_head active;
	
	/* Counters to control fps rate */
	int frame;
	int ini_jiffies;
};

static LIST_HEAD(csi_devlist);

struct ccm_config {
	char ccm[I2C_NAME_SIZE];
	char iovdd_str[32];
	char avdd_str[32];
	char dvdd_str[32];
	int twi_id;
	uint i2c_addr;
	int vflip;
	int hflip;
	int stby_mode;
	int interface;
	int flash_pol;		
//	user_gpio_set_t reset_io;
//	user_gpio_set_t standby_io;
//	user_gpio_set_t power_io;
//	user_gpio_set_t flash_io;
//	user_gpio_set_t af_power_io;
	//modified to 33
	struct gpio_config reset_io;
	struct gpio_config standby_io;
	struct gpio_config power_io;
	struct gpio_config flash_io;
	struct gpio_config af_power_io;
	
	struct regulator 	 *iovdd;		  /*interface voltage source of sensor module*/
	struct regulator 	 *avdd;			/*anlog voltage source of sensor module*/
	struct regulator 	 *dvdd;			/*core voltage source of sensor module*/

    //add by heyihang.Jan 15, 2013	
	uint vol_iovdd;
	uint vol_avdd;
	uint vol_dvdd;
	
	__csi_subdev_info_t ccm_info;  
	struct v4l2_subdev			*sd;
};

struct csi_dev {
	struct list_head       	csi_devlist;
	struct v4l2_device 	   	v4l2_dev;
	struct v4l2_subdev			*sd;
	struct platform_device	*pdev;

	int						id;
	
	spinlock_t              slock;

	/* various device info */
	struct video_device     *vfd;

	struct csi_dmaqueue     vidq;

	/* Several counters */
	unsigned 		   		ms;
	unsigned long           jiffies;

	/* Input Number */
	int			   			input;

	/* video capture */
	struct csi_fmt          *fmt;
	unsigned int            width;
	unsigned int            height;
	unsigned int						frame_size;
	struct videobuf_queue   vb_vidq;
	unsigned int 						capture_mode;
	
	/*working state*/
	unsigned long 		   	generating;
	int						opened;

	/*pin,clock,irq resource*/
	int							csi_pin_hd;

    //add by heyihang.Jan 17, 2013
    script_item_u     *csi_pin_list;
	int               csi_pin_cnt;
    
	struct clk				*csi_clk_src;
	struct clk				*csi_ahb_clk;
	struct clk				*csi_module_clk;
	struct clk				*csi_dram_clk;
	struct clk				*csi_isp_src_clk;
	struct clk				*csi_isp_clk;
	int						irq;
	void __iomem			*regs;
	struct resource			*regs_res;
	
	/*power issue*/
	
	int								 stby_mode;
	struct regulator 	 *iovdd;		  /*interface voltage source of sensor module*/
  struct regulator 	 *avdd;			/*anlog voltage source of sensor module*/
  struct regulator 	 *dvdd;			/*core voltage source of sensor module*/

    //add by heyihang.Jan 15, 2013
    uint vol_iovdd;
	uint vol_avdd;
	uint vol_dvdd;
    
	/* attribution */
	int interface;
	int vflip;
	int hflip;
	int flash_pol;
	
	/* csi io */
//	user_gpio_set_t reset_io;
//	user_gpio_set_t standby_io;
//	user_gpio_set_t power_io;
//	user_gpio_set_t flash_io;
//	user_gpio_set_t af_power_io;
	//modified to 33
	struct gpio_config reset_io;
	struct gpio_config standby_io;
	struct gpio_config power_io;
	struct gpio_config flash_io;
	struct gpio_config af_power_io;
	
	/*parameters*/
	__csi_conf_t			csi_mode;
	struct csi_buf_addr		csi_buf_addr;
	
	/* ccm config */
  int dev_qty;
	int module_flag;
	__csi_subdev_info_t *ccm_info;  /*current config*/
	struct ccm_config *ccm_cfg[MAX_NUM_INPUTS];
};

void  bsp_csi_open(struct csi_dev *dev);
void  bsp_csi_close(struct csi_dev *dev);
void  bsp_csi_configure(struct csi_dev *dev,__csi_conf_t *mode);
void	inline  bsp_csi_set_buffer_address(struct csi_dev *dev,__csi_buf_t buf, u32 addr);
u32		inline  bsp_csi_get_buffer_address(struct csi_dev *dev,__csi_buf_t buf);
void  bsp_csi_double_buffer_enable(struct csi_dev *dev);
void  bsp_csi_double_buffer_disable(struct csi_dev *dev);
void	inline  bsp_csi_double_buffer_select_next(struct csi_dev *dev,__csi_double_buf_t type);
void	inline  bsp_csi_double_buffer_get_status(struct csi_dev *dev,__csi_double_buf_status_t * status);
void  bsp_csi_capture_video_start(struct csi_dev *dev);
void  bsp_csi_capture_video_stop(struct csi_dev *dev);
void  bsp_csi_capture_picture(struct csi_dev *dev);
void  bsp_csi_capture_get_status(struct csi_dev *dev,__csi_capture_status * status);
void 	bsp_csi_set_size(struct csi_dev *dev, u32 length_h, u32 length_v, u32 buf_length_h);
void 	bsp_csi_set_offset(struct csi_dev *dev,u32 start_h, u32 start_v);
void  bsp_csi_int_enable(struct csi_dev *dev,__csi_int_t interrupt);
void  bsp_csi_int_disable(struct csi_dev *dev,__csi_int_t interrupt);
void 	inline 	bsp_csi_int_get_status(struct csi_dev *dev,__csi_int_status_t * status);
void 	inline bsp_csi_int_clear_status(struct csi_dev *dev,__csi_int_t interrupt);

#endif  /* _CSI_H_ */
