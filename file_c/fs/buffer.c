/*--------------------------------------------------------------------------*
 * 说明：该文件实现对file_buffer的管理，向上层提供读取数据块服务。
 *                                      huangheshui   06/04/07  20:24
 * 备注：
 *	   从进程的观点来看，它们要读的数据块都在“文件缓冲区中”，至于
 * 真正的数据从哪里来，它们是不用操心的。
 *	  “文件缓冲区”接到进程的请求后，首先寻找目前缓冲区中是否存在匹配
 * 的合格的数据块。如果不存在，那么它向设备请求数据块，当设备满足请求
 * 后，则可以完成任务了。
 *
 * 内部函数列表：
 * 1，fnInit_Buffer(void *start, void *end);
 * 2, fnFind_bufBlock(unsigned int dev, unsigned int block);
 * 3, fnGet_bufblk_inhash( unsigned int dev, unsigned int block);
 * 4, fnGet_bufblk(unsigned int dev, unsigned int block);
 *
 * 接口函数(向上层提供服务)：
 * 1, fnRelse_bufblk(struct stBuf_Head *bh);
 * 2, fnread_bufblk(unsigned int dev, unsigned int block);
 *
 *修改: 添加设备同步函数                    2007-06-18  18:48
 *--------------------------------------------------------------------------*/
#include "..\..\include\color.h"
#include "buffer.h"


extern void fnSleepOn(struct PCB * *pwait);
extern void panic(char *szerror);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);
extern void fnDev_RW_blk(int rwcmd, struct stBuf_Head * bh);
extern void SyncMinodes();
extern unsigned int uglbTotal_PmPages;

/*-----------------------以下是该模块中的函数声明---------------------------*/
void fnInit_Buffer();
void fnRemove_from_Queues(struct stBuf_Head *bh);
void fnInsert_to_Queues(struct stBuf_Head *bh);
struct stBuf_Head * fnFind_bufBlock(unsigned short dev, unsigned int block);
void WaitonBuffer(struct stBuf_Head *bh);
struct stBuf_Head * fnGet_bufblk_inhash(unsigned short dev, unsigned int block);
struct stBuf_Head * fnGet_bufblk(unsigned short dev, unsigned int block);
void fnRelse_bufblk(struct stBuf_Head *bh);
struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block);
/*-----------------------以上是该模块中的函数声明---------------------------*/

#define	COMPLIER_BASEADDR  0xC0000000
#define	NR_HASH  32 
#define	IndexHash(dev, blknr)	((unsigned int)((dev) ^ (blknr)) % NR_HASH)
#define	Hash(dev, blknr)	glbHash_table[IndexHash(dev, blknr)]
	
/* 用于管理buffer的全局变量 */
struct stBuf_Head *	glbHash_table[NR_HASH];
struct stBuf_Head *glbFree_list;   /* 空闲块链表头指针 */
struct PCB *glbWait_Bufblk;
void *start, *end;
int NR_HEARDS = 0;
 
/*==========================================================*
 *F: 指定一段内存作为file buffer,该函数初始化buffer
 *I: void
 *O: void
 *==========================================================*/
void fnInit_Buffer()
{
	struct stBuf_Head *bh;
	unsigned int pblk, i;

	//缓冲区从kernel pagetable后 ~ 0x400000前
	NR_HEARDS = 0;
	start = (void *)(uglbTotal_PmPages * sizeof(int) + COMPLIER_BASEADDR + 0x100000);
    end = (void *)(COMPLIER_BASEADDR + 0x400000);
	bh = (struct stBuf_Head *)start;
	pblk = (unsigned int)end;
	/* blk的开端 >= (bh + 1)的开端 */
	while((pblk -= BLOCK_SIZE) >= (unsigned int)(bh + 1))
	{
		bh->usdev_id = 0;
		bh->uldev_blknr = 0;
		bh->uc_dirt = 0;
		bh->uc_lock = 0;
		bh->uc_valid = 0;
		bh->us_count = 0;
		bh->bh_next = 0;
		bh->bh_prev = 0;
		bh->pb_data = (unsigned char *)pblk ;
		bh->bh_next_free = bh + 1;  /* 对于最后一个bh需要修改 */
		bh->bh_prev_free = bh - 1;  /* 对于第一个bh需要修改 */
		bh++;
		NR_HEARDS++;
	}
	bh--;	/* bh--是最后一个buf_head */
	glbFree_list = (struct stBuf_Head *)start;
	glbFree_list->bh_prev_free = bh;
	bh->bh_next_free = glbFree_list;  /* 构成了循环链表 */
	/* 初始化hash表 */
	for(i = 0; i < NR_HASH; i++)
		glbHash_table[i] = 0;
}


/*==========================================================*
 *F: 将bh从hash表和free_list删除
 *I: struct stBuf_Head * 
 *O: void
 *==========================================================*/
