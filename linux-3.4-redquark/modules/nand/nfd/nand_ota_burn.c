#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <mach/clock.h>
#include <mach/platform.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/dma-mapping.h>
#include <mach/dma.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <mach/gpio.h>
#include <linux/gpio.h>

#include "nand_lib.h"

extern int NAND_Print(const char *fmt, ...);

#define  OOB_BUF_SIZE                   32
#define NAND_BOOT0_BLK_START    0
#define NAND_BOOT0_BLK_CNT		2
#define NAND_UBOOT_BLK_START    (NAND_BOOT0_BLK_START+NAND_BOOT0_BLK_CNT)
#define NAND_UBOOT_BLK_CNT		5
#define NAND_BOOT0_PAGE_CNT_PER_COPY     64


#define debug NAND_Print

extern int get_nand_para(void *boot_buf);
extern int gen_uboot_check_sum( void *boot_buf );
extern int gen_check_sum( void *boot_buf );
extern int get_dram_para(void *boot_buf);
extern int get_nand_para_for_boot1(void *boot_buf);

extern int NAND_PhysicLockInit(void);
extern int NAND_PhysicLock(void);
extern int NAND_PhysicUnLock(void);
extern int NAND_PhysicLockExit(void);

extern __u32 NAND_GetPageSize(void);
extern __u32 NAND_GetPageCntPerBlk(void);
extern __u32 NAND_GetReadRetryType(void);
//extern __u32 NAND_GetVersion(__u8* nand_version);
extern __s32 PHY_Readretry_reset(void);




__s32  burn_boot0_1k_mode( __u32 read_retry_type, __u32 Boot0_buf )
{
    __u32 i, j, k;
	__u32 pages_per_block;
	__u32 copies_per_block;
    __u8  oob_buf[32];
    struct boot_physical_param  para;

    debug("burn boot0 normal mode!\n");

    for(i=0;i<32;i++)
        oob_buf[i] = 0xff;

    NAND_GetVersion(oob_buf);
	if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	/* cal copy cnt per bock */
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	/* burn boot0 copys */
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
        debug("boot0 %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		continue;
    	}

        /* �ڿ�����дboot0���� */
        for( j = 0;  j < copies_per_block;  j++ )
       	{

			for( k = 0;  k < NAND_BOOT0_PAGE_CNT_PER_COPY;  k++ )
			{
				para.chip  = 0;
				para.block = i;
				para.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				para.mainbuf = (void *) (Boot0_buf + k * 1024);
				para.oobbuf = oob_buf;
				if( PHY_SimpleWrite_1K( &para ) <0)
				{
					debug("Warning. Fail in writing page %d in block %d.\n", j * NAND_BOOT0_PAGE_CNT_PER_COPY + k, i );
       			}
       		}
       	}
    }
	return 0;

error:
    return -1;
}

__s32  burn_boot0_1k_mode_F16( __u32 read_retry_type, __u32 Boot0_buf )
{
    __u32 i, j, k;
	__u32 pages_per_block;
	__u32 copies_per_block;
    __u8  oob_buf[32];
    struct boot_physical_param  para;

    debug("burn boot0 1k F16 mode!\n");

    for(i=0;i<32;i++)
        oob_buf[i] = 0xff;

    NAND_GetVersion(oob_buf);
	if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	/* cal copy cnt per bock */
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	/* burn boot0 copys */
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
        debug("boot0 %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		continue;
    	}

        /* �ڿ�����дboot0���� */
        for( j = 0;  j < copies_per_block;  j++ )
       	{

			for( k = 0;  k < NAND_BOOT0_PAGE_CNT_PER_COPY;  k++ )
			{
				para.chip  = 0;
				para.block = i;
				para.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				para.mainbuf = (void *) (Boot0_buf + k * 1024);
				para.oobbuf = oob_buf;
				if( PHY_SimpleWrite_1K_F16( &para ) <0)
				{
					debug("Warning. Fail in writing page %d in block %d.\n", j * NAND_BOOT0_PAGE_CNT_PER_COPY + k, i );
       			}
       		}
       	}
    }
	return 0;

