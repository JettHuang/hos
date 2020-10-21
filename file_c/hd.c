/*----------------------------------------------------------*
 *说明: AT硬盘驱动程序
 *				huanghehsui 05/30/07 14:33
 *----------------------------------------------------------*/
#include  "..\include\asm.h"
#include  "..\include\color.h"
#include  "..\include\hd.h"
#include  "..\include\blk.h"

extern void hd_interrupt();
extern struct stBlk_dev_IO  glbBlk_devs[NR_BLK_DEV];
extern void printk(unsigned long color, const char *lpstr);
extern void fnEnd_request(int major, int update);
extern void set_trap_gate(int index, void (*fnoffset)());

/*-----------------------------该模块中函数声明开始-------------------------------*/
void fnGet_hds_Info();
int fnIsController_Ready();
int fnWin_Result();
int fnIsDrive_Busy();
void fnHd_out(unsigned int drive, unsigned int nsect, unsigned int sect,
			  unsigned int head, unsigned int cyl, unsigned int cmd, 
			  void (* intr_addr)());
void fnReset_Controller();
void fnReset_hd(unsigned int nr);
void fnRecal_intr();
void bad_rw_intr();
void fnRead_intr();
void fnWrite_intr();
void fnDo_hd_request();
/*-----------------------------该模块中函数声明开始-------------------------------*/

#define	COMPLIER_BASEADDR  0xC0000000
#define CURRENT	  (glbBlk_devs[MAJOR_NR_HD].current_request)
/* 硬盘参数及类型结构 */
struct stHDtype{
	int head;      /* 磁头个数 */
	int	sector;    /* 每磁道扇区个数 */    
	int	cylinder;  /* 柱面数 */
	int	wpcom;     /* 写前预补偿柱面号 */
	int	lzone;     /* 磁头着陆区柱面号 */
	int	ctrl;      /* 控制字节 */
}glbHDs_info[MAX_HD];

/* 硬盘的分区结构,每个硬盘分4个区, glbHDparts[0]和glbHDparts[5]描述整个硬盘 */
struct stHDpart{
	unsigned int start_sector;  /* 起始扇区号, base 0 */
	unsigned int nr_sectors;    /* 该分区占有扇区总数 */
}glbHDparts[5 * MAX_HD] = {{0, 0}, };

int HD_BIOS = (0x90080 + COMPLIER_BASEADDR);   /* setup中保存的参数表0的地址 */
unsigned int glbHds_nr;  /* 硬盘个数 */
void (* pfnDo_hd)();      /* 硬盘中断处理程序指针 */
static int flgNeed_Reset = 1;
static int flgNeed_Recalibrate = 1;


/*=====================================================*
 *F: 获取系统中硬盘参数表,并保存之(在hd_init()中调用它)
 *=====================================================*/
void fnGet_hds_Info()
{
	unsigned int i, drive;
	unsigned char cmos_hd;

	for(drive = 0; drive < MAX_HD; drive++)
	{
		glbHDs_info[drive].cylinder = *(unsigned short *)HD_BIOS;
		glbHDs_info[drive].head = *(unsigned char *)(2 + HD_BIOS);
		glbHDs_info[drive].wpcom = *(unsigned short *)(5 + HD_BIOS);
		glbHDs_info[drive].ctrl = *(unsigned char *)(8 + HD_BIOS);
		glbHDs_info[drive].lzone = *(unsigned short *)(12 + HD_BIOS);
		glbHDs_info[drive].sector = *(unsigned char *)(14 + HD_BIOS);
		HD_BIOS += 16;  /* 下一张表 */
	}
	if(glbHDs_info[1].cylinder)
		glbHds_nr = 2;  /* 有两个硬盘 */
	else
		glbHds_nr = 1;
	/* 计算出每个硬盘的起始扇区号、扇区总数 */
	for(i = 0; i < glbHds_nr; i++)
	{
		glbHDparts[i * 5].start_sector = 0;  /* 从0开始计算(逻辑上的噢) */
		glbHDparts[i * 5].nr_sectors = glbHDs_info[i].cylinder * 
			glbHDs_info[i].head * glbHDs_info[i].sector;  /* 扇区总数 */
	}
	/* 再次确认系统中的硬盘是否AT兼容的(参考p141, line124) */
	CMOS_READ(0x12, cmos_hd);
	if(cmos_hd & 0xf0)     /* 检验hd0 */
	{
		if(cmos_hd & 0x0f) /* 检验hd1 */
			glbHds_nr = 2;
		else
			glbHds_nr = 1;
	}
	else
	{
		glbHds_nr = 0;  /* 没有兼容的硬盘 */
		printk(RED, "not find compatible hard disk.");
	}
	/* 清理不兼容的硬盘分区表 */
	for(i = glbHds_nr; i < 2; i++)
	{
		glbHDparts[i * 5].start_sector = 0;
		glbHDparts[i * 5].nr_sectors = 0;
	}	    
}

