/*----------------------------------------------------------------------------*
 *关于软盘操作1.44 MB, 我的电脑只有一个软区所以支持一个
 *软区.这里的函数调用关系比较复杂,但是总的调用原则是:
 *		发送命令给FDC------>回到用户模式(FDC与CPU并行)------->FDC中断报告结果
 *-------->中断处理函数响应(作出指示) ------->   .....
 *													huangheshui	05/25/07  21:45
 *----------------------------------------------------------------------------*/
#include  "..\include\asm.h"
#include  "..\include\color.h"
#include  "..\include\floppy.h"
#include  "..\include\process.h"
#include  "..\include\blk.h"

extern struct stBlk_dev_IO  glbBlk_devs[NR_BLK_DEV];
extern void floppy_interrupt();
extern void fnVmkCpy(char *dest, char *sour, unsigned int size);
extern void fnSleepOn(struct PCB * *pwait);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);
extern void fnEnd_request(int major, int update);
extern int fnAddTimer(int jiffies, void (*pfn)());
extern void set_trap_gate(int index, void (*fnoffset)());

/*-----------------------------该模块中函数声明开始-------------------------------*/
void fnDo_transfer();
int ticks_to_floppy_on();
void floppy_on();
void floppy_off();
void do_floppy_timer();
int floppy_change();
void setup_DMA();
void output_byte(unsigned char byte);
int read_result();
void recalibrate_floppy();
void fnRecal_interrupt();
void fnReset_floppy();
void fnReset_interrupt();
void fnSeek_interrupt();
void bad_flp_intr();
void fnSetup_rw_floppy();
void fnRw_interrupt();
void fnDo_fd_request();
void fnDo_transfer();
/*-----------------------------该模块中函数声明开始-------------------------------*/

#define	COMPLIER_BASEADDR	0xC0000000
/*当前处理的请求项宏*/
#define CURRENT	  (glbBlk_devs[MAJOR_NR_FLY].current_request)
//软盘结构定义
struct floppy_struct{
	unsigned int size,
				 sect,
				 head,
				 track,
				 stretch;
	unsigned char gap,
				  rate,
				  specl;
}floppy_type = {2880, 18, 2, 80, 0, 0x1b, 0x00, 0xcf}; /* 1.44Mb软盘参数 */

struct PCB * glbpWait_motor = 0; /* 等待floppy motor的进程指针 */
int glbMon_timer;                /* 软区启动加速所需时间ticks */
int glbMoff_timer;               /* 软区关闭前的延迟时间ticks */
static int flgNeed_Recalibrate = 0; /* 标志：需要重新校正 */
static int flgNeed_Reset = 0;       /* 需要对FDC复位 */
static int flgNeed_Seek = 0;		/* 需要寻道操作 */
void (* pfnDo_floppy)();            /* 当前对软盘的操作中断处理 */

unsigned char glbCurrent_DOR = 0x0c;     /* 数字输出寄存器(允许DMA和中断请求、启动FDC) */
unsigned char glbReply_Buf[MAX_REPLIES]; /* 执行FDC命令返回结果缓冲区 */

unsigned char glbCurrent_cylinder = 255; /* 当前柱面号 */
unsigned char glbHead = 0;				 /* 磁头号 */
unsigned char glbSector = 0;             /* 扇区号 */
unsigned char glbCylinder = 0;           /* 柱面号 */
unsigned char glbCur_specl = 0xff;		 /* 步进率+磁头卸载时间 */
unsigned char glbCur_rate = 0xff;        /* 数据传输率 */
unsigned char glbCommand = 0;            /* FDC读写命令 */
unsigned char glbSeek_cylinder = 0;      /* 要寻找的柱面号(习惯叫寻道) */  

/* DMA需要用到的临时缓冲区,我不知道DMA使用的是物理地址?还是线性地址(经过分页机构) */
char glbFly_DMAbuf[1024];


/*=======================================================*
 *F: 返回软盘正常运转的<到计时>ticks
 *I: void
 *O: void
 *C: 如果此时还没有启动软盘，那么将启动它并开始计时.
 *=======================================================*/