error:
    return -1;
}


__s32  burn_boot0_lsb_mode(__u32 read_retry_type, __u32 Boot0_buf )
{
    __u32 i,k,page_cnt_per_copy,read_retry_mode,pages_per_block,length;
    __u8  oob_buf[32];
    __u32 page_size;
    struct boot_physical_param  para;
    void * buf;


	read_retry_mode = (read_retry_type>>16)&0xff;
	pages_per_block = NAND_GetPageCntPerBlk();
	
    debug("burn boot0 lsb mode,retry mode:%x!\n",read_retry_mode);

    for(i=0;i<32;i++)
        oob_buf[i] = 0xff;

	/* get nand driver version */
    NAND_GetVersion(oob_buf);
	if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}

	/* lsb enable */
	debug("lsb enalbe \n");
	debug("read retry mode: 0x%x\n", read_retry_type);
	if( NFC_LSBInit(read_retry_type) )
	{
	    debug("lsb init failed.\n");
		goto error;
	}
	NFC_LSBEnable(0, read_retry_type);



	/* ��� page count */
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!");
			goto error;
		}
	}
	/* ��� page count */
	if(page_size == 8192*2) //change for h27ucg8t2btr 16k pagesize
		page_size = 8192;
	length = 8192*4; 
	debug("boot0 length:%x\n",length);
	page_cnt_per_copy = length/page_size;

	/* burn boot0 */
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
        debug("boot0 %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		continue;
    	}

        /* �ڿ�����дboot0����, lsb mode�£�ÿ����ֻ��д������page */
		for( k = 0;  k < page_cnt_per_copy;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (Boot0_buf + k * page_size);
			para.oobbuf = oob_buf;
			if( PHY_SimpleWrite_Seq( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i );
   			}
   		}
			

    }

		buf =(void *) kmalloc(page_size,GFP_KERNEL);
		if(!buf)
		{
			debug("malloc fail:%x\n",(__u32)buf);	
			goto exit;
		}
    //check boot0
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
		struct boot_physical_param  para;
		__u32  k;

        debug("verify boot0 %x \n", i);

        /* �ڿ�����дboot0����, lsb mode�£�ÿ����ֻ��дǰ4��page */
		for( k = 0;  k < page_cnt_per_copy;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (buf);
			para.oobbuf = oob_buf;
			if( PHY_SimpleRead_Seq( &para ) <0 )
			{
				debug("Warning. Fail in reading page %d in block %d.\n",  k, i );
   			}
   		}

    }
		
exit:
	
    /* lsb disable */
    NFC_LSBDisable(0, read_retry_type);
    NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	
	debug("lsb disalbe 0\n");
	NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	
	debug("lsb disalbe 1\n");
	NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	
	debug("lsb disalbe 2\n");
	NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	

	return 0;

error:
    return -1;
}

int NAND_BurnBoot0(uint length, void *buf)
{
	__u32 read_retry_type = 0, read_retry_mode;
	void *buffer;
	NAND_PhysicLock();
	PHY_WaitAllRbReady();
	debug("buf_from %x \n",buf);
	buffer =(void *)kmalloc(length,GFP_KERNEL);
  if(!buffer)
	{
		debug("malloc fail:%x\n",(__u32)buffer);	
		goto error;
	}	
	
	debug("buf_kmalloc %x \n",buffer);
	copy_from_user(buffer,(const void*)buf,length);

	get_dram_para(buffer);
	debug("get dram para ok\n");
	get_nand_para(buffer);
	debug("get nand para ok\n");
	gen_check_sum(buffer);
	debug("get check sum ok\n");

	PHY_Readretry_reset();
	read_retry_type = NAND_GetReadRetryType();
	read_retry_mode = (read_retry_type>>16)&0xff;
	if( (read_retry_type>0)&&(read_retry_mode < 0x10))
	{
		if(read_retry_mode==0x4)
		{
			if( burn_boot0_1k_mode_F16(read_retry_type, (__u32)buffer) )
		        goto error;
		}	
	    else if( burn_boot0_lsb_mode(read_retry_type, (__u32)buffer) )
	        goto error;
	}
	else
	{
	    if( burn_boot0_1k_mode_F16(read_retry_type, (__u32)buffer) )
	        goto error;
	}
	debug("burn boot 0 success\n");
	PHY_WaitAllRbReady();
	NAND_PhysicUnLock();
    kfree(buffer);
	return 0;

error:
	debug("burn boot 0 failed\n");
	NAND_PhysicUnLock();
    kfree(buffer);
    return -1;

}