/*=====================================================*
 *获得硬盘分区表,该函数需读取硬盘，所以安排在进程1中
 *暂时不实现它.
 *=====================================================*/



/*============================================================*
 *F: 循环等待硬盘控制器(HDC)就绪!检测控制器忙否,我不知道为什么
 *   还检测STATUS_READY(drive)?
 *O: 0--忙(超时), !0--就绪
 *============================================================*/
int fnIsController_Ready()
{
	int retries = 10000;
	unsigned char status;

	do{
		In_Byte(HDC_STATUS_PORT, status)
	}while(--retries && (status & (STATUS_BUSY | STATUS_READY)) != STATUS_READY);
	return retries;  /* 返回剩余值 */
}


/*=====================================================*
 *F: 检测HDC执行命令后的状态,通过读取状态寄存器中的结果状态。
 *I: void
 *O: 0--执行正常, 1--执行出错
 *=====================================================*/
int fnWin_Result()
{
	unsigned char status;

	In_Byte(HDC_STATUS_PORT, status)
	if((status & (STATUS_BUSY | STATUS_READY | STATUS_WFAULT | STATUS_ERROR))
		== STATUS_READY)
		return 0;
	if(status & STATUS_ERROR)  /* 命令执行错误 */
	{
		In_Byte(HDC_ERROR_PORT, status)  /* 将错误值清理掉! */
	}
	return 1;
}


/*==============================================================*
 *F: 等待硬盘就绪(hdc && drive),好象与fnIsController_Ready()相同
 *I: void
 *O: 0-ready, 1-等待超时
 *==============================================================*/
int fnIsDrive_Busy()
{
	int i;
	unsigned char status;

	for(i = 0; i < 10000; i++)
	{
		In_Byte(HDC_STATUS_PORT, status)
		if(STATUS_READY == (status & (STATUS_BUSY | STATUS_READY)))
			break;
	}
	/* 再次读刚才的状态值 */
	In_Byte(HDC_STATUS_PORT, status)
	status &= STATUS_BUSY | STATUS_READY;
	if(status == STATUS_READY)
		return 0;
	printk(RED, "fnIsDrive_Busy() times out!");
	return 1;
}


/*=======================================================*
 *F: 向硬盘控制器(HDC)发送命令块,全部是7B,6个参数+1个命令
 *I: 略
 *O: void
 *=======================================================*/
void fnHd_out(unsigned int drive, unsigned int nsect, unsigned int sect,
			  unsigned int head, unsigned int cyl, unsigned int cmd, 
			  void (* intr_addr)())
{
	unsigned char param;

	if(drive > 1 || head > 15)
		printk(RED, "fnHd_out(): error!");
	if(!fnIsController_Ready())
		printk(RED, "fnHd_out(): HD controller error!");
	pfnDo_hd = intr_addr;
	/* 参数:控制字节 */
	param = (unsigned char)glbHDs_info[drive].ctrl;  
	Out_Byte(HDC_CTRL_PORT, param)
	/* 参数:写预补偿柱面号( >> 2) */
	param = (unsigned char)(glbHDs_info[drive].wpcom >> 2); 
	Out_Byte(HDC_PRECOMP_PORT, param)
	/* 参数:读|写扇区总数 */
	param = (unsigned char)nsect;  
	Out_Byte(HDC_NR_SECTOR_PORT, param)
	/* 参数:起始扇区 */
	param = (unsigned char)sect;  
	Out_Byte(HDC_START_SEC_PORT, param)
	/* 参数:柱面号低8位 */
	param = (unsigned char)cyl;         
	Out_Byte(HDC_CYL_LOW_PORT, param)
	/* 参数:柱面号高8位 */
	param = (unsigned char)(cyl >> 8);  
	Out_Byte(HDC_CYL_HIGH_PORT, param)
	/* 参数:驱动器号 + 磁头号 */
	param = (unsigned char)(0xa0 | (drive << 4) | head); 
	Out_Byte(HDC_CURRENT_PORT, param)
	/* 发送命令码 */
	param = (unsigned char)cmd;
	Out_Byte(HDC_COMMAND_PORT, param)
}


