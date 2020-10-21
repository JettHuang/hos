/*--------------------------------------------------------*
 *   块设备头文件, 为了保持与linux的一致，有些地方我全盘
 *使用了。
 *
 *--------------------------------------------------------*/
#ifndef __BLK_H
#define __BLK_H

#include "fs\buffer.h"

#define NR_BLK_DEV	  7     /* 块设备的数量 */
#define NR_REQUEST	  48    /* 一共有48个请求项 */

/* 请求项结构定义 */
struct stRequestItem{
	int dev;	                        /* 使用该项的设备号, -1表示空闲 */
	int cmd;	                        /* READ OR WRITE命令 */
	int errors;                         /* 操作时产生的错误次数 */
	unsigned int begin_sector;          /* 起始扇区 */
	unsigned int nr_sectors;            /* 读、写扇区数 */
	unsigned char *pdata_buf;           /* 数据缓冲区 */
	struct PCB *waiting;                /* 这里我没有用到 */
	struct stBuf_Head *bh;              /* 缓冲区块head */
	struct stRequestItem *pnext;        /* 下一个请求项 */
};

/* 块设备结构,用描述<设备操作函数>、该设备的I/O请求队列 */
struct stBlk_dev_IO{
	void (*pfn_doRequest)();                /* 设备I/O请求的处理函数 */
	struct stRequestItem *current_request;  /* 相应的请求队列 */
};


/*
 * 电梯调度算法的优先比较原则, cmd --> dev --> sector
 * s1优先与s2,则返回true
 * value read < value write
 */
#define IN_ORDER(s1, s2) \
	((s1)->cmd < (s2)->cmd || (s1)->cmd == (s2)->cmd && \
		((s1)->dev < (s2)->dev || \
		((s1)->dev == (s2)->dev && (s1)->begin_sector < (s2)->begin_sector)))

#define MAJOR(a) (((unsigned short)(a)) >> 8)  /* 高字节主设备号 */
#define MINOR(a) ((a) & 0xff)			       /* 低字节次设备号 */

#endif /*__BLK_H*/



