__s32  read_boot0_1k_mode( __u32 read_retry_type, __u32 Boot0_buf )
{
    __u32 i, j, k,m,err_flag;
	__u32 pages_per_block;
	__u32 copies_per_block;
    __u8  oob_buf[32];
    struct boot_physical_param  para;

    debug("read boot0 normal mode!\n");

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	/* cal copy cnt per bock */
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY;

	/* read boot0 copys */
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {
        debug("boot0 blk %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;

        for( j = 0;  j < copies_per_block;  j++ )
       	{
			err_flag = 0;
			//for( k = 0;  k < NAND_BOOT0_PAGE_CNT_PER_COPY;  k++ )
			for( k = 0;  k < 1;  k++ )
			{
				para.chip  = 0;
				para.block = i;
				para.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
				para.mainbuf = (void *) (Boot0_buf + k * 1024);
				para.oobbuf = oob_buf;
				for(m=0;m<32;m++)
			        oob_buf[m] = 0x55;
				if( PHY_SimpleRead_1K( &para ) <0)
				{
					debug("Warning. Fail in read page %d in block %d.\n", j * NAND_BOOT0_PAGE_CNT_PER_COPY + k, i );
					err_flag = 1;
					break;
				}
				if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
				{
					debug("get flash driver version error!");
					err_flag = 1;
					break;
				}
       		}
			if(err_flag == 0)
				break;
       	}
		if(err_flag == 0)
			break;
    }
	return 0;

error:
    return -1;
}


__s32  read_boot0_lsb_mode(__u32 read_retry_type, __u32 Boot0_buf )
{
    __u32 i, j,err_flag;
    __u8  oob_buf[32];
    __u32 page_size;
    struct boot_physical_param  para;



	/* ��� page count */
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!");
			goto error;
		}
	}

	page_size = 8192;


    //check boot0
    for( i = NAND_BOOT0_BLK_START;  i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT);  i++ )
    {

		__u32  k;

        debug("boot0 blk %x \n", i);
		err_flag = 0;

		for( k = 0;  k < 1;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (Boot0_buf + k * page_size);
			para.oobbuf = oob_buf;

			for(j=0;j<32;j++)
				oob_buf[j]=0x55;

			if( PHY_SimpleRead_Seq( &para ) <0 )
			{
				debug("Warning. Fail in reading page %d in block %d.\n",  k, i );
				err_flag = 1;
				break;
			}
			if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
			{
				debug("get flash driver version error!");
				err_flag = 1;
				break;
			}

   		}

		if(err_flag == 0)
			break;

    }

	return 0;

error:
    return -1;
}


int NAND_ReadBoot0(uint length, void *buf)
{
	__u32 read_retry_type = 0, read_retry_mode;
	void *buffer;

	PHY_WaitAllRbReady();

//	NAND_PhysicLock();
	buffer =(void *)kmalloc(1024*24,GFP_KERNEL);
  if(!buffer)
	{
		debug("malloc fail:%x\n",(__u32)buffer);	
		goto error;
	}
	PHY_Readretry_reset();
	read_retry_type = NAND_GetReadRetryType();
	read_retry_mode = (read_retry_type>>16)&0xff;
	if( (read_retry_type>0)&&(read_retry_mode < 0x10))
	{
		if(read_retry_mode==0x4)
		{
			if( read_boot0_1k_mode(read_retry_type, (__u32)buffer) )
	        	goto error;
		}
		else if( read_boot0_lsb_mode(read_retry_type, (__u32)buffer) )
	        goto error;
	}
	else
	{
	    if( read_boot0_1k_mode(read_retry_type, (__u32)buffer) )
	        goto error;
	}

	memcpy(buf,buffer,length);
	debug("nand read boot0 head ok\n");

//	NAND_PhysicUnLock();
	return 0;

error:
//	NAND_PhysicUnLock();
	debug("nand read boot0 head fail\n");
    return -1;

}


