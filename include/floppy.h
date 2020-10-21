/*-------------------------------------------------*
 *关于软盘操作1.44 MB
 *			huangheshui	05/25/07  21:45
 *-------------------------------------------------*/
#ifndef __FLOPPY_H
#define __FLOPPY_H

/* 软盘控制器的端口 */
#define FDC_DOR_PORT		0x3f2   /* 数字输出寄存器 */
#define FDC_STATUS_PORT		0x3f4   /* 主状态寄存器 */
#define FDC_DATA_PORT		0x3f5   /* 数据、命令端口 */
#define FDC_DIR_PORT		0x3f7	/* 数字输入寄存器 */
#define FDC_DCR_PORT		0x3f7	/* 传输率控制寄存器 */

/* DMA端口 */
#define DMA_ADDR			0x004	/* port for low 16 bits of DMA address */
#define DMA_TOP				0x081	/* port for top 4 bits of 20-bit DMA addr */
#define DMA_COUNT			0x005	/* port for DMA count (count =  bytes - 1) */
#define DMA_FLIPFLOP		0x00C	/* DMA byte pointer flip-flop */
#define DMA_MODE			0x00B	/* DMA mode port */
#define DMA_INIT			0x00A	/* DMA init port */
#define DMA_RESET_VAL		0x006

/* 主状态寄存器各bit含义 */
#define STATUS_DRIVER_BUSYMASK	0x0f /* 驱动器忙 */
#define STATUS_FDC_BUSY			0x10 /* 软盘控制器忙 */
#define STATUS_DMA_USED			0x20 /* 0 -- 使用DMA */
#define STATUS_DIRECTION		0x40 /* 数据传输方向 0:cpu-->FDC, 1:FDC-->cpu */
#define STATUS_DATA_READY		0x80 /* 数据寄存器就绪 */

/* 状态字节0(ST0)各个比特位的含义 */
#define ST0_DS		0x03 /* 驱动器选择号(发生中断时) */
#define ST0_HA		0x04 /* 磁头号 */
#define ST0_NR		0x08 /* 驱动器未准备好 */
#define ST0_ECE		0x10 /* 设备检测出错(零道校准出错) */
#define ST0_SE		0x20 /* seek end 寻道或重新校正操作执行结束 */ 
#define ST0_INTR	0xc0 /* 中断代码屏蔽位 */

/* 状态字节1(ST1)各个比特位的含义 */
#define ST1_MAM		0x01 /* missing address mark, 没找到地址标志 */
#define ST1_WP		0x02 /* write protect, 写保护 */
#define ST1_ND		0x04 /* No data--unreadable, 没找到指定的扇区 */
#define ST1_OR		0x10 /* over run, 数据传输超时(DMA故障) */
#define ST1_CRC		0x20 /* CRC检验出错 */
#define ST1_EOC		0x80 /* 访问超过磁道上最大扇区号 */

/* 状态字节2(ST2)各个比特位的含义 */ 
#define ST2_MAM		0x01 /* missing address mark, 没找到地址标志 */
#define ST2_BC		0x02 /* Bad cylinder, 磁道坏 */
#define ST2_SNS		0x04 /* scan not satisfied, 检索(扫描)条件不足 */
#define ST2_SEH		0x08 /* scan equal hit, 检索条件满足 */
#define ST2_WC		0x10 /* wrong cylinder, 磁道(柱面号)不符 */
#define ST2_CRC		0x20 /* 数据场CRC校验错 */
#define ST2_CM		0x40 /* control mark = deleted, 读数据遇到删除标志 */

/* 状态字节3(ST3)各个比特位的含义 */ 
#define ST3_HA		0x04 /* head address, 磁头号 */
#define ST3_TZ		0x10 /* track zero sign, 1=track 0 零磁道信号 */
#define ST3_WP		0x40 /* 写保护*/

/*------FDC_COMMAND软盘命令码 -------*/
#define FDC_RECALIBRATE		0x07 /* move to track 0, 重新校正(磁头退到0磁道) */
#define FDC_SEEK			0x0f /* seek track, 磁头寻道 */
#define FDC_READ			0xe6 /* 读数据(MT多磁道操作、MFM格式、跳过删除数据) */
#define FDC_WRITE			0xc5 /* 写数据(MT,MFM) */
#define FDC_SENSEI			0x08 /* 检测中断状态 */
#define FDC_SPECIFY			0x03 /* 设定驱动器参数(步进速率、磁头卸载时间等) */

/* DMA command */
#define DMA_READ			0x46 /* DMA读盘 */
#define DMA_WRITE			0x4A /* DMA写盘 */

/* A盘电动机检测 */
#define MOTOR_A_RUNING		0x10
#define	DRIVER_A_SELECT		0x00
#define DRIVER_A			0x00

#define MAX_REPLIES			0x07 /* 最多7B返回结果 */
#define MAJOR_NR_FLY		0x02 /* floppy major nr */
#define MAX_ERRORS			0x08 /* 最多允许出现8次错误 */

#endif /* __FLOPPY_H */
















