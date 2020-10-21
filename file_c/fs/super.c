/*----------------------------------------------------------------*
 *��ģ�����superblock[]��,���ļ�ϵͳ�ļ���. �ļ�ϵͳ�Ĺ����Ƕ�̬��,
 *Ҳ����˵�ڴ����������ֲ�������.һ������(ȷ�е�˵�ļ�ϵͳ)��ʹ��ǰ
 *super block���뱻���ص��ڴ���.
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
//ȫ�ֱ���
//
struct stFile glbFiletable[FILE_NR] = {0, };
struct stMSuperBlock glbMsbtable[SUPER_NR] = {0, };
int ROOT_DEV = 0x0200;


/*=======================================================*
 *F: �ȴ�ָ����super block����.
 *I: sb-�������ڴ�ָ��
 *O: void
 *=======================================================*/
void WaitonSuperblock(struct stMSuperBlock *sb)
{
	while(sb->s_lock)
		fnSleepOn(&sb->s_wait);
}


/*=======================================================*
 *F: ��ָ����super block����.
 *I: sb-�������ڴ�ָ��
 *O: void
 *=======================================================*/
void LockSuperblock(struct stMSuperBlock *sb)
{
	while(sb->s_lock)
		fnSleepOn(&sb->s_wait);
	sb->s_lock = 1;
}


/*=======================================================*
 *F: ��ָ����super block����.
 *I: sb-�������ڴ�ָ��
 *O: void
 *=======================================================*/
void UnlockSuperblock(struct stMSuperBlock *sb)
{
	sb->s_lock = 0;
	fnWakeUp(&sb->s_wait);
}


/*=======================================================*
 *F: ȡָ���豸�ĳ�����(���ҹ���).
 *I: dev-�豸��
 *O: stMSuperBlock *
 *=======================================================*/
struct stMSuperBlock * GetSuperblock(int dev)
{
	struct stMSuperBlock *sb;

	if(!dev)
		return 0;
	/* ��ʼ����������� */
	sb = glbMsbtable;
	while(sb < glbMsbtable + SUPER_NR)
	{
		if(sb->s_dev == dev)
		{
			WaitonSuperblock(sb);
			if(sb->s_dev == dev)  /* ����Ƿ�����������ȥ�� */
				return sb;
			sb = glbMsbtable;     /* ���¿�ʼ���� */
		}
		else
			sb++;
	}
	return 0;
}


/*=======================================================*
 *F: �ͷ�ָ���豸�ĳ�����.
 *I: dev-�豸��
 *O: void
 *C: sb���������ü�������,�ؽ����sb��
 *=======================================================*/
void PutSuperblock(int dev)
{
	struct stMSuperBlock *sb;
	int i;

	if(dev == ROOT_DEV)
		panic("PutSuperblock(): dev == ROOT_DEV");
	if(!(sb = GetSuperblock(dev)))	/* ��������в����ڸ�sb */
		return;
	/* ���ļ�ϵͳ�ϵ�ĳ��minode��������super */
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
	sb->s_dev = 0; /* �ͷŸ�sb,������minode�ǲ�һ���� */
	UnlockSuperblock(sb);  /* ����sb */
}


