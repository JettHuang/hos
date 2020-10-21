/*---------------------------------------------------------------------*
 *说明:  正如名字truncate那样, 该模块中的函数用于释放指定Minode(Dinode)
 *     在设备上占用的所有逻辑块, 包括一次直接块, 一次间接块和二次间接块。
 *     这样将该文件占用的设备空间释放掉, 文件长度为0。注意该Minode(Dinode)
 *     还是有意义的存在的。这里的函数复制了linux0.11并适当修改一下.
 *                                   2007-06-18  18:03
 *---------------------------------------------------------------------*/
#include "fs.h"
#include "buffer.h"

extern struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block);
extern void FreeDataBlock(unsigned short dev, int block);
extern void fnRelse_bufblk(struct stBuf_Head *bh);


/*=======================================================*
 *F: 释放一次间接块
 *I: dev-设备号, block-逻辑块号
 *O: void
 *=======================================================*/
static void free_ind(unsigned int dev, unsigned int block)
{
	struct stBuf_Head *bh;
	unsigned short *p;
	int i;

	if(!block)
		return;
	if((bh = fnread_bufblk((unsigned short)dev, block)))
	{
		p = (unsigned short *)bh->pb_data;
		for(i = 0; i < 512; i++, p++)
			if(*p)
				FreeDataBlock((unsigned short)dev, *p);
		fnRelse_bufblk(bh);
	}
	FreeDataBlock((unsigned short)dev, block);
}


/*=======================================================*
 *F: 释放二次间接块
 *I: dev-设备号, block-逻辑块号
 *O: void
 *=======================================================*/
static void free_dind(unsigned int dev, unsigned int block)
{
	struct stBuf_Head *bh;
	unsigned short *p;
	int i;

	if(!block)
		return;
	if((bh = fnread_bufblk((unsigned short)dev, block))) //读取二次间接块中的第一次间接块
	{
		p = (unsigned short *)bh->pb_data;
		for(i = 0; i < 512; i++, p++)
			if(*p)
				free_ind(dev, *p);
		fnRelse_bufblk(bh);
	}
	FreeDataBlock((unsigned short)dev, block);
}


/*=======================================================*
 *F: 将指定的文件节点对应的文件内容截为0
 *I: mi-内存节点指针
 *O: void
 *=======================================================*/
void Truncate(struct stMinode *mi)
{
	int i;

	//不是常规文件或目录文件则返回
	if(!(S_ISREG(mi->i_mode) || S_ISDIR(mi->i_mode)))
		return;
	//释放直接块
	for(i = 0; i < 7; i++)
		if(mi->i_zone[i]) 
		{
			FreeDataBlock(mi->i_dev, mi->i_zone[i]);
			mi->i_zone[i]=0;
		}
	free_ind(mi->i_dev, mi->i_zone[7]);
	free_dind(mi->i_dev, mi->i_zone[8]);
	mi->i_zone[7] = mi->i_zone[8] = 0;
	mi->i_size = 0;
	mi->i_dirt = 1;
	mi->i_time = mi->i_ctime = 0;
}














