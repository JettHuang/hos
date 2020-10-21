/*----------------------------------------------------------------*
 *该模块管理superblock[]表,和文件系统的加载. 文件系统的挂载是动态的,
 *也就是说在磁盘上是体现不出来的.一个磁盘(确切的说文件系统)被使用前
 *super block必须被加载到内存中.
 *                              huangheshui  06/09/07 10:20
 *                                           2007-06-18  18:35
 *----------------------------------------------------------------*/
#include "fs.h"
#include "buffer.h"
#include "..\..\include\color.h"
#include "..\..\include\process.h"
#include "..\..\include\tty.h"

extern void fnRelse_bufblk(struct stBuf_Head *bh);
extern struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block);
extern struct stMinode glbMinodetable[MINODE_NR];
extern void SyncDevce(int dev);
extern void fnSleepOn(struct PCB * *pwait);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);
extern void panic(char *szerror);
extern struct stMinode * Miget(int dev, int nr);
extern void MiPut(struct stMinode *mi);
extern struct stMinode * PathnametoMi(const char *pathname);
extern struct PCB *pCurrent; 
extern int ReadTTY(struct TTY *t, char *kybuf, int kynum, int mode, char kywake);
extern void WriteTTY(struct TTY *t, char *pszstr, int len);
extern void PaintScreen(int color);
extern void vsprintf(char *buf, char *fmt, ...);
extern int Testbit(unsigned char *startaddr, unsigned int offset);
//全局变量
//
struct stFile glbFiletable[FILE_NR] = {0, };
struct stMSuperBlock glbMsbtable[SUPER_NR] = {0, };
int ROOT_DEV = 0x0200;


/*=======================================================*
 *F: 等待指定的super block解锁.
 *I: sb-超级块内存指针
 *O: void
 *=======================================================*/
void WaitonSuperblock(struct stMSuperBlock *sb)
{
	while(sb->s_lock)
		fnSleepOn(&sb->s_wait);
}


/*=======================================================*
 *F: 对指定的super block加锁.
 *I: sb-超级块内存指针
 *O: void
 *=======================================================*/
void LockSuperblock(struct stMSuperBlock *sb)
{
	while(sb->s_lock)
		fnSleepOn(&sb->s_wait);
	sb->s_lock = 1;
}


/*=======================================================*
 *F: 对指定的super block解锁.
 *I: sb-超级块内存指针
 *O: void
 *=======================================================*/
void UnlockSuperblock(struct stMSuperBlock *sb)
{
	sb->s_lock = 0;
	fnWakeUp(&sb->s_wait);
}


/*=======================================================*
 *F: 取指定设备的超级块(查找功能).
 *I: dev-设备号
 *O: stMSuperBlock *
 *=======================================================*/
struct stMSuperBlock * GetSuperblock(int dev)
{
	struct stMSuperBlock *sb;

	if(!dev)
		return 0;
	/* 开始搜索超级块表 */
	sb = glbMsbtable;
	while(sb < glbMsbtable + SUPER_NR)
	{
		if(sb->s_dev == dev)
		{
			WaitonSuperblock(sb);
			if(sb->s_dev == dev)  /* 检测是否被其它进程抢去了 */
				return sb;
			sb = glbMsbtable;     /* 重新开始搜索 */
		}
		else
			sb++;
	}
	return 0;
}


/*=======================================================*
 *F: 释放指定设备的超级块.
 *I: dev-设备号
 *O: void
 *C: sb不存在引用计数该项,必将清空sb槽
 *=======================================================*/
void PutSuperblock(int dev)
{
	struct stMSuperBlock *sb;
	int i;

	if(dev == ROOT_DEV)
		panic("PutSuperblock(): dev == ROOT_DEV");
	if(!(sb = GetSuperblock(dev)))	/* 超级块表中不存在该sb */
		return;
	/* 该文件系统上的某个minode挂载其它super */
	if(sb->s_imount) 
	{
		printk(YELLOW, "PutSuperblock():sb->s_imount");
		return;
	}
	LockSuperblock(sb);
	for(i = 0; i < I_MAP_SLOTS; i++)
		fnRelse_bufblk(sb->s_imap[i]);
	for(i = 0; i < Z_MAP_SLOTS; i++)
		fnRelse_bufblk(sb->s_zmap[i]);
	sb->s_dev = 0; /* 释放该sb,这是与minode是不一样的 */
	UnlockSuperblock(sb);  /* 解锁sb */
}


/*=======================================================*
 *F: 从设备读取超级块到内存中.
 *I: dev-设备号
 *O: void
 *C: 如果该sb存在就直接返回sb,否则找个空闲槽,再从设备上读入
 *=======================================================*/
