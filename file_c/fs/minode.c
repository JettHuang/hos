/*----------------------------------------------------------------------*
 *minode.c模块功能是:管理minode[]表,上层文件操作都是建立在minode的基础
 *上的,也就是说要操作一个文件必须将对应的dinode放到minode内存表中去,
 *一旦没有空闲的表项,将会导致失败. 从这一点看:文件的打开数量是受限制的.
 *											huangheshui 06/06/07 11:55
 *                                          2007-06-17 16:09
 *----------------------------------------------------------------------*/
#include "fs.h"
#include "buffer.h"


extern void panic(char *szerror);
extern struct stMSuperBlock * GetSuperblock(int dev);
extern void fnRelse_bufblk(struct stBuf_Head *bh);
extern void SetMem(unsigned char *blkaddr, int size, unsigned char nval);
extern struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block);
extern void FreeDinode(struct stMinode *mi);
extern void fnSleepOn(struct PCB * *pwait);
extern void fnWakeUp(struct PCB * *pwait);
extern int NewDataBlock(unsigned short dev);
extern void Truncate(struct stMinode *mi);

/*--------------------------下面是该模块中的函数声明-----------------------*/
void WaitonMinode(struct stMinode *mi);
void LockMinode(struct stMinode *mi);
void UnlockMinode(struct stMinode *mi);
void MiPut(struct stMinode * mi);
struct stMinode * GetEmptyMinode();
struct stMinode * Miget(int dev, int nr);
void ReadfromDinode(struct stMinode *mi);
void WritetoDinode(struct stMinode *mi);
/*--------------------------上面是该模块中的函数声明-----------------------*/

extern struct stMSuperBlock glbMsbtable[SUPER_NR]; 
struct stMinode glbMinodetable[MINODE_NR] = {0, };

/*=======================================================*
 *F: 等待指定的minode解锁.
 *I: mi
 *O: void
 *=======================================================*/
void WaitonMinode(struct stMinode *mi)
{
	while(mi->i_lock)
		fnSleepOn(&mi->i_wait);
}

/*=======================================================*
 *F: 对指定的minode加锁.
 *I: mi
 *O: void
 *=======================================================*/
void LockMinode(struct stMinode *mi)
{
	while(mi->i_lock)
		fnSleepOn(&mi->i_wait);
	mi->i_lock = 1;
}

/*=======================================================*
 *F: 对指定的minode解锁.
 *I: mi
 *O: void
 *=======================================================*/
void UnlockMinode(struct stMinode *mi)
{
	mi->i_lock = 0;
	fnWakeUp(&mi->i_wait);
}


/*=======================================================*
 *F: 同步所有的minodes,将会引起bh写盘
 *I: void
 *O: void
 *C: 这里的同步指的是将Minode与buffer中同步, 然后buffer由
 *   buffer.c中的函数保持与设备同步.
 *=======================================================*/
void SyncMinodes()
{
	int i;
	struct stMinode *mi;

	mi = glbMinodetable + 0;
	for(i = 0; i < MINODE_NR; i++, mi++)
	{
		WaitonMinode(mi);
		if(mi->i_dirt)
			WritetoDinode(mi);
	}
}


/*=======================================================*
 *F: 释放一个minode.可能需要写盘
 *I: mi
 *O: void
 *=======================================================*/
void MiPut(struct stMinode *mi)
{
	if(!mi)
		return;
	WaitonMinode(mi);
	if(!mi->i_count)
		panic("MiPut():mi->i_count == 0");
	if(!mi->i_dev)  /* 设备号==0 */
	{
		mi->i_count--;
		return;
	}
	if(mi->i_count > 1)  /* 还有它人使用 */
	{
		mi->i_count--;
		return;
	}
	/* 如果mi->i_nlinks == 0说明该dinode没有被文件使用 */
	if(!mi->i_nlinks)
	{
		/* 释放该dinode拥有的磁盘数据块,将这个文件节点也丢掉 */
		Truncate(mi);
		FreeDinode(mi);
		return;
	}
	if(mi->i_dirt)
		WritetoDinode(mi);
	mi->i_count--;  /* 至此i_count应该为0 */
}


/*=======================================================*
 *F: 从内存中的minode表中获得一个空闲的节点槽.
 *I: void
 *O: stMinode *
 *C: 不存在等待空闲项现象,没有找到就panic
 *=======================================================*/
struct stMinode * GetEmptyMinode()
{
	struct stMinode *mi, *lastmi;
	int i;