/*=======================================================*
 *F: ���豸��ȡ�����鵽�ڴ���.
 *I: dev-�豸��
 *O: void
 *C: �����sb���ھ�ֱ�ӷ���sb,�����Ҹ����в�,�ٴ��豸�϶���
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
	/* ��һ�����в� */
	for(sb = glbMsbtable; ;sb++)
	{
		if(sb >= glbMsbtable + SUPER_NR)
			return 0;
		if(!sb->s_dev)
			break;
	}
	/* ��ʼ����super block�� */
	sb->s_dev = (unsigned short)dev;
	sb->s_isup = 0;   /* ���ļ�ϵͳ�ĸ�Ŀ¼minode */
	sb->s_imount = 0;
	sb->s_time = 0;
	sb->s_rd_only = 0;
	sb->s_dirt = 0;
	/* �����ó�����, �����豸�϶�ȡ�����鵽sb�� */
	LockSuperblock(sb);
	if(!(bh = fnread_bufblk((unsigned short)dev, 1)))  /* ���豸�ϵ�sbʧ��*/
	{
		sb->s_dev = 0;
		UnlockSuperblock(sb);
		return 0;
	}
	/*����dev superblk --> mem superblk */
	*((struct stDSuperBlock *)sb) = 
		*((struct stDSuperBlock *)(bh->pb_data));
	fnRelse_bufblk(bh);
	/* �����ļ�ϵͳ����ȷ�� */
	if(sb->s_magic != SUPER_MAGIC)
	{
		sb->s_dev = 0;
		UnlockSuperblock(sb);
		printk(RED, "error: SUPER_MAGIC is not Minix1.0\n");
		return 0;
	}
	/* ��ʼ����super��Ӧ��dinodeλͼ��datazoneλͼ�� */
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
	/* ��֤�Ƿ�ȫ������ */
	if(block != 2 + sb->s_imap_blocks + sb->s_zmap_blocks)
	{
		for(i = 0; i < I_MAP_SLOTS; i++)
			fnRelse_bufblk(sb->s_imap[i]);
		for(i = 0; i < Z_MAP_SLOTS; i++)
			fnRelse_bufblk(sb->s_zmap[i]);
		sb->s_dev = 0;
		UnlockSuperblock(sb);  /* ����sb */
		return 0;
	}
	/* ���˶�ȡsuper�ɹ� */
	sb->s_imap[0]->pb_data[0] |= 0x01;  /* bitmap�е�bit0�ǲ�����ʹ��,Ĭ����1(ռ��) */
	sb->s_zmap[0]->pb_data[0] |= 0x01;
	UnlockSuperblock(sb);  /* ����sb */
	return sb;
}


/*=======================================================*
 *F: ж���ļ�ϵͳ.
 *I: devname-�豸����
 *O: void
 *=======================================================*/
void Unmount(char *devname)
{
	struct stMSuperBlock *sb;
	struct stMinode *mi;
	int dev;

	/* �豸��ӳ�䵽��Ӧ��minode */
	if(!(mi = PathnametoMi(devname)))
	{
		printk(YELLOW, "Unmount(): mi = 0");
		return;
	}
	dev = mi->i_zone[0];  /* ��ȡ�豸�� */
	MiPut(mi);
	/* ��ø��豸�ĳ����� */
	if(!(sb = GetSuperblock(dev)) || !(sb->s_imount))
	{
		printk(YELLOW, "this dev is not mounted");
		return;
	}
	if(!sb->s_imount->i_mount)  /* ���ص�û�а�װ��־ */
		printk(YELLOW, "warning: munted minode's i_mount = 0");
	/* ����Ƿ��н�����ʹ�ø��豸�ϵ��ļ� */
	for(mi = glbMinodetable; mi < glbMinodetable + MINODE_NR; mi++)
		if(mi->i_dev == dev && mi->i_count)
		{
			printk(YELLOW, "this dev is still be used");
			return;
		}
	sb->s_imount->i_mount = 0;
	MiPut(sb->s_imount);
	sb->s_imount = 0;
	MiPut(sb->s_isup);  /* �ͷŸ��ļ�ϵͳ�ĸ��ڵ�minode */
	sb->s_isup = 0;
	PutSuperblock(dev); /* �ͷŸó�����(ж��) */
	//ͬ������(������Minodes)
	SyncDevce(dev);
}


/*=======================================================*
 *F: ��װ�ļ�ϵͳ.
 *I: devname-�豸����, dirname-����Ŀ¼
 *O: void
 *=======================================================*/