struct stMSuperBlock * ReadSuperblock(int dev)
{
	struct stMSuperBlock *sb;
	struct stBuf_Head *bh;
	int i, block;

	if(!dev)
		return 0;
	if(sb = GetSuperblock(dev))
		return sb;
	/* 找一个空闲槽 */
	for(sb = glbMsbtable; ;sb++)
	{
		if(sb >= glbMsbtable + SUPER_NR)
			return 0;
		if(!sb->s_dev)
			break;
	}
	/* 初始化该super block槽 */
	sb->s_dev = (unsigned short)dev;
	sb->s_isup = 0;   /* 该文件系统的根目录minode */
	sb->s_imount = 0;
	sb->s_time = 0;
	sb->s_rd_only = 0;
	sb->s_dirt = 0;
	/* 锁定该超级块, 并从设备上读取超级块到sb中 */
	LockSuperblock(sb);
	if(!(bh = fnread_bufblk((unsigned short)dev, 1)))  /* 读设备上的sb失败*/
	{
		sb->s_dev = 0;
		UnlockSuperblock(sb);
		return 0;
	}
	/*复制dev superblk --> mem superblk */
	*((struct stDSuperBlock *)sb) = 
		*((struct stDSuperBlock *)(bh->pb_data));
	fnRelse_bufblk(bh);
	/* 检验文件系统的正确性 */
	if(sb->s_magic != SUPER_MAGIC)
	{
		sb->s_dev = 0;
		UnlockSuperblock(sb);
		printk(RED, "error: SUPER_MAGIC is not Minix1.0\n");
		return 0;
	}
	/* 开始读入super对应的dinode位图和datazone位图块 */
	for(i = 0; i < I_MAP_SLOTS; i++)
		sb->s_imap[i] = 0;
	for(i = 0; i < Z_MAP_SLOTS; i++)
		sb->s_zmap[i] = 0;
	block = 2;
	for(i = 0; i < sb->s_imap_blocks; i++)
	{
		if(sb->s_imap[i] = fnread_bufblk((unsigned short)dev, block))
			block++;
		else
			break;
	}
	for(i = 0; i < sb->s_zmap_blocks; i++)
	{
		if(sb->s_zmap[i] = fnread_bufblk((unsigned short)dev, block))
			block++;
		else
			break;
	}
	/* 验证是否全部读入 */
	if(block != 2 + sb->s_imap_blocks + sb->s_zmap_blocks)
	{
		for(i = 0; i < I_MAP_SLOTS; i++)
			fnRelse_bufblk(sb->s_imap[i]);
		for(i = 0; i < Z_MAP_SLOTS; i++)
			fnRelse_bufblk(sb->s_zmap[i]);
		sb->s_dev = 0;
		UnlockSuperblock(sb);  /* 解锁sb */
		return 0;
	}
	/* 至此读取super成功 */
	sb->s_imap[0]->pb_data[0] |= 0x01;  /* bitmap中的bit0是不允许使用,默认是1(占用) */
	sb->s_zmap[0]->pb_data[0] |= 0x01;
	UnlockSuperblock(sb);  /* 解锁sb */
	return sb;
}


/*=======================================================*
 *F: 卸载文件系统.
 *I: devname-设备名称
 *O: void
 *=======================================================*/
void Unmount(char *devname)
{
	struct stMSuperBlock *sb;
	struct stMinode *mi;
	int dev;

	/* 设备名映射到对应的minode */
	if(!(mi = PathnametoMi(devname)))
	{
		printk(YELLOW, "Unmount(): mi = 0");
		return;
	}
	dev = mi->i_zone[0];  /* 获取设备号 */
	MiPut(mi);
	/* 获得该设备的超级块 */
	if(!(sb = GetSuperblock(dev)) || !(sb->s_imount))
	{
		printk(YELLOW, "this dev is not mounted");
		return;
	}
	if(!sb->s_imount->i_mount)  /* 挂载点没有安装标志 */
		printk(YELLOW, "warning: munted minode's i_mount = 0");
	/* 检测是否有进程在使用该设备上的文件 */
	for(mi = glbMinodetable; mi < glbMinodetable + MINODE_NR; mi++)
		if(mi->i_dev == dev && mi->i_count)
		{
			printk(YELLOW, "this dev is still be used");
			return;
		}
	sb->s_imount->i_mount = 0;
	MiPut(sb->s_imount);
	sb->s_imount = 0;
	MiPut(sb->s_isup);  /* 释放该文件系统的根节点minode */
	sb->s_isup = 0;
	PutSuperblock(dev); /* 释放该超级块(卸掉) */
	//同步数据(包含的Minodes)
	SyncDevce(dev);
}


/*=======================================================*
 *F: 安装文件系统.
 *I: devname-设备名称, dirname-挂载目录
 *O: void
 *=======================================================*/