	lastmi = glbMinodetable;
	do{
		mi = 0;
		for(i = MINODE_NR; i; i--)
		{
			if(++lastmi >= glbMinodetable + MINODE_NR)
				lastmi = glbMinodetable;  /* 从头开始 */
			if(!lastmi->i_count)
			{
				mi = lastmi;
				break;
			}
		}
		if(!mi)  /* 没有找到 */
			panic("GetEmptyMinode(): mi is 0");
		WaitonMinode(mi);
		if(mi->i_dirt)
			WritetoDinode(mi);
	}while(mi->i_count);  /* 空欢喜 */
	SetMem((unsigned char *)mi, sizeof(struct stMinode), 0);
	mi->i_count = 1;
	return mi;
}


/*=======================================================*
 *F: 从minode表中获得指定的minode
 *I: dev-设备号, nr-节点号
 *O: stMinode *
 *=======================================================*/
struct stMinode * Miget(int dev, int nr)
{
	struct stMinode *mi, *empty;

	if(!dev)
		panic("Miget(): dev is 0");
	/* 先预备一个minode槽 */
	empty = GetEmptyMinode();
	/* 扫描整个minode表,是已经否存在符合要求的minode */
	mi = glbMinodetable;
	while(mi < glbMinodetable + MINODE_NR)
	{
		if(mi->i_dev != dev || mi->i_num != nr)
		{
			mi++;
			continue;
		}
		WaitonMinode(mi);  /* 找到目标,等待解锁 */
		if(mi->i_dev != dev || mi->i_num != nr)
		{
			mi = glbMinodetable;  /* 被其它进程抢去了,重新开始 */
			continue;
		}
		mi->i_count++;
		/* 检测该minode是否为其它文件系统的安装点 */
		if(mi->i_mount)
		{
			int i;

			for(i = 0; i < SUPER_NR; i++)
				if (glbMsbtable[i].s_imount == mi)
					break;
			if(i >= SUPER_NR) 
			{
				panic("Mounted inode hasn't got sb");
				if (empty)
					MiPut(empty);
				return mi;
			}
			/* 找到了文件系统的msuper_block */
			MiPut(mi);  /* 为什么要释放这个挂载点,万一被 写盘了谁来与superblock对应 */
			dev = glbMsbtable[i].s_dev;
			nr = ROOT_INO;
			mi = glbMinodetable;
			/* 查找该文件系统的根节点, 因为<挂载点>等价于<根节点> */
			continue;
		}
		if (empty)
			MiPut(empty);
		return mi;
	}
	/* 没有在内存表中找到符合要求的minode,那么从设备上读取 */
	if(!empty)
		return 0;
	mi = empty;
	mi->i_dev = (unsigned short)dev;
	mi->i_num = (unsigned short)nr;
	ReadfromDinode(mi);
	return mi;
}


/*=======================================================*
 *F: 从设备上读取dinode --> minode;
 *I: stMinode *mi
 *O: void
 *=======================================================*/
void ReadfromDinode(struct stMinode *mi)
{
	struct stMSuperBlock *sb;  /* 内存中的超级块指针 */
	struct stBuf_Head *bh;
	int block;

	LockMinode(mi);
	/* 获得某个设备上fs的超级块 */
	if(!(sb = GetSuperblock(mi->i_dev)))
		panic("ReadfromDinode(): super not exist!");
	/* 计算该dinode在磁盘上的逻辑块号 */
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(mi->i_num -1) / INODES_PER_BLOCK;
	if(!(bh = fnread_bufblk(mi->i_dev, block)))
		panic("ReadfromDinode(): bh is 0");
	/* 复制dinode --> minode */
	*(struct stDinode *)mi = 
		((struct stDinode *)bh->pb_data)[(mi->i_num - 1) % INODES_PER_BLOCK];
	fnRelse_bufblk(bh);
	UnlockMinode(mi);
}


/*=======================================================*
 *F: 向设备上写入minode --> dinode(minode ---> bh ---> 设备)
 *I: stMinode *mi
 *O: void
 *=======================================================*/
void WritetoDinode(struct stMinode *mi)
{
	struct stMSuperBlock *sb;  /* 内存中的超级块指针 */
	struct stBuf_Head *bh;
	int block;

	LockMinode(mi);
	if(!mi->i_dirt || !mi->i_dev)
	{
		UnlockMinode(mi);
		return;
	}
	/* 获得某个设备上fs的超级块 */
	if(!(sb = GetSuperblock(mi->i_dev)))
		panic("ReadfromDinode(): super not exist!");
	/* 计算该dinode在磁盘上的逻辑块号 */
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(mi->i_num -1) / INODES_PER_BLOCK;
	if(!(bh = fnread_bufblk(mi->i_dev, block)))
		panic("ReadfromDinode(): bh is 0");
	/* 复制dinode --> minode */
	((struct stDinode *)bh->pb_data)[(mi->i_num - 1) % INODES_PER_BLOCK]
		= *(struct stDinode *)mi;
	bh->uc_dirt = 1;  /* 需要写盘更新 */
	mi->i_dirt = 0;
	fnRelse_bufblk(bh);
	UnlockMinode(mi);
}