void fnRemove_from_Queues(struct stBuf_Head *bh)
{
	if(!bh)
		return ;
	/* 首先从hash中删除,hash上的的链表不是循环的 */
	if(bh->bh_prev)
		bh->bh_prev->bh_next = bh->bh_next;
	if(bh->bh_next)
		bh->bh_next->bh_prev = bh->bh_prev;
	/* 判断bh是否为第一个元素 */
	if(Hash(bh->usdev_id, bh->uldev_blknr) == bh)
		Hash(bh->usdev_id, bh->uldev_blknr) = bh->bh_next;
	if(!(bh->bh_prev_free) || !(bh->bh_next_free))  /* 这是不可能的 */
		panic("fnRemove_from_Queues(): free hd just only one");
	/* 从free_list上删除,注意空闲表上此时拥有所有的buf_head(临时拆下,然后又安上), 它是个循环链表 */
	bh->bh_prev_free->bh_next_free = bh->bh_next_free;
	bh->bh_next_free->bh_prev_free = bh->bh_prev_free;
	if(glbFree_list == bh)
		glbFree_list = bh->bh_next_free;
}


/*==========================================================*
 *F: 将bh插入到hash表的头部和free_list的尾部.
 *I: struct stBuf_Head * 
 *O: void
 *C: 此插入方法体现了RLU策略
 *==========================================================*/
void fnInsert_to_Queues(struct stBuf_Head *bh)
{
	if(!bh)
		return;
	/* 插入free_list尾部,因为它将最后被替换掉RLU */
	bh->bh_prev_free = glbFree_list->bh_prev_free;
	bh->bh_next_free = glbFree_list;
	glbFree_list->bh_prev_free->bh_next_free = bh;
	glbFree_list->bh_prev_free = bh;
	bh->bh_prev = 0;
	bh->bh_next = 0;
	/* 插入hash队头部, 0设备保留 */
	if(!bh->usdev_id)
		return;
	bh->bh_next = Hash(bh->usdev_id, bh->uldev_blknr);
	Hash(bh->usdev_id, bh->uldev_blknr) = bh;
	if(bh->bh_next != 0)
		bh->bh_next->bh_prev = bh;
}


/*==========================================================*
 *F: 在HASH表里查找指定设备的blk.
 *I: dev, block
 *O: 找到返回bh的指针，否则0.
 *C: 该函数只检测符合要求的bh是否存在.
 *==========================================================*/
struct stBuf_Head * fnFind_bufBlock(unsigned short dev, unsigned int block) 
{
	struct stBuf_Head *tmp;

	for(tmp = Hash(dev, block); tmp != 0; tmp = tmp->bh_next)
		if((tmp->uldev_blknr == block) && (tmp->usdev_id == dev))
			return tmp;
	return	0;
}


/*==========================================================*
 *等待指定的bh解锁
 *==========================================================*/
void WaitonBuffer(struct stBuf_Head *bh)
{
	while(bh->uc_lock)
		fnSleepOn(&bh->task_wait);
}


/*==========================================================*
 *F: 在HASH表里获得一个可以使用的(dev,blk)缓冲块.
 *I: dev, block
 *O: 如果获得返回bh的指针,否则0.
 *C: 调用该函数的进程可能会因等待缓冲块而休眠.
 *==========================================================*/
struct stBuf_Head * fnGet_bufblk_inhash(unsigned short dev, unsigned int block)
{
	struct stBuf_Head *bh;
		
	/* 先检查是否存在 */
	if(!(bh = fnFind_bufBlock(dev, block)))
		return  0;
	bh->us_count++;
	/* 等待缓冲区,不能用则进入sleep状态 */
	WaitonBuffer(bh);
	/* 我认为没有必要再检查 dev、block 的正确性了,因为bh->us_count++; */
	return bh;
}


/*==========================================================*
 *F: 从整个buffer中获得一个可以使用的(dev,blk)缓冲块.
 *I: dev, block
 *O: 获得返回bh的指针
 *C:    该函数可能会因等待缓冲块而休眠,首先在hash中寻找已经存在的buffer,
 *  如果找不到的话，在去free_list中获得一个空闲块，很有可能会让一个blk
 *  写盘(缓写),如果free_list没有空闲块, 那么将因等待buffer而sleep. 获得的
 *  bufblk是否可以直接使用,需要检测bh->isvalid.
 *===========================================================*/
