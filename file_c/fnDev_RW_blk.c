/*---------------------------------------------------*
 *说明: 为上层buffer提供底层设备的读写功能
 *
 *                      huangheshui  2007-06-18  12:38
 *---------------------------------------------------*/
#include "..\include\process.h"
#include "..\include\blk.h"
#include "fs\buffer.h"
#include "..\include\asm.h"
#include "..\include\color.h"


extern void fnSleepOn(struct PCB * *pwait);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);

/* 块设备数组 */
struct stBlk_dev_IO  glbBlk_devs[NR_BLK_DEV] = {
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0}
}; 
struct stRequestItem  glbRequest_items[NR_REQUEST]; /*IO层请求项数组*/
struct PCB * glbWaitReqitem; /*挂载等待空闲项的进程*/


/*=======================================================*
 *F: 释放锁定的缓冲区块bh
 *I: bh-缓冲区块描述符指针
 *O: void
 *=======================================================*/
void unlock_blkbuf(struct stBuf_Head *bh)
{
	if(!bh->uc_lock)
		printk(RED, "warning: free buffer being unlocked!");
	bh->uc_lock = 0;
	fnWakeUp(&bh->task_wait); /* 唤醒等待bh的进程 */
}


/*=======================================================*
 *F: 锁定指定的缓冲区块bh
 *I: bh-缓冲区块描述符指针
 *O: void
 *=======================================================*/
void lock_blkbuf(struct stBuf_Head *bh)
{
	Cli()
	while(bh->uc_lock)
		fnSleepOn(&bh->task_wait);
	bh->uc_lock = 1;
	Sti()
}


/*=======================================================*
 *F: 向指定的设备IO上添加一个请求项
 *I: pdev-块设备结构指针, preq-请求项指针
 *O: void
 *=======================================================*/
void add_request(struct stBlk_dev_IO *pdev, struct stRequestItem *preq)
{
	struct stRequestItem *tmp;

	preq->pnext = 0;
	//关中断, 防止被设备IO打断
	Cli()
	if(preq->bh)
		preq->bh->uc_dirt = 0;          /* 清空"脏"标志 */
	if(!(tmp = pdev->current_request))  /* 之前没有请求项 */
	{
		pdev->current_request = preq;
		Sti()
		(pdev->pfn_doRequest)();        /* 发起设备IO操作 */
		return;
	}
	/* 已经有其它请求项在等待,利用电梯调度算法 */
	for(; tmp->pnext; tmp = tmp->pnext)
	{
		if((IN_ORDER(tmp, preq) || !IN_ORDER(tmp, tmp->pnext)) &&
			IN_ORDER(preq, tmp->pnext)
		  )
		  break;  /* 找到了插入点 */
	}
	preq->pnext = tmp->pnext;
	tmp->pnext = preq;
	Sti()
}


/*=======================================================*
 *F: 创建请求项并插入请求队列
 *I: major-主设备号, rwcmd-读写命令字, bh-缓冲区块描述符指针
 *O: void
 *=======================================================*/
void make_request(int major, int rwcmd, struct stBuf_Head *bh)
{
	struct stRequestItem *req;

	if(!bh)
	{
		printk(RED, "warning:make_request() bh==0!");
		return;
	}
	if(rwcmd != READ && rwcmd != WRITE)
	{
		printk(RED, "error:make_request() rwcmd is error!");
		return;
	}
	lock_blkbuf(bh);
	/*检测是否有必要读写*/
	if((rwcmd == WRITE && !bh->uc_dirt) || (rwcmd == READ && bh->uc_valid))
	{
		unlock_blkbuf(bh);
		return;
	}
_repeat:
	if(rwcmd == READ)
		req = glbRequest_items + NR_REQUEST;  /* 读请求拥有的请求项多 */
	else
		req = glbRequest_items + ((NR_REQUEST * 2) / 3);  /* 写请求少1/3 */
	/* 搜索空闲项 */
	while(--req <= glbRequest_items)
		if(req->dev < 0)
			break;
	if(req < glbRequest_items)  /* 没有找到 */
	{
		fnSleepOn(&glbWaitReqitem);
		goto _repeat;
	}
	/* 填写请求项结构中的请求信息 */
	req->dev = bh->usdev_id;
	req->cmd = rwcmd;
	req->errors = 0;
	req->begin_sector = bh->uldev_blknr << 1;  /* 起始扇区 */
	req->nr_sectors = 2;
	req->pdata_buf = bh->pb_data;
	req->waiting = 0;  /* 这里没有使用 */
	req->bh = bh;
	req->pnext = 0;
	add_request(glbBlk_devs + major, req);  /* 加入相应设备的请求队列 */
}


/*=======================================================*
 *F: 结束一个IO请求项
 *I: major-主设备号, update-刷新标志
 *O: void
 *=======================================================*/
void fnEnd_request(int major, int update)
{
	if(glbBlk_devs[major].current_request->bh)
	{
		glbBlk_devs[major].current_request->bh->uc_valid = (unsigned char)update;
		unlock_blkbuf(glbBlk_devs[major].current_request->bh);
	}
	if(!update)
		printk(RED, "fnEnd_request(): update ==0\n");
	/* 释放requestitem */
	glbBlk_devs[major].current_request->dev = -1;
	glbBlk_devs[major].current_request = glbBlk_devs[major].current_request->pnext;
	fnWakeUp(&glbWaitReqitem);
}


/*=======================================================*
 *F: 读写一个块设备上的数据块,读到缓冲区块中.
 *I: rwcmd-读写命令字, bh-缓冲区块描述符指针
 *O: void
 *C: 为上层缓冲区提供服务接口.
 *=======================================================*/
void fnDev_RW_blk(int rwcmd, struct stBuf_Head *bh)
{
	unsigned int major;  /* 主设备号 */

	if((major = MAJOR(bh->usdev_id)) > NR_BLK_DEV ||
		!(glbBlk_devs[major].pfn_doRequest))
	{
		printk(RED, "warning:fnDev_RW_blk()try to read nonexistent blkdev!\n");
		return;
	}
	make_request(major, rwcmd, bh);  /* 请求读写 */
}


/*=======================================================* 
 *F: 初始化请求数组.
 *I: void
 *O: void
 *C: 在StartMain()中调用
 *=======================================================*/
void blk_dev_init()
{
	int i;

	for(i = 0; i < NR_REQUEST; i++)
	{
		glbRequest_items[i].dev = -1;
		glbRequest_items[i].pnext = 0;
	}
}





