__s32 burn_uboot_in_one_blk_normal_mode(__u32 UBOOT_buf, __u32 length)
{
    __u32 i, k,read_retry_type,read_retry_mode;
    __u8  oob_buf[32];
    __u32 page_size, pages_per_block, pages_per_copy;
    struct boot_physical_param  para;

     debug("burn uboot normal mode!\n");
     //debug("uboot_buf: 0x%x \n", UBOOT_buf);

	 read_retry_type = NAND_GetReadRetryType();
	 read_retry_mode = (read_retry_type>>16)&0xff;


    for(i=0;i<32;i++)
        oob_buf[i] = 0xff;

	/* get nand driver version */
    NAND_GetVersion(oob_buf);
	if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}


	/* ��� page count */
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!");
			goto error;
		}
	}

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	debug("pages_per_block: 0x%x\n", pages_per_block);

	/* ����ÿ����������page */
	if(length%page_size)
	{
		debug("uboot length check error!\n");
		goto error;
	}
	pages_per_copy = length/page_size;
	if(pages_per_copy>pages_per_block)
	{
		debug("pages_per_copy check error!\n");
		goto error;
	}

	debug("pages_per_copy: 0x%x\n", pages_per_copy);

	
	/* burn uboot */
    for( i = NAND_UBOOT_BLK_START;  i < (NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT);  i++ )
    {
        debug("uboot %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		continue;
    	}

        /* �ڿ�����дboot0����, lsb mode�£�ÿ����ֻ��дǰ4��page */
		for( k = 0;  k < pages_per_copy;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (UBOOT_buf + k * page_size);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));
			if( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i );
   			}
   		}
        debug("fill uboot(normal mode) block with dummy data\n");
		for( k = pages_per_copy;  k < pages_per_block;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (UBOOT_buf + k * page_size);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));
			if( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i );
   			}
   		}

    }

	memset(oob_buf, 0, 32);
    //check uboot
    for( i = NAND_UBOOT_BLK_START;  i < (NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT);  i++ )
    {
	    debug("verify uboot blk %x \n", i);

      	for( k = 0;  k < pages_per_copy;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (UBOOT_buf + k * page_size);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));

			if( PHY_SimpleRead( &para ) <0 )
			{
				debug("Warning. Fail in read page %d in block %d.\n", k, i );
   			}
   		}
    }

	return 0;

error:
    return -1;
}