int ticks_to_floppy_on()
{
	glbMoff_timer = 2000;                 /* 20s每次都要设置的! */
	if(!(glbCurrent_DOR & MOTOR_A_RUNING)) /* 如果电动机还没有开启 */
	{
		glbCurrent_DOR |= MOTOR_A_RUNING;
		Out_Byte(FDC_DOR_PORT, glbCurrent_DOR)
		glbMon_timer = 50;                 /* 0.5s的启动时间 */
	}
	return glbMon_timer;
}


/*=======================================================*
 *F: 开启软驱电动机，直到正常转速才算开启完毕
 *I: void
 *O: void
 *=======================================================*/
void floppy_on()
{
	while(ticks_to_floppy_on())
		fnSleepOn(&glbpWait_motor);  /* 等待到正常转速 */
}


/*=======================================================*
 *F: 关闭软驱电动机
 *I: void
 *O: void
 *=======================================================*/
void floppy_off()
{
	glbMoff_timer = 3 * 100;  /* 3s后关闭电动机 */
}


/*=======================================================*
 *F: 软驱定时处理程序，在clock intr中调用
 *I: void
 *O: void
 *=======================================================*/
void do_floppy_timer()
{
	if(!(glbCurrent_DOR & MOTOR_A_RUNING))  /* 电动机没有开启,返回 */
		return;
	if(glbMon_timer)
	{
		if(!(--glbMon_timer))          /* 已经达到正常转速 */
		{
			fnWakeUp(&glbpWait_motor); /* 唤醒等待进程 */
		}
	}
	else if(!glbMoff_timer)  /* 停转时间到 */
	{
		glbCurrent_DOR &= ~MOTOR_A_RUNING; /* 关闭电动机 */
		Out_Byte(FDC_DOR_PORT, glbCurrent_DOR)
	}
	else
		glbMoff_timer--;
}


/*=======================================================*
 *F: 检验软驱A中的软盘是否已更换
 *I: 如果更换--1, 否则--0
 *O: void
 *=======================================================*/
int floppy_change()
{
	unsigned char valdir;

	floppy_on();
	In_Byte(FDC_DIR_PORT, valdir)
	if(valdir & 0x80)
	{
		printk(BROWN, "floppy changed");
		floppy_off();
		return 1;
	}
	floppy_off();
	return 0;
}


/*=======================================================*
 *F: 初始化(启动)软盘的DMA通道,参数(DMA_READ, DMA_WRITE)
 *I: void
 *O: void
 *=======================================================*/
void setup_DMA()
{
	union {
		unsigned int pyhsic; /* 缓冲区的地址 */
		struct {
			unsigned char first; /* 低8位 */
			unsigned char second;
			unsigned char third;
			unsigned char forth; /* 高8位 */
		}bytes;
	}addr;
	unsigned char command;

	/* 我们的buffer在1M上(物理地址),获得临时DMA缓冲区物理地址 */
	addr.pyhsic = (unsigned int)glbFly_DMAbuf - COMPLIER_BASEADDR; 
	if(glbCommand == FDC_WRITE)
		fnVmkCpy(glbFly_DMAbuf, (char *)CURRENT->pdata_buf, 1024);
	if(glbCommand == FDC_READ)
		command = DMA_READ;
	else
		command = DMA_WRITE;
	/* 屏蔽DMA通道2 */
	/* 单通道屏蔽寄存器端口=0x0A, bit0~1指定DMA通道(0~3), bit2:1--屏蔽，0--允许请求 */
	Out_Byte(DMA_INIT, 4|2)
	/* 写方式字(read=0x46, write=0x4A) */
	Out_Byte(DMA_FLIPFLOP, command)
	Out_Byte(DMA_MODE, command)
	/* 写入addr */
	Out_Byte(DMA_ADDR, addr.bytes.first)
	Out_Byte(DMA_ADDR, addr.bytes.second)
	Out_Byte(DMA_TOP, addr.bytes.third) /* bit 16~19 */
	/* 写入计数count-1 */
	Out_Byte(DMA_COUNT, 0xff)
	Out_Byte(DMA_COUNT, 0x03) /* count = 0x3ff + 1 = 1024字节 */
	/* 开启DMA通道2的请求 */
	Out_Byte(DMA_INIT, 0|2)
}


/*------------------------------------------------------------------------*
 *插曲: 下面的函数提供：读写、错误控制、软盘中断
 *
 *------------------------------------------------------------------------*/							
