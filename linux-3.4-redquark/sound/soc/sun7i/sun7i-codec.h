
#ifndef _SUN7I_CODEC_H
#define _SUN7I_CODEC_H

//Codec Register
#define CODEC_BASSADDRESS         (SW_PA_ADDA_IO_BASE) /* 0x01c22c00 */
#define SUN7I_DAC_DPC                (0x00)
#define SUN7I_DAC_FIFOC              (0x04)
#define SUN7I_DAC_FIFOS              (0x08)
#define SUN7I_DAC_TXDATA             (0x0c)
#define SUN7I_DAC_ACTL               (0x10)
#define SUN7I_DAC_TUNE               (0x14)
#define SUN7I_DAC_DEBUG              (0x18)
#define SUN7I_ADC_FIFOC              (0x1c)
#define SUN7I_ADC_FIFOS              (0x20)
#define SUN7I_ADC_RXDATA             (0x24)
#define SUN7I_ADC_ACTL               (0x28)
#define SUN7I_ADC_DEBUG              (0x2c)
#define SUN7I_DAC_TXCNT              (0x30)
#define SUN7I_ADC_RXCNT              (0x34)
#define SUN7I_BIAS_CRT		         (0x38)
#define SUN7I_MIC_CRT		         (0x3c)
#define SUN7I_CODEC_REGS_NUM         (13)

#define DAIFMT_16BITS             (16)
#define DAIFMT_20BITS             (20)

#define DAIFMT_BS_MASK            (~(1<<16))  	//FIFO big small mode mask
#define DAIFMT_BITS_MASK          (~(1<<5))		//FIFO Bits select mask,not used yet.
#define SAMPLE_RATE_MASK          (~(7<<29))  	//Sample Rate slect mask

#define DAC_EN                    (31)
#define DIGITAL_VOL               (12)
//For CODEC OLD VERSION
#define DAC_VERSION               (23)

#define DAC_CHANNEL				  (6)
#define LAST_SE                   (26)
#define TX_FIFO_MODE              (24)
#define DRA_LEVEL                 (21)
#define TX_TRI_LEVEL              (8)
#define DAC_MODE                  (6)			//not used yet
#define TASR                      (5)			//not used yet
#define DAC_DRQ                   (4)
#define DAC_FIFO_FLUSH            (0)
/*0x10*/
#define VOLUME                    (0)
#define PA_MUTE                   (6)
#define MIXPAS                    (7)
#define DACPAS                    (8)
#define MIXEN                     (29)
#define LINEIN_MIXER_L                     (19)
#define LINEIN_MIXER_R                     (18)
#define LDAC_MIXER                 (15)
#define RDAC_MIXER                  (14)
#define DACAEN_L                  (30)
#define DACAEN_R                  (31)

/*0x1c*/
#define ADC_DIG_EN                (28)
#define RX_FIFO_MODE              (24)
#define RX_TRI_LEVEL              (8)
#define ADC_MODE                  (7)
#define RASR                      (6)
#define ADC_DRQ                   (4)
#define ADC_FIFO_FLUSH            (0)
/*0x28*/
#define  ADC_LF_EN                (31)
#define  ADC_RI_EN                (30)
#define  MIC1_EN                  (29)
#define  MIC2_EN                  (28)
#define  VMIC_EN                  (27)
#define  ADC_SELECT               (17)
#define  line_in_mode             (16)
#define  PA_ENABLE                (4)
#define  HP_DIRECT                (3)
/*0x3c*/
#define  PHONEOUT_EN                (4)
#define  mic1_preg1                (29)
#define  mic2_preg1                (26)
#define  PHONEOUTG                (5)
#define  MIC1_BOOST_PHONEOUT                (3)
#define  MIC2_BOOST_PHONEOUT                (2)
#define  RIGRT_PHONEOUT                 (1)
#define  LEFT_PHONEOUT                 (0)

enum m1_codec_config {
	CMD_MIC_SEL =0,
	CMD_ADC_SEL,
};

void  __iomem *baseaddr;

#define AUDIO_RATE_DEFAULT	44100
	struct sun7i_pcm_dma_params {
		char *name;		
		dma_addr_t dma_addr;	
	};
#define ST_RUNNING		(1<<0)
#define ST_OPENED		(1<<1)
 
/*struct sun7i_pcm_dma_params {
	struct sw_dma_client *client;	 stream identifier 
	unsigned int channel;				 Channel ID 
	dma_addr_t dma_addr;
	int dma_size;		 Size of the DMA transfer
}; */

#define codec_rdreg(reg)	    readl((baseaddr+(reg)))
#define codec_wrreg(reg,val)  writel((val),(baseaddr+(reg)))

/*
* Convenience kcontrol builders
*/
#define CODEC_SINGLE_VALUE(xreg, xshift, xmax,	xinvert)\
		((unsigned long)&(struct codec_mixer_control)\
		{.reg	=	xreg,	.shift	=	xshift,	.rshift	=	xshift,	.max	=	xmax,\
   	.invert	=	xinvert})

#define CODEC_SINGLE(xname,	reg,	shift,	max,	invert)\
{	.iface	= SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,\
	.info	= snd_codec_info_volsw,	.get = snd_codec_get_volsw,\
	.put	= snd_codec_put_volsw,\
	.private_value	= CODEC_SINGLE_VALUE(reg, shift, max, invert)}

/*	mixer control*/	
struct	codec_mixer_control{
	int	min;
	int     max;
	int     where;
	unsigned int mask;
	unsigned int reg;
	unsigned int rreg;
	unsigned int shift;
	unsigned int rshift;
	unsigned int invert;
	unsigned int value;
};

extern int __init snd_chip_codec_mixer_new(struct snd_card *card);
#endif