/*============================================================*
 *F: 诊断复位(重新校正)硬盘控制器(HDC),没有中断处理函数对应之
 *I: void
 *O: void
 *============================================================*/
void fnReset_Controller()
{
	int i;
	unsigned char tmp;

	/* 向控制端口发送(4-复位) */
	Out_Byte(HDC_CTRL_PORT, 0x04)
	for(i = 0; i < 100; i++)  /* 延时等待 */
		NOP()
	tmp = (unsigned char)(glbHDs_info[0].ctrl & 0x0f);  /* 不禁止重试、重读 */
	Out_Byte(HDC_CTRL_PORT, tmp)
	if(fnIsDrive_Busy())
		printk(RED, "fnReset_Controller(): drive busy!");
	/* 读错误寄存器 */
	In_Byte(HDC_ERROR_PORT, tmp)
	if(tmp != 0x01)
		printk(RED, "fnReset_Controller(): reset failed!");
}


/*=====================================================*
 *F: 复位指定的硬盘nr, 先校正HDC,再发送硬盘控制器命令-->
 * “建立驱动器参数”
 *I: nr-驱动器号
 *O: void
 *=====================================================*/
void fnReset_hd(unsigned int nr)
{
	fnReset_Controller();
	fnHd_out(nr, glbHDs_info[nr].sector, glbHDs_info[nr].sector,
		glbHDs_info[nr].head - 1, glbHDs_info[nr].cylinder, 
		CMD_SPECIFY, &fnRecal_intr);  /* 放置校正中断处理函数 */
}


/*===========================================================*
 *F: 校正中断处理函数.
 *C: 如果硬盘控制器返回错误信息,则首先调用bad_rw_intr(),然后
 *   调用fnDo_hd_request()中的"复位"功能或继续处理下一个请求.
 *===========================================================*/
void fnRecal_intr()
{
	if(fnWin_Result())  /* 命令执行失败 */
		bad_rw_intr();
	/* 根据情况下达下一个命令*/
	fnDo_hd_request();
}


/*=====================================================*
 *F: 读写硬盘失败处理函数.
 *I: void
 *O: void
 *=====================================================*/
void bad_rw_intr()
{
	CURRENT->errors++;
	if(CURRENT->errors >= MAX_ERRORS)
		fnEnd_request(MAJOR_NR_HD, 0);  /* 没有更新数据 */
	if(CURRENT->errors >= MAX_ERRORS / 2)
		flgNeed_Reset = 1;              /* 需要调用fnReset_Controller() */
}


/*=====================================================*
 *F: HDC读操作, 中断处理程序
 *=====================================================*/
void fnRead_intr()
{
	unsigned char *tmp;

	/* 检测命令执行的结果 */
	if(fnWin_Result())
	{
		bad_rw_intr();
		/* 根据情况下达下一个命令*/
		fnDo_hd_request();
		return;
	}
	tmp = CURRENT->pdata_buf;
	/* 将数据读到请求结构.缓冲区 */
	Read_port(HDC_DATA_PORT, tmp, 256)
	CURRENT->errors = 0;
	if(--CURRENT->nr_sectors)       /* 还没有读完 */
	{
		CURRENT->pdata_buf += 512;  /* 下一个扇区 */
		CURRENT->begin_sector++;
		pfnDo_hd = &fnRead_intr;
		return;
	}
	fnEnd_request(MAJOR_NR_HD, 1);  /* 更新数据 */
	fnDo_hd_request();              /* 处理下一个请求项 */
}


/*=====================================================*
 *F: HDC写操作, 中断处理程序
 *=====================================================*/
void fnWrite_intr()
{
	unsigned char *tmp;

	/* 检测命令执行的结果 */
	if(fnWin_Result())
	{
		bad_rw_intr();
		/* 根据情况下达下一个命令 */
		fnDo_hd_request();
		return;
	}
	CURRENT->errors = 0;
	if(--CURRENT->nr_sectors)  /* 还没有写完 */
	{
		CURRENT->pdata_buf += 512;  /* 下一个扇区 */
		CURRENT->begin_sector++;
		pfnDo_hd = &fnWrite_intr;
		/* 写下一个扇区 */
		tmp = CURRENT->pdata_buf;
		Write_port(HDC_DATA_PORT, tmp, 256)
		return;
	}
	fnEnd_request(MAJOR_NR_HD, 1);  /* 数据合法 */
	fnDo_hd_request();  /* 处理下一个请求项 */
}


