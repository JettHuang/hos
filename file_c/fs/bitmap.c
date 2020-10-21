/*-----------------------------------------------------------*
 *bitmap.c的功能是操作块设备上的<位图>,来管理设备的存储空间.
 *							huanghehsui  06/05/07  21:22
 *                                       2007-06-18 17:20
 *-----------------------------------------------------------*/
#include "fs.h"
#include "buffer.h"
#include "..\..\include\color.h"

extern void panic(char *szerror);
extern struct stMSuperBlock * GetSuperblock(int dev);
extern struct stBuf_Head * fnGet_bufblk(unsigned short dev, unsigned int block);
extern void fnRelse_bufblk(struct stBuf_Head *bh);
extern struct stBuf_Head * fnGet_bufblk_inhash(unsigned short dev, unsigned int block);
extern struct stMinode * GetEmptyMinode();
extern void MiPut(struct stMinode * mi);


/*=======================================================*
 *F: 设置从指定地址开始, 偏移值offset处的bit(bit <- 1);
 *I: startaddr, offset
 *O: 该bit的原值(0|1)
 *C: 注意边界值，没有检查它
 *=======================================================*/
int Setbit(unsigned char *startaddr, unsigned int offset)
{
	unsigned int ibyte;   /* 该位所在的字节索引值 */
	unsigned int tmp;
	
	if(!startaddr)
		return 1;  //注意通过这种方式报错
	ibyte = offset / (sizeof(char) * 8);
    offset &= (sizeof(char) * 8) -1;
	tmp = startaddr[ibyte] & (0x01 << offset);
	startaddr[ibyte] |= (0x01 << offset);
	if(tmp)
		return 1;
	return 0;
}


/*=======================================================*
 *F: 复位从指定地址开始, 偏移值offset处的bit(bit <- 0);
 *I: startaddr, offset
 *O: 该bit的原值(0|1)
 *=======================================================*/
int Resetbit(unsigned char *startaddr, unsigned int offset)
{
	unsigned int ibyte;   /* 该位所在的字节索引值 */
	unsigned int tmp;
	
	if(!startaddr) 
		return 0;  //注意通过这种方式报错
	ibyte = offset / (sizeof(char) * 8);
    offset &= (sizeof(char) * 8) -1;
	tmp = startaddr[ibyte] & (0x01 << offset);
	startaddr[ibyte] &= ~(0x01 << offset);
	if(tmp)
		return 1;
	return 0;
}

/*=======================================================*
 *F: 测试从指定地址开始, 偏移值offset处的bit值;
 *I: startaddr, offset
 *O: 该bit的值(0|1)
 *C: 如果测试不存在的bit那么将返回1(占用标志)
 *=======================================================*/
int Testbit(unsigned char *startaddr, unsigned int offset)
{
	unsigned int ibyte;   /* 该位所在的字节索引值 */
	unsigned int tmp;
	
	if(!startaddr) 
		return 1;  //注意通过这种方式报错
	ibyte = offset / (sizeof(char) * 8);
    offset &= (sizeof(char) * 8) -1;
	tmp = startaddr[ibyte] & (0x01 << offset);
	if(tmp)
		return 1;
	return 0;
}


/*=======================================================*
 *F: 查找从指定地址开始,第一个为0的bit
 *I: startaddr
 *O: 该bit的偏移值.
 *=======================================================*/
int FindFirstZerobit(unsigned char *startaddr)
{
	unsigned int offset;

	/* startaddr指向的字符长度是1kb */
	for(offset = 0; offset < 8192; offset++)
		if(0 == (startaddr[offset / (sizeof(char) * 8)] & (0x01 << (offset % (sizeof(char) * 8)))))
			break;
	return offset;
}


/*=======================================================*
 *F: 清空从指定的一个数据块
 *I: blkaddr
 *O: void
 *=======================================================*/