/*=======================================================*
 *F: 向软盘控制器输出一个字节数据(命令或参数)
 *I: byte-命令字或命令参数
 *O: void
 *=======================================================*/
void output_byte(unsigned char byte)
{
	int count;
	unsigned char status;

	if(flgNeed_Reset) /* 复位就不要继续写数据了 */
		return;
	for(count = 0; count < 10000; count++)
	{
		In_Byte(FDC_STATUS_PORT, status) /* 读取状态寄存器 */
		status &= STATUS_DATA_READY | STATUS_DIRECTION; /* ready =1, direction = 0 */
		if(status == STATUS_DATA_READY)
		{
			status = byte; /* 真差劲,竟然不能直接送byte */
			Out_Byte(FDC_DATA_PORT, status) 
			return; /* 成功 */
		}
	}
	flgNeed_Reset = 1;
	printk(RED, "output_byte() failed!");
}


/*=======================================================*
 *F: 读取FDC执行的结果信息，最多7B,存放在reply_buffer[]中.
 *I: void
 *O: 返回读取的结果字节数目, -1表示出错
 *=======================================================*/
int read_result()
{
	int i = 0, count;
	unsigned char status, reply;

	if(flgNeed_Reset) /* FDC error,就不能读数据了 */
		return -1;
	for(count = 0; count < 10000; count++)
	{
		In_Byte(FDC_STATUS_PORT, status) /* 读取状态寄存器 */
		status &= STATUS_DATA_READY | STATUS_DIRECTION | STATUS_FDC_BUSY;
		if(status == STATUS_DATA_READY)
			return i; /* 此时FDC不忙、cpu->FDC说明命令执行结束了 */
		if(status == (STATUS_DATA_READY | STATUS_DIRECTION | STATUS_FDC_BUSY))
		{
			if(i >= MAX_REPLIES)
				break; /* FDC error */
			In_Byte(FDC_DATA_PORT, reply)
			glbReply_Buf[i++] = reply; /* 保存reply */
		}
	}
	flgNeed_Reset = 1;
	printk(RED, "read_result() failed!");
	return -1;
}


/*=======================================================*
 *F: 让软盘校正,向软盘控制器FDC发送校正命令和参数，并复位校正标志
 *I: void
 *O: void
 *=======================================================*/
void recalibrate_floppy()
{
	flgNeed_Recalibrate = 0;
	glbCurrent_cylinder = 0; /*校正就是将磁头归零柱面*/
	pfnDo_floppy = fnRecal_interrupt; /*处理软盘FDC校正中引发中断*/
	output_byte(FDC_RECALIBRATE);
	output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); /*发送参数:磁头号+驱动器号*/
	if(flgNeed_Reset) /*很可能发生output_byte()失败*/
		fnDo_fd_request();
}


/*=======================================================*
 *F: 处理软盘校正执行后产生的中断
 *=======================================================*/
void fnRecal_interrupt()
{
	/* 需要用命令获得执行结果 */
	output_byte(FDC_SENSEI);
	if(read_result()!=2 || (glbReply_Buf[0] & 0xe0) == 0x60) /* 校正命令异常结束 */
	{/* 应该返回两个结果字节 */
		flgNeed_Reset = 1;
	}
	else
		flgNeed_Recalibrate = 0;
	fnDo_fd_request();
}


/*=======================================================*
 *F: 复位FDC,通过将数字输出寄存器DOR's bit2置位一定时间
 *=======================================================*/
void fnReset_floppy()
{
	int i;
	unsigned char dor;

	flgNeed_Reset = 0;
	glbCur_specl = 0xff;  /* 步进率+磁头卸载时间 */
	glbCur_rate = 0xff;
	flgNeed_Recalibrate = 1;  /* 驱动器应该重新校正 */
	/* 准备reset FDC */
	pfnDo_floppy = fnReset_interrupt;
	dor = (unsigned char)(glbCurrent_DOR & ~0x04);
	Out_Byte(FDC_DOR_PORT, dor);
	for(i = 0; i < 100; i++)
		NOP()
	Out_Byte(FDC_DOR_PORT, glbCurrent_DOR);  /* 启动FDC,之前motor应该是旋转的 */
}