__s32 burn_uboot_in_one_blk_lsb_mode(__u32 UBOOT_buf, __u32 length)
{
    __u32 i, k,read_retry_type,read_retry_mode;
    __u8  oob_buf[32];
    __u32 page_size, pages_per_block, pages_per_copy;
    struct boot_physical_param  para;
    void * buf;

	debug("burn uboot lsb mode!\n");

	read_retry_type = NAND_GetReadRetryType();
	read_retry_mode = (read_retry_type>>16)&0xff;

	debug("lsb enalbe \n");
	debug("read retry mode: 0x%x\n", read_retry_type);
	if( NFC_LSBInit(read_retry_type) )
	{
		debug("lsb init failed.\n");
		goto error;
	}
	NFC_LSBEnable(0, read_retry_type);

    for(i=0;i<32;i++)
        oob_buf[i] = 0xff;

	/* get nand driver version */
    NAND_GetVersion(oob_buf);
    if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}


	/* ��� page count */
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!");
			goto error;
		}
	}

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	debug("pages_per_block: 0x%x\n", pages_per_block);

	/* ����ÿ����������page */
	if(length%page_size)
	{
		debug("uboot length check error!\n");
		goto error;
	}
	pages_per_copy = length/page_size;
	if(pages_per_copy>pages_per_block)
	{
		debug("pages_per_copy check error!\n");
		goto error;
	}

	debug("pages_per_copy: 0x%x\n", pages_per_copy);

	//while((*(volatile unsigned int *)0) != 0x1234);
	/* burn uboot */
    for( i = NAND_UBOOT_BLK_START;  i < (NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT);  i++ )
    {
        debug("uboot %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		continue;
    	}

        /* �ڿ�����дboot0����, lsb mode�£�ÿ����ֻ��дǰ4��page */
		for( k = 0;  k < pages_per_copy;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = _lsb_page_trans(k);
			//debug("get lsb page ,src page num:%x ,phy page num:%x \n",k,para.page);
			if(para.page==0xffffffff){
				debug("get lsb page error,src page num:%x \n",k);
				break;
			}
			para.mainbuf = (void *) (UBOOT_buf + k * page_size);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));
			if( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i );
   			}
   		}
		debug("fill uboot(lsb mode) block with dummy data\n");
		for(k = pages_per_copy; k < pages_per_block ; k++)
		{
			para.chip  = 0;
			para.block = i;
			para.page  = _lsb_page_trans(k);
			if(para.page==0xffffffff){
				debug("not lsb page ,the last lsb page num:%x \n",_lsb_page_trans(k-1));
				break;
			}
			para.mainbuf = (void *) (UBOOT_buf);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));
			if( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i );
   			}
   		}		
    }

	memset(oob_buf, 0, 32);
		buf =(void *) kmalloc(page_size,GFP_KERNEL);
		if(!buf)
		{
			debug("malloc fail:%x\n",(__u32)buf);	
			goto exit;
		}	
	
    //check uboot
    for( i = NAND_UBOOT_BLK_START;  i < (NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT);  i++ )
    {
	    debug("verify uboot blk %x \n", i);

      	for( k = 0;  k < pages_per_block;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = _lsb_page_trans(k);
			if(para.page==0xffffffff){
				debug("not lsb page ,the last lsb page num:%x \n",_lsb_page_trans(k-1));
				break;
			}
			para.mainbuf = (void *) (buf);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));

			if( PHY_SimpleRead( &para ) <0 )
			{
				debug("Warning. Fail in read page %d in block %d.\n", k, i );
   			}
   		}
    }

exit:
    /* lsb disable */

    NFC_LSBDisable(0, read_retry_type);
    NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	
	debug("lsb disalbe 0\n");
	NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	
	debug("lsb disalbe 1\n");
	NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	
	debug("lsb disalbe 2\n");
	NFC_LSBExit(read_retry_type);
	PHY_WaitAllRbReady();	
	

	return 0;

error:
    return -1;
}