struct stBuf_Head * fnGet_bufblk(unsigned short dev, unsigned int block)
{
	struct stBuf_Head *bh, *tmp;

_repeat:
	/* 首先搜索hash */
	if(bh = fnGet_bufblk_inhash(dev, block))
		return bh;
	/* 在free_list上获得一个bufblk */
	tmp = glbFree_list;
	do{
		if(tmp->us_count != 0)
			continue;	/* 继续检测下一个bh */
		bh = tmp;
		break;
	}while((tmp = tmp->bh_next_free) != glbFree_list);
    /* 没有空闲的buf_blk */
	if(!bh)
	{
		fnSleepOn(&glbWait_Bufblk);  /* 等待空闲bh */
		goto _repeat;
	}
	/* 等待缓冲区解锁 */
	WaitonBuffer(bh);
	if(bh->us_count)  /* 可能被其它进程抢先占用(被挂在它前面) */
		goto _repeat;
	if(bh->uc_dirt)
	{
		fnDev_RW_blk(WRITE, bh);  /* 写盘 */
		WaitonBuffer(bh);  
		/* 可能被其它进程先占用, 其它进程可能执行到上面的WaitonBuffer(bh); */
		if(bh->us_count)
			goto _repeat;
	}
	/* 就在刚才睡眠的时候, 很有可能其它进程获得了该进程想找的bh(H；是可能发生的) */
	if(fnFind_bufBlock(dev, block))
	{
		fnRelse_bufblk(bh);  /* 释放新得的bh */
		goto _repeat;
	}
	/* 现在我们获得了一个可用的块,init it! */
	bh->us_count = 1;
	bh->uc_dirt = 0;
	bh->uc_valid = 0;
	/* 暂时删除 */
	fnRemove_from_Queues(bh);
	/* 设置设备号和blknr */
	bh->usdev_id = dev;
	bh->uldev_blknr = block;
	/* 重新插入 */
	fnInsert_to_Queues(bh);
	return bh;
}


/*==========================================================*
 *F: 同步指定设备的缓冲区块与设备逻辑块.
 *I: dev-设备号
 *O: void
 *==========================================================*/
void SyncDevce(int dev)
{
	struct stBuf_Head *bh;
	int i;

	bh = (struct stBuf_Head *)start;
	for(i = 0; i < NR_HEARDS; i++, bh++)
	{
		if(bh->usdev_id != dev)
			continue;
		WaitonBuffer(bh);
		if(bh->usdev_id == dev && bh->uc_dirt)
			fnDev_RW_blk(WRITE, bh);
	}
	SyncMinodes(); //同步文件Minode
	//再次同步, 避免漏掉同步节点
	bh = (struct stBuf_Head *)start;
	for(i = 0; i < NR_HEARDS; i++, bh++)
	{
		if(bh->usdev_id != dev)
			continue;
		WaitonBuffer(bh);
		if(bh->usdev_id == dev && bh->uc_dirt)
			fnDev_RW_blk(WRITE, bh);
	}
}


/*==========================================================*
 *F: 同步缓冲区中的所有块与设备逻辑块.
 *I: void
 *O: void
 *==========================================================*/
void SyncAllDev()
{
	struct stBuf_Head *bh;
	int i;

	SyncMinodes(); //同步文件Minode
	//再次同步, 避免漏掉同步节点
	bh = (struct stBuf_Head *)start;
	for(i = 0; i < NR_HEARDS; i++, bh++)
	{
		WaitonBuffer(bh);
		if(bh->uc_dirt)
			fnDev_RW_blk(WRITE, bh);
	}
}


/*==========================================================*
 *F: 将指定的设备在缓冲区中的数据块无效.
 *I: dev-设备号
 *O: void
 *==========================================================*/
void InvalidateBuffer(int dev)
{
	struct stBuf_Head *bh;
	int i;

	bh = (struct stBuf_Head *)start;
	for(i = 0; i < NR_HEARDS; i++, bh++)
	{
		if(bh->usdev_id != dev)
			continue;
		WaitonBuffer(bh);
		if(bh->usdev_id == dev)
			bh->uc_valid = bh->uc_dirt = 0;  //无效标志
	}
}


/*------------------------下面两个函数是向上层提供服务的接口函数-------------------*/
/*==========================================================*
 *F: 释放指定的缓冲块.
 *I: struct stBuf_Head * bh
 *O: void
 *==========================================================*/
void fnRelse_bufblk(struct stBuf_Head *bh)
{
	if(!bh)
		return;
	/* 等待缓冲区解锁 */
	WaitonBuffer(bh);
	if(!(bh->us_count--))
		panic("fnRelse_bufblk(): bh->us_count is 0");
	/* 唤醒等待空闲bh的进程 */
	fnWakeUp(&glbWait_Bufblk);
}


/*==========================================================*
 *F: 读设备上的数据块.
 *I: dev, block
 *O: void
 *==========================================================*/
struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block) 
{
	struct stBuf_Head *bh;

	if(!dev)
		return 0;
	if(!(bh = fnGet_bufblk(dev, block)))
		panic("fnread_bufblk(): returned bh is 0\n");
	if(bh->uc_valid)  /* 数据块仍然有效 */
		return bh;
	/* 请求设备读盘 */
	fnDev_RW_blk(READ, bh);  /* 读写盘时会上锁 */
	/* 等待缓冲区解锁 */
	WaitonBuffer(bh);
	if(bh->uc_valid)
		return bh;
	fnRelse_bufblk(bh);  /* 读盘失败,释放bh */
	printk(RED, "fnread_bufblk(): can't get valid bh\n");
	return 0;
}