/*=======================================================*
 *F: 处理“复位FDC”命令产生的中断
 *=======================================================*/
void fnReset_interrupt()
{
	/* 需要用命令获得执行结果 */
	output_byte(FDC_SENSEI);
	read_result();             /* 读取命令执行结果 */
	output_byte(FDC_SPECIFY);  /* 设定参数 */
	output_byte(glbCur_specl); /* step rate time | head unload time */
	output_byte(6);            /* head load time = 6ms, DMA 有疑问 */
	fnDo_fd_request(); 
}


/*=======================================================*
 *F: 处理未知的软驱中断
 *=======================================================*/
void fnUnknown_interrupt()
{
    /* 需要用命令获得执行结果 */
	output_byte(FDC_SENSEI);
	if(read_result()!=2 || (glbReply_Buf[0] & 0xe0) == 0x60)
		flgNeed_Reset = 1;
	else
		flgNeed_Recalibrate = 1; 
}


/*=======================================================*
 *F: 处理FDC_SEEK命令执行产生的中断,FDC告诉我们可以读写操作了
 *=======================================================*/
void fnSeek_interrupt()
{
	/* 需要用命令获得执行结果 */
	output_byte(FDC_SENSEI);
	if(read_result()!=2 || (glbReply_Buf[0] & 0xf8) != 0x20 ||
		glbReply_Buf[1] != glbSeek_cylinder) /* seek错误,或没有到达要求cylinder */
	{
		bad_flp_intr();  /* 错误处理 */
		fnDo_fd_request();
		return;
	}
	glbCurrent_cylinder = glbReply_Buf[1];  /* 设置当前cylinder */
	fnSetup_rw_floppy();                    /* 可以发起读写操作了*/
}


/*=======================================================*
 *F: 处理中断中发现的错误,seek出错才算错
 *=======================================================*/
void bad_flp_intr()
{
	CURRENT->errors++;
	if(CURRENT->errors > MAX_ERRORS)
	{
		fnEnd_request(MAJOR_NR_FLY, 0);  /* 设置无效标志0 */
		return;
	}
	if(CURRENT->errors > MAX_ERRORS / 2)
		flgNeed_Reset = 1;  /* reset FDC */
	else
		flgNeed_Recalibrate = 1;  /* 要求校正 */
}


/*=======================================================*
 *F:   设置DMA并输出软盘操作命令及参数(1 CMD + 0~7 PARAM),
 *  实现读写发起功能
 *=======================================================*/
void fnSetup_rw_floppy()
{
	setup_DMA();
	pfnDo_floppy = fnRw_interrupt;
	output_byte(glbCommand);                /* 发送命令字节 */
	output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); /* 磁头号 + 驱动器号 */
	output_byte(glbCylinder);               /* 柱面号 */
	output_byte(glbHead);                   /* 磁头号 */
	output_byte(glbSector);                 /* 起始扇区 */
	output_byte(2);                         /* N=2表示512 bytes/sector */
	output_byte((unsigned char)floppy_type.sect); /* 磁道上最大的扇区号base=1 */
	output_byte(floppy_type.gap);           /* 扇区间隔长度 */
	output_byte(0xff);                      /* 无用!因为N=0时才有用 */
	if(flgNeed_Reset)
		fnDo_fd_request();
}


/*=======================================================*
 *F: 处理执行读写命令产生的中断
 *=======================================================*/
void fnRw_interrupt()
{
	if(read_result()!=7 || (glbReply_Buf[0] & 0xf8) ||
		(glbReply_Buf[1] & 0xb7) || (glbReply_Buf[2] & 0x73))
	{
		if(glbReply_Buf[1] & 0x02)
		{
			printk(RED, "write protect in floppy!");
			fnEnd_request(MAJOR_NR_FLY, 0);  /* 设置无效标志0 */
		}
		else
			 bad_flp_intr();
		fnDo_fd_request();
		return;
	}
	/* OK,开始转移数据 */
	if(glbCommand == FDC_READ)
		fnVmkCpy((char *)CURRENT->pdata_buf, glbFly_DMAbuf, 1024);
	fnEnd_request(MAJOR_NR_FLY, 1); /* 设置有效标志1 */
	fnDo_fd_request();              /* 继续操作下一个请求项 */
}