void Mount(char *devname, char *dirname)
{
	struct stMSuperBlock *sb;
	struct stMinode *mi, *dirmi;
	int dev;

	/* �豸��ӳ�䵽��Ӧ��minode */
	if(!(mi = PathnametoMi(devname)))
	{
		printk(YELLOW, "mount(): mi = 0");
		return;
	}
	dev = mi->i_zone[0];  /* ��ȡ�豸�� */
	MiPut(mi);
	/* ��ȡ���ص�dirmi */
	if(!(dirmi = PathnametoMi(dirname)))
	{
		printk(YELLOW, "mount(): dirmi = 0");
		return;
	}
	/* ���ص����ר��,���Ҳ����Ǹ��ڵ� */
	if(dirmi->i_count != 1 && dirmi->i_num == ROOT_INO)
	{
		printk(YELLOW, "mount(): root minode can't be mount");
		MiPut(dirmi);
		return;
	}
	if(!S_ISDIR(dirmi->i_mode))  /* ���ص������Ŀ¼ */
	{
		printk(YELLOW, "mount(): mounted mi must be directory");
		MiPut(dirmi);
		return;
	}
	/* ���豸�ĳ�������뵽�ڴ� */
	if(!(sb = ReadSuperblock(dev)))
	{
		printk(YELLOW, "mount(): read super failed");
		MiPut(dirmi);
		return;
	}
	/* һ���ļ�ϵͳֻ�ܱ�������һ���ڵ��� */
	if(sb->s_imount)
	{
		printk(YELLOW, "mount(): devname has been mounted");
		MiPut(dirmi);
		return;
	}
	/* һ�����ص�ֻ�ܹ���һ���ļ�ϵͳ */
	if(dirmi->i_mount)
	{
		printk(YELLOW, "mount(): dirname has  mounted other fs");
		MiPut(dirmi);
		return;
	}
	/* ���ڿ���˵Ӧ��û��������,���ص�������ڴ���!! */
	sb->s_imount = dirmi;
	dirmi->i_mount = 1;
	dirmi->i_dirt = 1;  /* Ϊʲôdirt? */
}


/*=======================================================*
 *F: ��װ���ļ�ϵͳ(only one).
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
	/* ��ʼ��ϵͳ�ļ��� */
	for(i = 0; i < FILE_NR; i++)
		glbFiletable[i].f_count = 0;
	/* ���ļ�ϵͳ������ */
	if(MAJOR(ROOT_DEV) == 2)
	{
		printk(GREEN, "\nInsert Rootfs floppy and press ENTER\n");
		//�ȴ�����
		ReadTTY(pCurrent->ptty, &input, 1, KB_SYNC, 0);
	}
	/* ��ʼ������������ */
	for(sb = glbMsbtable; sb < glbMsbtable + SUPER_NR; sb++)
	{
		sb->s_dev = 0;
		sb->s_lock = 0;
		sb->s_wait = 0;
	}
	/* ��ʼ�����豸�ϵ�super */
	if(!(sb = ReadSuperblock(ROOT_DEV)))
		panic("unable to mount rootfs\n");
	if(!(mi = Miget(ROOT_DEV, ROOT_INO)))  /* ���ڵ�(��Ŀ¼�ڵ�) */
		panic("Get root inode failed\n");
	/* ����: ����Ĺ��رȽ��ر�, mi����Ϊ���ظ��ļ�ϵͳ��minode,����Ϊ�ļ�ϵͳ���ڵ�.
	 *       ��Ϊ�ù��ص㲻���豸�ϵ�,�����ڴ��е�.(��Լ��һ���ڵ�)
	 */
	mi->i_count += 3;
	sb->s_imount = sb->s_isup = mi;
	//���ù���Ŀ¼�ȵ�
	pCurrent->pwd = mi;
	pCurrent->root = mi;
	//���ظ��ļ�ϵͳ�ɹ�,����ͳ�ƹ���
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
	//�ɹ���!
}