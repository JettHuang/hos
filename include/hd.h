/*----------------------------------------------------------*
 * 硬盘驱动程序头文件
 *
 *----------------------------------------------------------*/
#ifndef	__HARDDISK_H
#define __HARDDISK_H

/* 硬盘控制器(HDC)寄存器端口 */
#define  HDC_BASE0_PORT		 0x1f0   /* base register of controller 0 */
#define  HDC_DATA_PORT		 0x1f0   /* 数据端口 */
#define  HDC_PRECOMP_PORT	 0x1f1   /* 硬盘写前预补偿, 写 */
#define  HDC_ERROR_PORT		 0x1f1   /* error code, 读 */
#define  HDC_NR_SECTOR_PORT  0x1f2   /* 要读/写的扇区总数 */
#define  HDC_START_SEC_PORT  0x1f3   /* 起始扇区号 */
#define  HDC_CYL_LOW_PORT	 0x1f4   /* low byte of start cylinder number */
#define  HDC_CYL_HIGH_PORT	 0x1f5   /* high byte of start cylinder number */
#define  HDC_CURRENT_PORT	 0x1f6   /* 101dhhhh, d=drive, hhhh=head */
#define  HDC_STATUS_PORT	 0x1f7   /* HDC的状态, 读 */
#define  HDC_COMMAND_PORT	 0x1f7   /* 向HDC写命令端口, 写 */

#define  HDC_CTRL_PORT		 0x3f6   /* HDC控制寄存器端口 */

/* HDC状态寄存器的各个bit意义 */
#define  STATUS_BUSY		0x80	/* controller busy */
#define	 STATUS_READY		0x40	/* drive ready */
#define	 STATUS_WFAULT		0x20	/* write fault */
#define	 STATUS_SEEK_OVER	0x10	/* seek complete (obsolete) 该端口荒废 */
#define	 STATUS_DATA_REQ	0x08	/* data transfer request */
#define	 STATUS_CR_DATA	    0x04	/* corrected data, ECC校验错误 */
#define	 STATUS_INDEX		0x02	/* index pulse, 收到索引 */
#define	 STATUS_ERROR		0x01	/* error, 命令执行错误 */

/* HDC的各个命令码 */
#define  CMD_IDLE			0x00	/* for w_command: drive idle */
#define  CMD_RECALIBRATE	0x10	/* recalibrate drive(驱动器重新校正、复位) */
#define  CMD_READ			0x20	/* read data */
#define  CMD_WRITE			0x30	/* write data */
#define  CMD_VERIFY	        0x40	/* read verify, 扇区检查 */
#define  CMD_FORMAT		    0x50	/* format track, 格式化磁道 */
#define  CMD_SEEK			0x70	/* seek cylinder, 寻道 */
#define  CMD_DIAG			0x90	/* execute device diagnostics, 控制器诊断 */
#define  CMD_SPECIFY		0x91	/* specify parameters, 建立驱动器参数 */

/* 错误寄存器各个bit位的意义 */
#define	 ERROR_BAD_BLOCK	0x80	/* bad block */
#define	 ERROR_BAD_ECC		0x40	/* bad ecc bytes */
#define	 ERROR_NO_ID		0x10	/* id not found */
#define	 ERROR_ACMD		    0x04	/* aborted command */
#define	 ERROR_TRACK_ZERO	0x02	/* track zero error */
#define	 ERROR_NO_DMARK	    0x01	/* no data address mark */

/* 硬盘分区表结构 */
struct stPartition{
	unsigned char boot_ind;  /* 0x80--active */
	unsigned char sector;
	unsigned char cylinder;
	unsigned char sys_ind;
	unsigned char end_head;
	unsigned char end_sector;
	unsigned char end_cylinder;
	unsigned int start_sector;  /* starting sector 从0开始计算 */
	unsigned int nr_sectors;    /* 该分区的扇区个数 */
};

/* others */
#define  MAX_ERRORS		0x07  /* 读/写一个扇区时允许的最多出错次数 */
#define  MAX_HD			0x02  /* 系统最多支持的硬盘个数 */
#define  MAJOR_NR_HD	0x03  /* 硬盘的主设备号 */

/*
 * 下面的宏定义了从(向)HDC数据端口读(写)数据
 * port--端口号, pbuf--缓冲区地址, nr--字的个数
 */
/* 读 port --> es:edi */
#define  Read_port(port, pbuf, nr) \
__asm  mov ecx, pbuf \
__asm  mov edi, ecx \
__asm  mov ecx, nr \
__asm  mov dx, port \
__asm  cld \
__asm  rep insw

/* 写 ds:esi --> port */
#define  Write_port(port, pbuf, nr) \
__asm  mov ecx, pbuf \
__asm  mov esi, ecx \
__asm  mov ecx, nr \
__asm  mov dx, port \
__asm  cld \
__asm  rep outsw

/* 读CMOS中的参数byte,disp--参数偏移、retv--返回值 */
#define  CMOS_READ(disp, retv) \
__asm  mov al, disp \
__asm  out 70h, al \
__asm  in al, 71h \
__asm  mov retv, al


#endif /* __HARDDISK_H */

/*-----------------------补充知识开始-----------------------*
硬盘参数表描述(不全)

 Offset Size	    Description

	  00   word  maximum number of cylinders
	  02   byte  maximum number of heads
	  03   word  starting reduced write current cylinder
	  05   word  starting write pre-comp cylinder
	  07   byte  maximum ECC data burst length
	  08   byte  control byte:
 *-----------------------补充知识结束-----------------------*/



