/*===============================================================*
 *F: fnDo_fd_request()是处理请求的首发函数,我希望它能够尽快返回;
 *   它有几个功能分支:校正、复位、寻道、请求下一项。有几个函数调用
 *   了它,各个调用的目的不尽相同,只要不长时间停留在ring0态就好了
 *===============================================================*/
void fnDo_fd_request()
{
	unsigned int block;

	flgNeed_Seek = 0; /* seek 是由transfer()发起的 */
	if(flgNeed_Reset) /* reset FDC功能 */
	{
		fnReset_floppy();
		return;
	}
	if(flgNeed_Recalibrate) /* 校正驱动器功能 */
	{
		recalibrate_floppy();
		return;
	}
	/* 检验请求项, 真正的处理读写请求功能 */
__repeat:
	if(!CURRENT)
		return;
	block = CURRENT->begin_sector;      /* base 0 */
	if(block + 2 > floppy_type.size)    /* 超过软盘的大小 */
	{
		fnEnd_request(MAJOR_NR_FLY, 0); /* 设置无效标志0 */
		goto __repeat;
	}
	/* 求对应在磁道上的扇区号,磁头号,磁道号,搜索磁道号 */
	glbSector = (unsigned char)(block % floppy_type.sect);   /* 磁道上的扇区号 base 0 */
	block /= floppy_type.sect;                               /* 逻辑磁道号 base 0 */
	glbHead = (unsigned char)(block % floppy_type.head);     /* 磁头号 */
	glbCylinder = (unsigned char)(block / floppy_type.head); /* 柱面号 */
	glbSeek_cylinder = (unsigned char)(glbCylinder << floppy_type.stretch); /* 寻道可能需要调整 */
	if(glbSeek_cylinder != glbCurrent_cylinder)
		flgNeed_Seek = 1;                   /* need seek */
	glbSector++;                            /* 在软盘中,实际扇区计数从1开始 */
	if(CURRENT->cmd == READ)
		glbCommand = FDC_READ;
	else if(CURRENT->cmd == WRITE)
		glbCommand = FDC_WRITE;
	else
		printk(RED, "fnDo_fd_request(): unknown cmd!");
	/* 准备:这里是不能够使用floppy_on()的因为进程已经睡眠过一次了 */
	fnAddTimer(ticks_to_floppy_on(), &fnDo_transfer);	
}


/*=======================================================*
 *F: fnDo_transfer()由定时器调用的.完成motor正常运转后的
 *   floppy访问处理
 *=======================================================*/
void fnDo_transfer()
{
	if(glbCur_rate != floppy_type.rate)
	{
		glbCur_rate = floppy_type.rate;
		Out_Byte(FDC_DCR_PORT, glbCur_rate)
	}
	if(flgNeed_Reset) /* 要求reset FDC, 应该不会执行该语句吧 */
	{
		fnDo_fd_request();
		return;
	}
	if(!flgNeed_Seek) /* 不需要seek,可以RW */
	{
		fnSetup_rw_floppy();
		return;
	}
	/* 寻道处理,告诉FDC寻道 */
	pfnDo_floppy = fnSeek_interrupt;
	if(glbSeek_cylinder)
	{
		output_byte(FDC_SEEK);
		/* 发送参数:磁头号+驱动器号 */
		output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); 
		output_byte(glbSeek_cylinder);	/* 发送参数:磁道号 */
	}
	else /* glbSeek_cylinder == 0,校正可以完成该要求 */
	{
		output_byte(FDC_RECALIBRATE);
		/* 发送参数:磁头号+驱动器号 */
		output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); 
	}
	/* 上面output_byte()可能产生reset,要求reset FDC */
	if(flgNeed_Reset) 
		fnDo_fd_request();
}


/*=======================================================*
 *F: 软驱初始化函数
 *I: void
 *O: void
 *C: 在StartMain()中被调用
 *=======================================================*/
void fnFloppy_init()
{
	unsigned char val;

	glbBlk_devs[MAJOR_NR_FLY].pfn_doRequest = fnDo_fd_request;
	set_trap_gate(0x26 ,&floppy_interrupt);
	/* 复位软盘的屏蔽位 */
	In_Byte(0x21, val)
	val &= ~0x40;
	Mask_8259AA(val)
}