void ClearBlock(unsigned int *blkaddr)
{
	int i;

	for(i = 0; i < 1024 / sizeof(unsigned int); i++)
		*blkaddr++ = 0;
}


/*=======================================================*
 *F: 设置一个内存块内容
 *I: blkaddr, size(byte), nval
 *O: void
 *=======================================================*/
void SetMem(unsigned char *blkaddr, int size, unsigned char nval)
{
	int i;

	for(i =0; i < size; i++)
		*blkaddr++ = nval;
}


/*=======================================================*
 *F: 释放设备dev上数据区中的逻辑块block
 *I: dev, block
 *O: void
 *=======================================================*/
void FreeDataBlock(unsigned short dev, int block)
{
	struct stMSuperBlock *sb;  /* 内存中的超级块指针 */
	struct stBuf_Head *bh;

	/* 获得某个设备上fs的超级块 */
	if(!(sb = GetSuperblock(dev)))
		panic("FreeDataBlock(): super not exist!");
	/* 检测block是否为数据区的逻辑块号 */
	if(block < sb->s_firstdatazone || block >= sb->s_nzones)
		panic("FreeDataBlock():block is not in datazone");
	/* 如果缓冲区中存在该block,就释放bh */
	bh = fnGet_bufblk_inhash(dev, block);
	if(bh)
	{
		if(bh->us_count !=1)
			panic("FreeDataBlock():bh->us_count !=1");
		/* 该bh对应的block已经被释放,故bh也就不应该存在 */
		bh->uc_dirt = 0;
		bh->uc_valid = 0;
		fnRelse_bufblk(bh);
	}
	/* 修改数据区的bitmap */
	block = block - sb->s_firstdatazone + 1;  /* 数据区中的第一块blk编号是1而不是0 */
	if(0 == Resetbit(sb->s_zmap[block / 8192]->pb_data, block & 8191))
		panic("Resetbit() return 0");
	sb->s_zmap[block / 8192]->uc_dirt = 1;    /* 对应的bh需要写盘 */
}


/*=======================================================*
 *F: 申请设备dev上数据区中的一个逻辑块
 *I: dev-设备号
 *O: block-逻辑块号
 *C: 之前缓冲区中肯定不存在新分配的块
 *=======================================================*/
int NewDataBlock(unsigned short dev)
{
	struct stMSuperBlock *sb;  /* 内存中的超级块指针 */
	struct stBuf_Head *bh;
	int i, offset;

	/* 获得某个设备上fs的超级块 */
	if(!(sb = GetSuperblock(dev)))
		panic("FreeDataBlock(): super not exist!");
	/* 扫描数据区的逻辑块位图, 寻找第一个空闲的块(bit = 0) */
	offset = 8192;
	for(i = 0; i < 8; i++)
	{
		if(0 != (bh = sb->s_zmap[i]))
			if((offset = FindFirstZerobit(bh->pb_data)) < 8192)
				break;
	}
	if(i >= 8 || offset >= 8192)
		return 0;  /* 没有找到空闲块 */
	offset += i * 8192 + sb->s_firstdatazone - 1;  /* offset是数据块的逻辑号 */
	if(offset >= sb->s_nzones)
		return 0;
	/* 设置相应的bit */
	if(Setbit(bh->pb_data, offset))
		panic("NewDataBlock(): Setbit return 1");
	bh->uc_dirt = 1;
	/* 寻找一个可用的bh,清空数据块以显示它是新的(对应的设备块也就是新的喽);
	   不读老数据了,所以没有用 fnread_bufblk() */
	if(!(bh = fnGet_bufblk(dev, offset)))
		panic("NewDataBlock(): get bh falide");
	/* 这儿保证了新获得的块在清空时,不允许进程使用 */
	if(bh->us_count !=1)
		panic("NewDataBlock(): bh->us_count !=1");
	ClearBlock((unsigned int *)bh->pb_data);
	bh->uc_valid = 1;
	bh->uc_dirt = 1;
	fnRelse_bufblk(bh);
	return offset;
}