/*=====================================================*
 *F: 处理硬盘操作请求
 *fnDo_hd_request()与floppy.c中的fnDo_fd_request()的结构
 *类似,工作性质也类似.
 *=====================================================*/
void fnDo_hd_request()
{
	int i;
	char *tmp;
	unsigned char status;
	unsigned int block, dev;
	unsigned int sect, head, cyl;  /* 实际的扇区号、磁头号、柱面号 */
	unsigned int nr_sect;

	/* 检查CURRENT请求项的合法性 */
_repeat:
	if(!CURRENT)
		return;
	if(MAJOR(CURRENT->dev) != MAJOR_NR_HD)
		printk(RED, "fnDo_hd_request():dev is wrong!");
	dev = MINOR(CURRENT->dev);      /*子设备号(分区号)*/
	block = CURRENT->begin_sector;  /* 分区中的相对扇区号, base 0 */
	if(dev >= 5 * glbHds_nr || (block + 2) > glbHDparts[dev].nr_sectors)
	{
		fnEnd_request(MAJOR_NR_HD, 0);  /* 数据非法 */
		goto _repeat;
	}
	block += glbHDparts[dev].start_sector;  /* 计算出到某硬盘的绝对扇区号 base 0 */
	dev /= 5;  /* dev 是硬盘0或1 */
	/* 上面是逻辑扇区号(base 0), 转换成实际的柱面、磁头、磁道扇区(base 1) */
	sect = block % glbHDs_info[dev].sector;
	sect++;
	block /= glbHDs_info[dev].sector;      /* 逻辑磁道号 */
	head = block % glbHDs_info[dev].head;  /* 磁头号base 0 */
	cyl = block / glbHDs_info[dev].head;   /* 柱面号base 0 */
	nr_sect = CURRENT->nr_sectors;         /* 读写扇区个数 */
	if(flgNeed_Reset)
	{
		flgNeed_Reset = 0;
		flgNeed_Recalibrate = 1;
		fnReset_hd(MINOR(CURRENT->dev) / 5);  /* 也就是dev */
		return;
	}
	if(flgNeed_Recalibrate)
	{
		flgNeed_Recalibrate = 0;
		fnHd_out(dev, glbHDs_info[dev].sector, 0, 0, 0, CMD_RECALIBRATE, &fnRecal_intr);
		return;
	}
	if(CURRENT->cmd == WRITE)
	{
		fnHd_out(dev, nr_sect, sect, head, cyl, CMD_WRITE, &fnWrite_intr);
		In_Byte(HDC_STATUS_PORT, status)
		for(i = 0; i < 3000 && !(status & STATUS_DATA_REQ); i++)
			In_Byte(HDC_STATUS_PORT, status)
		/* 请求服务没有置位, 则写硬盘操作失败 */
		if(!(status & STATUS_DATA_REQ))
		{
			bad_rw_intr();
			goto _repeat;  /* 再处理一次 */
		}
		Write_port(HDC_DATA_PORT, tmp, 256)
	}
	else if(CURRENT->cmd == WRITE)
	{
		fnHd_out(dev, nr_sect, sect, head, cyl, CMD_READ, &fnRead_intr);
	}
	else
	{
		printk(RED, "fnDo_hd_request(): CURRENT->cmd error!");
	}
}


/*=====================================================*
 *F: 硬盘初始化 
 *C: 在StartMain()中调用
 *=====================================================*/
void fnHd_init()
{
	unsigned char val;

	fnGet_hds_Info();  /* 注意: 一定在BIOS数据被破坏前执行 */
	glbBlk_devs[MAJOR_NR_HD].pfn_doRequest = fnDo_hd_request;
	set_trap_gate(0x2e ,&hd_interrupt);
	/* 复位8259a主片的int2屏蔽位 */
	In_Byte(0x21, val)
	val &= 0xfb;
	Mask_8259AA(val)
	/* 复位8259a从片的int14屏蔽位 */
	In_Byte(0xa1, val)
	val &= 0xbf;
	Mask_8259AB(val)
}