__s32 burn_uboot_in_one_blk_lsb_mode_F16(__u32 UBOOT_buf, __u32 length)
{
    __u32 i, k,read_retry_type,read_retry_mode;
    __u8  oob_buf[32];
    __u32 page_size, pages_per_block, pages_per_copy,page_a;
    struct boot_physical_param  para;
    void * buf;

	debug("burn uboot lsb mode F16!\n");

	read_retry_type = NAND_GetReadRetryType();
	read_retry_mode = (read_retry_type>>16)&0xff;


    for(i=0;i<32;i++)
        oob_buf[i] = 0xff;

	/* get nand driver version */
    NAND_GetVersion(oob_buf);
    if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}


	/* ��� page count */
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!");
			goto error;
		}
	}

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	debug("pages_per_block: 0x%x\n", pages_per_block);

	/* ����ÿ����������page */
	if(length%page_size)
	{
		debug("uboot length check error!\n");
		goto error;
	}
	pages_per_copy = length/page_size;
	if(pages_per_copy>pages_per_block)
	{
		debug("pages_per_copy check error!\n");
		goto error;
	}

	debug("pages_per_copy: 0x%x\n", pages_per_copy);

    for( i = NAND_UBOOT_BLK_START;  i < (NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT);  i++ )
    {
        debug("uboot %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		continue;
    	}
		para.page = 0;
		page_a = 0;
		for( k = 0;  k < pages_per_copy;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = _lsb_page_trans(k);
			//debug("get lsb page ,src page num:%x ,phy page num:%x \n",k,para.page);
			if(para.page==0xffffffff){
				debug("get lsb page error,src page num:%x \n",k);
				break;
			}
			if((para.page!=(page_a + 1))&&(para.page > page_a ))
			{
				struct boot_physical_param  para_1;
				__u32 j;
				para_1.chip = 0;
				para_1.block = i;
				for(j=page_a+1; j<para.page; j++)
				{
					para_1.page = j;
					if(PHY_SimpleWrite_0xFF(&para_1) < 0)
						debug("Warning. Fail in writing page %d in block %d.\n", j, i );
				}			
			}
			page_a = para.page;
			para.mainbuf = (void *) (UBOOT_buf + k * page_size);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));
			if( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i );
   			}
   		}
		debug("fill uboot(lsb mode) block with dummy data\n");
		for(k = page_a+1; k < pages_per_block ; k++)
		{
			para.chip  = 0;
			para.block = i;
			if(_Is_lsb_page_or_not(k))
			{
				para.mainbuf = (void *) (UBOOT_buf );
				para.oobbuf = oob_buf;
				para.page = k;
				if( PHY_SimpleWrite( &para ) <0 )
				{
					debug("Warning. Fail in writing page dummy data %d in block %d.\n", k, i );
				}

			}
			else
			{
				struct boot_physical_param  para_1;
				para_1.chip = 0;
				para_1.block = i;
				para_1.page = k;
				if(PHY_SimpleWrite_0xFF(&para_1) < 0)
					debug("Warning. Fail in writing 0xFF to page %d in block %d.\n", k, i );

			}			
		}
    }

	memset(oob_buf, 0, 32);
		buf =(void *) kmalloc(page_size,GFP_KERNEL);
		if(!buf)
		{
			debug("malloc fail:%x\n",(__u32)buf);	
			goto exit;
		}	
	
    //check uboot
    for( i = NAND_UBOOT_BLK_START;  i < (NAND_UBOOT_BLK_START + NAND_UBOOT_BLK_CNT);  i++ )
    {
	    debug("verify uboot blk %x \n", i);

      	for( k = 0;  k < pages_per_block;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = _lsb_page_trans(k);
			if(para.page==0xffffffff){
				debug("not lsb page ,the last lsb page num:%x \n",_lsb_page_trans(k-1));
				break;
			}
			para.mainbuf = (void *) (buf);
			para.oobbuf = oob_buf;
			//debug("burn uboot: block: 0x%x, page: 0x%x, mainbuf: 0x%x, maindata: 0x%x \n", para.block, para.page, (__u32)para.mainbuf, *((__u32 *)para.mainbuf));

			if( PHY_SimpleRead( &para ) <0 )
			{
				debug("Warning. Fail in read page %d in block %d.\n", k, i );
   			}
   		}
    }
exit:
	return 0;

error:
    return -1;
}