void Mount(char *devname, char *dirname)
{
	struct stMSuperBlock *sb;
	struct stMinode *mi, *dirmi;
	int dev;

	/* 设备名映射到对应的minode */
	if(!(mi = PathnametoMi(devname)))
	{
		printk(YELLOW, "mount(): mi = 0");
		return;
	}
	dev = mi->i_zone[0];  /* 获取设备号 */
	MiPut(mi);
	/* 获取挂载点dirmi */
	if(!(dirmi = PathnametoMi(dirname)))
	{
		printk(YELLOW, "mount(): dirmi = 0");
		return;
	}
	/* 挂载点必须专用,并且不能是根节点 */
	if(dirmi->i_count != 1 && dirmi->i_num == ROOT_INO)
	{
		printk(YELLOW, "mount(): root minode can't be mount");
		MiPut(dirmi);
		return;
	}
	if(!S_ISDIR(dirmi->i_mode))  /* 挂载点必须是目录 */
	{
		printk(YELLOW, "mount(): mounted mi must be directory");
		MiPut(dirmi);
		return;
	}
	/* 将设备的超级块读入到内存 */
	if(!(sb = ReadSuperblock(dev)))
	{
		printk(YELLOW, "mount(): read super failed");
		MiPut(dirmi);
		return;
	}
	/* 一个文件系统只能被挂载在一个节点上 */
	if(sb->s_imount)
	{
		printk(YELLOW, "mount(): devname has been mounted");
		MiPut(dirmi);
		return;
	}
	/* 一个挂载点只能挂载一个文件系统 */
	if(dirmi->i_mount)
	{
		printk(YELLOW, "mount(): dirname has  mounted other fs");
		MiPut(dirmi);
		return;
	}
	/* 现在可以说应该没有问题了,挂载点必须在内存中!! */
	sb->s_imount = dirmi;
	dirmi->i_mount = 1;
	dirmi->i_dirt = 1;  /* 为什么dirt? */
}


/*=======================================================*
 *F: 安装根文件系统(only one).
 *I: void
 *O: void
 *=======================================================*/
void MountRootfs()
{
	struct stMSuperBlock *sb;
	struct stMinode *mi;
	int i;
	static int once = 0;
	char input;
	int free;
	char szBuf[64];

	if(once)
		return;
	once = 1;
	/* 初始化系统文件表 */
	for(i = 0; i < FILE_NR; i++)
		glbFiletable[i].f_count = 0;
	/* 根文件系统是软盘 */
	if(MAJOR(ROOT_DEV) == 2)
	{
		printk(GREEN, "\nInsert Rootfs floppy and press ENTER\n");
		//等待按键
		ReadTTY(pCurrent->ptty, &input, 1, KB_SYNC, 0);
	}
	/* 初始化超级块数组 */
	for(sb = glbMsbtable; sb < glbMsbtable + SUPER_NR; sb++)
	{
		sb->s_dev = 0;
		sb->s_lock = 0;
		sb->s_wait = 0;
	}
	/* 开始读根设备上的super */
	if(!(sb = ReadSuperblock(ROOT_DEV)))
		panic("unable to mount rootfs\n");
	if(!(mi = Miget(ROOT_DEV, ROOT_INO)))  /* 根节点(根目录节点) */
		panic("Get root inode failed\n");
	/* 评论: 这里的挂载比较特别, mi既作为挂载根文件系统的minode,又作为文件系统根节点.
	 *       因为该挂载点不是设备上的,而是内存中的.(节约了一个节点)
	 */
	mi->i_count += 3;
	sb->s_imount = sb->s_isup = mi;
	//设置工作目录等等
	pCurrent->pwd = mi;
	pCurrent->root = mi;
	//加载根文件系统成功,并做统计工作
	PaintScreen(pCurrent->ptty->vd.color >> 4);
	WriteTTY(pCurrent->ptty, "Mount RootFs Succeed!\n", -1);
	free = 0;
	i = sb->s_nzones;
	while(--i >= 0)
		if(!Testbit(sb->s_zmap[i >> 13]->pb_data, i & 8191))
			free++;
	vsprintf(szBuf, "0x%x / 0x%x free blocks\n", free, sb->s_nzones);
	WriteTTY(pCurrent->ptty, szBuf, -1);
	free = 0;
	i = sb->s_ninodes + 1;
	while(--i >= 0)
		if(!Testbit(sb->s_imap[i >> 13]->pb_data, i & 8191))
			free++;
	vsprintf(szBuf, "0x%x / 0x%x free inodes\n", free, sb->s_ninodes);
	WriteTTY(pCurrent->ptty, szBuf, -1);
	//成功了!
}