/*=======================================================*
 *F: 将文件中的逻辑数据块--->设备上的逻辑数据块
 *I: stMinode *mi, fileblk, createflg
 *O: 设备上的逻辑数据块号
 *C: 如果创建标志置位,则在对应的逻辑块不存在时就申请新磁盘块
 *=======================================================*/
int BlkmapFiletoDev(struct stMinode *mi, int fileblk, int createflg)
{
	struct stBuf_Head *bh;
	int i;

	if(fileblk < 0)
		panic("BlkmapFiletoDev(): fileblk < 0");
	if(fileblk >= 7 + 512 + 512 * 512)
		panic("BlkmapFiletoDev(): fileblk >= 7 + 512 + 512 * 512");  /* >=最大文件块数 */
	if(fileblk < 7)  /* 该文件块使用直接块表示 */
	{
		if(createflg && !mi->i_zone[fileblk])
			if(mi->i_zone[fileblk] = (unsigned short)NewDataBlock(mi->i_dev))
			{
				mi->i_ctime = 0;
				mi->i_dirt = 1;
			}
		return mi->i_zone[fileblk];
	}
	/* 如果fileblk >=7 && < 7 + 512 则使用一次间接块 */
	fileblk -= 7;
	if(fileblk < 512)
	{
		if(createflg && !mi->i_zone[7])  /* 分配一次间接块 */
			if(mi->i_zone[7] = (unsigned short)NewDataBlock(mi->i_dev))
			{
				mi->i_ctime = 0;
				mi->i_dirt = 1;
			}
		if(!mi->i_zone[7])  /* 分配一次间接块失败 */
			return 0;
		/* 读取一次间接块 */
		if(!(bh = fnread_bufblk(mi->i_dev, mi->i_zone[7])))
			return 0;
		i = ((unsigned short *)(bh->pb_data))[fileblk];
		/* 检测是否需要申请新磁盘块 */
		if(createflg && !i)
			if(i = NewDataBlock(mi->i_dev))
			{
				((unsigned short *)(bh->pb_data))[fileblk] = (unsigned short)i;
				bh->uc_dirt = 1;
			}
		/* 释放bh,返回盘块号 */
		fnRelse_bufblk(bh);
		return i;
	}
	/* 下面是二次间接块的处理 */
	fileblk -= 512;
	if(createflg && !mi->i_zone[8])
		if(mi->i_zone[8] = (unsigned short)NewDataBlock(mi->i_dev))
		{
			mi->i_ctime = 0;
			mi->i_dirt = 1;
		}
	if(!mi->i_zone[8])  /* 申请磁盘失败,或不需要创建 */
		return 0;
	if(!(bh = fnread_bufblk(mi->i_dev, mi->i_zone[8])))
		return 0;
	/* 取二次间接块上的第一次间接块[fileblk/512]的值 */
	i = ((unsigned short *)(bh->pb_data))[fileblk >> 9];
	if(createflg && !i)
		if(i = NewDataBlock(mi->i_dev))
		{
			((unsigned short *)(bh->pb_data))[fileblk >> 9] = (unsigned short)i;
			bh->uc_dirt = 1;
		}
	/* 释放bh */
	fnRelse_bufblk(bh);
	if(!i)  /* 申请二次间接块失败,或不需创建 */
		return 0;
	if(!(bh = fnread_bufblk(mi->i_dev, i)))
		return 0;
	/* 取第二个间接块中的对应项值 */
	i = ((unsigned short *)(bh->pb_data))[fileblk & 511];
	if(createflg && !i)
		if(i = NewDataBlock(mi->i_dev))
		{
			((unsigned short *)(bh->pb_data))[fileblk & 511] = (unsigned short)i;
			bh->uc_dirt = 1;
		}
	/* 释放bh */
	fnRelse_bufblk(bh);
	return i;
}


/*=======================================================*
 *F: 非创建地获得文件数据块的磁盘块号
 *I: mi-文件内存节点指针, fileblk-文件块号
 *O: 设备上的逻辑数据块号
 *C: 对应的逻辑块不存在时就返回0
 *=======================================================*/
int Bmap(struct stMinode *mi, int fileblk)
{
	return BlkmapFiletoDev(mi, fileblk, 0);
}


/*=======================================================*
 *F: 创建地获得文件数据块的磁盘块号
 *I: mi-文件内存节点指针, fileblk-文件块号
 *O: 设备上的逻辑数据块号
 *C: 对应的逻辑块不存在时申请磁盘失败就返回0
 *=======================================================*/
int CreateBmap(struct stMinode *mi, int fileblk)
{
	return BlkmapFiletoDev(mi, fileblk, 0);
}