/*=======================================================*
 *F: 释放指定的dinode,复位对应文件系统中dinode位图bit
 *I: minode
 *O: void
 *C: 为什么参数是minode呢?因为minode代表最新的dinode信息,
 *   拥有更多的被使用信息. minode--->缓冲区中的dinode--->
 *   磁盘上的dinode.
 *=======================================================*/
void FreeDinode(struct stMinode *mi)
{
	struct stMSuperBlock *sb;  /* 内存中的超级块指针 */
	struct stBuf_Head *bh;

	if(!mi)
		return;
	/* 如果mi->i_dev == 0:说明该minode没有被使用 */
	if(!mi->i_dev)
		return;
	/* 检测该minode是否仍被其它使用 */
	if(mi->i_count > 1)
		panic("FreeDinode():mi->i_count > 1");
	if(mi->i_nlinks)
		panic("FreeDinode():mi->i_nlinks !=0");  /* 被其它目录项使用(映射) */
	/* 获取该inode所在的文件系统超级块 */
	if(!(sb = GetSuperblock(mi->i_dev)))
		panic("FreeDinode(): sb not exist");
	if(mi->i_num < 1 || mi->i_num > sb->s_ninodes)
		panic("FreeDinode():mi->i_num invalid");
	/* 对应该dinode的位图不存在(本来就没有, 或不在缓冲区中) */
	if(!(bh = sb->s_imap[mi->i_num >> 13]))
		panic("FreeDinode():sb->s_imap not exist");
	if(0 == Resetbit(bh->pb_data, mi->i_num & 8191))
		panic("FreeDinode():Resetbit return 0");
	bh->uc_dirt = 1;  /* 该位图需要写盘 */
	/* 清空该minode槽 */
	SetMem((unsigned char *)mi, sizeof(struct stMinode), 0);
}


/*=======================================================*
 *F: 从设备dev上申请一个新dinode.
 *I: dev- 设备号
 *O: minode(我们把dinode加载到内存中了)
 *C: 为什么一定要加载到内存中呢?因为是分配一个新dinoe,所以
 *   内存中是应该不存在对应的minode;磁盘上的dinode是无效数据,
 *   道理与NewDataBlock()一样.
 *=======================================================*/
struct stMinode * NewDinode(int dev)
{
	struct stMSuperBlock *sb;  /* 内存中的超级块指针 */
	struct stBuf_Head *bh;
	struct stMinode * mi;
	int i, offset;

	/* 从minode表中获得一个空闲项 */
	if(!(mi = GetEmptyMinode()))
		return 0;
	if(!(sb = GetSuperblock(dev)))
		panic("NewDinode():sb not exist");
	/* 扫描dinode位图,寻找空闲dinode */
	offset = 8192;
	for(i = 0; i < 8; i++)
		if(0 != (bh = sb->s_imap[i]))
			if((offset = FindFirstZerobit(bh->pb_data)) < 8192)
				break;
	/* 没有找到空闲dinode */
	if(offset >= 8192 || (offset + i * 8192) > sb->s_ninodes)
	{
		MiPut(mi);  /* 释放该minode */
		return 0;
	}
	/* 更改dinode位图中相应的bit */
	if(Setbit(bh->pb_data, offset))
		panic("NewDinode(): Setbit return 1");
	bh->uc_dirt = 1;
	/* 初始化该mi */
	mi->i_count = 1;
	mi->i_nlinks = 1;
	mi->i_dev = (unsigned short)dev;
	mi->i_gid = 0;   /* 没有使用 */
	mi->i_uid = 0;
	mi->i_dirt = 1;  /* 需要写盘更新dinode */
	mi->i_num = (unsigned short)(offset + i * 8192);
	mi->i_atime = mi->i_ctime = mi->i_time = 0;  /* 设置时间 */
	return mi;
}