__s32 burn_uboot_in_many_blks(__u32 UBOOT_buf, __u32 length)
{
     __u32 i,  k;
    __u8  oob_buf[32];
    __u32 page_size, pages_per_block, pages_per_copy, page_index;
    struct boot_physical_param  para;

     debug("burn uboot normal mode!\n");

    for(i=0;i<32;i++)
        oob_buf[i] = 0xff;

	/* get nand driver version */
    NAND_GetVersion(oob_buf);
	if((oob_buf[0]!=0xff)||(oob_buf[1]!= 0x00))
	{
		debug("get flash driver version error!");
		goto error;
	}


	/* ��� page count */
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!");
			goto error;
		}
	}

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!", pages_per_block);
		goto error;
	}

	/* ����ÿ����������page */
	if(length%page_size)
	{
		debug("uboot length check error!\n");
		goto error;
	}
	pages_per_copy = length/page_size;
	if(pages_per_copy<=pages_per_block)
	{
		debug("pages_per_copy check error!\n");
		goto error;
	}


	/* burn uboot */
	page_index = 0;
    for( i = NAND_UBOOT_BLK_START;  i < (NAND_UBOOT_BLK_START + 9);  i++ )
    {
        debug("uboot %x \n", i);

		/* ������ */
		para.chip  = 0;
		para.block = i;
		if( PHY_SimpleErase( &para ) <0 )
		{
		    debug("Fail in erasing block %d.\n", i );
    		continue;
    	}

        /* �ڿ�����дboot0����, lsb mode�£�ÿ����ֻ��дǰ4��page */
		for( k = 0;  k < pages_per_block;  k++ )
		{
			para.chip  = 0;
			para.block = i;
			para.page  = k;
			para.mainbuf = (void *) (UBOOT_buf + page_index* page_size);
			para.oobbuf = oob_buf;
			if( PHY_SimpleWrite( &para ) <0 )
			{
				debug("Warning. Fail in writing page %d in block %d.\n", k, i );
   			}
   			page_index++;

   			if(page_index >= pages_per_copy)
   				break;
   		}

   		if(page_index >= pages_per_copy)
   			break;

    }

    if(page_index >= pages_per_copy)
		return 0;
	else
		goto error;

error:
    return -1;
}


int NAND_BurnBoot1(uint length, void *buf)
{
	int ret = 0;
	__u32 page_size, pages_per_block, block_size;
	__u32 read_retry_type,read_retry_mode;
	void *buffer;

	NAND_PhysicLock();
	PHY_WaitAllRbReady();

	buffer =(void *) kmalloc(length,GFP_KERNEL);
  
  if(!buffer)
	{
		debug("malloc fail:%x\n",(__u32)buffer);	
		goto error;
	}

	copy_from_user(buffer, (const void*)buf, length);

	get_nand_para_for_boot1(buffer);
	gen_uboot_check_sum(buffer);

	PHY_Readretry_reset();

	/* ��� page count */
	read_retry_type = NAND_GetReadRetryType();
	read_retry_mode = (read_retry_type>>16)&0xff;
	page_size = NAND_GetPageSize();
	{
		if(page_size %1024)
		{
			debug("get flash page size error!\n");
			goto error;
		}
	}

	/* ��� page count */
	pages_per_block = NAND_GetPageCntPerBlk();
	if(pages_per_block%64)
	{
		debug("get page cnt per block error %x!\n", pages_per_block);
		goto error;
	}

	block_size = page_size*pages_per_block;
	if(length%page_size)
	{
		debug(" uboot length check error!\n");
		goto error;
	}

	if(length<=block_size)
	{
		if(((read_retry_type>0))&&((read_retry_mode==0x0)||(read_retry_mode==0x1)||(read_retry_mode==0x2)||(read_retry_mode==0x3)))
			ret = burn_uboot_in_one_blk_normal_mode((__u32)buffer, length);			
		else if((read_retry_type>0)&&(read_retry_mode == 0x4))
			ret = burn_uboot_in_one_blk_normal_mode((__u32)buffer, length);
		else
			ret = burn_uboot_in_one_blk_normal_mode((__u32)buffer, length);
		debug("%d %d\n", __LINE__, ret);
	}
	else
	{
		ret = burn_uboot_in_many_blks((__u32)buffer, length);
		debug("%d %d\n", __LINE__, ret);
	}
	PHY_WaitAllRbReady();
	NAND_PhysicUnLock();
	debug("burn uboot  success\n");
	return ret;

error:
	NAND_PhysicUnLock();
	debug("burn uboot failed\n");
	return -1;

}



void test_dram_para(void *buffer)
{
	int *data;
	int  i;

	data = (int *)buffer;
	for(i=0;i<40;i+=4)
	{
		debug("%x %x %x %x\n", data[i+0], data[i+1], data[i+2], data[i+3]);
	}
	debug("\n");

	return;
}

