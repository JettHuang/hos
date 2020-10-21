#ifndef __BUFFER_H
#define __BUFFER_H

/* 定义buffer_head的结构 */
struct stBuf_Head{
	unsigned char *pb_data;     /* 缓冲区中的数据块指针 */
	unsigned long uldev_blknr;	/* 某设备上的逻辑块号 */
	unsigned short usdev_id;	/* 设备id */
	unsigned char uc_valid;     /* 0 - invalid,1- valid 对应数据块是否有效，比如设备读失败时无效 */
	unsigned char uc_dirt;	    /* 0 - clean , 1- dirty */
	unsigned short us_count;    /* 当前该块被引用的次数, 0 -- free */
	unsigned char  uc_lock;	    /* 0 - not locked , 1 - locked ;读写盘时需要锁定 */
	struct PCB *task_wait;      /* 等待该块的进程 */
	struct stBuf_Head *bh_prev; 
	struct stBuf_Head *bh_next;
/* 实际上,free_list 连接了所有缓冲块 */
	struct stBuf_Head *bh_prev_free;
	struct stBuf_Head *bh_next_free;
};

#define READ			0x1000
#define WRITE			0x2000
#define	BLOCK_SIZE	    1024	

#endif /*__BUFFER_H*/