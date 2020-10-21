/*----------------------------------------------------------------------*
 *minode.cģ�鹦����:����minode[]��,�ϲ��ļ��������ǽ�����minode�Ļ���
 *�ϵ�,Ҳ����˵Ҫ����һ���ļ����뽫��Ӧ��dinode�ŵ�minode�ڴ����ȥ,
 *һ��û�п��еı���,���ᵼ��ʧ��. ����һ�㿴:�ļ��Ĵ������������Ƶ�.
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

/*--------------------------�����Ǹ�ģ���еĺ�������-----------------------*/
void WaitonMinode(struct stMinode *mi);
void LockMinode(struct stMinode *mi);
void UnlockMinode(struct stMinode *mi);
void MiPut(struct stMinode * mi);
struct stMinode * GetEmptyMinode();
struct stMinode * Miget(int dev, int nr);
void ReadfromDinode(struct stMinode *mi);
void WritetoDinode(struct stMinode *mi);
/*--------------------------�����Ǹ�ģ���еĺ�������-----------------------*/

extern struct stMSuperBlock glbMsbtable[SUPER_NR]; 
struct stMinode glbMinodetable[MINODE_NR] = {0, };

/*=======================================================*
 *F: �ȴ�ָ����minode����.
 *I: mi
 *O: void
 *=======================================================*/
void WaitonMinode(struct stMinode *mi)
{
	while(mi->i_lock)
		fnSleepOn(&mi->i_wait);
}

/*=======================================================*
 *F: ��ָ����minode����.
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
 *F: ��ָ����minode����.
 *I: mi
 *O: void
 *=======================================================*/
void UnlockMinode(struct stMinode *mi)
{
	mi->i_lock = 0;
	fnWakeUp(&mi->i_wait);
}


/*=======================================================*
 *F: ͬ�����е�minodes,��������bhд��
 *I: void
 *O: void
 *C: �����ͬ��ָ���ǽ�Minode��buffer��ͬ��, Ȼ��buffer��
 *   buffer.c�еĺ����������豸ͬ��.
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
 *F: �ͷ�һ��minode.������Ҫд��
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
	if(!mi->i_dev)  /* �豸��==0 */
	{
		mi->i_count--;
		return;
	}
	if(mi->i_count > 1)  /* ��������ʹ�� */
	{
		mi->i_count--;
		return;
	}
	/* ���mi->i_nlinks == 0˵����dinodeû�б��ļ�ʹ�� */
	if(!mi->i_nlinks)
	{
		/* �ͷŸ�dinodeӵ�еĴ������ݿ�,������ļ��ڵ�Ҳ���� */
		Truncate(mi);
		FreeDinode(mi);
		return;
	}
	if(mi->i_dirt)
		WritetoDinode(mi);
	mi->i_count--;  /* ����i_countӦ��Ϊ0 */
}


/*=======================================================*
 *F: ���ڴ��е�minode���л��һ�����еĽڵ��.
 *I: void
 *O: stMinode *
 *C: �����ڵȴ�����������,û���ҵ���panic
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
				lastmi = glbMinodetable;  /* ��ͷ��ʼ */
			if(!lastmi->i_count)
			{
				mi = lastmi;
				break;
			}
		}
		if(!mi)  /* û���ҵ� */
			panic("GetEmptyMinode(): mi is 0");
		WaitonMinode(mi);
		if(mi->i_dirt)
			WritetoDinode(mi);
	}while(mi->i_count);  /* �ջ�ϲ */
	SetMem((unsigned char *)mi, sizeof(struct stMinode), 0);
	mi->i_count = 1;
	return mi;
}


/*=======================================================*
 *F: ��minode���л��ָ����minode
 *I: dev-�豸��, nr-�ڵ��
 *O: stMinode *
 *=======================================================*/
struct stMinode * Miget(int dev, int nr)
{
	struct stMinode *mi, *empty;

	if(!dev)
		panic("Miget(): dev is 0");
	/* ��Ԥ��һ��minode�� */
	empty = GetEmptyMinode();
	/* ɨ������minode��,���Ѿ�����ڷ���Ҫ���minode */
	mi = glbMinodetable;
	while(mi < glbMinodetable + MINODE_NR)
	{
		if(mi->i_dev != dev || mi->i_num != nr)
		{
			mi++;
			continue;
		}
		WaitonMinode(mi);  /* �ҵ�Ŀ��,�ȴ����� */
		if(mi->i_dev != dev || mi->i_num != nr)
		{
			mi = glbMinodetable;  /* ������������ȥ��,���¿�ʼ */
			continue;
		}
		mi->i_count++;
		/* ����minode�Ƿ�Ϊ�����ļ�ϵͳ�İ�װ�� */
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
			/* �ҵ����ļ�ϵͳ��msuper_block */
			MiPut(mi);  /* ΪʲôҪ�ͷ�������ص�,��һ�� д����˭����superblock��Ӧ */
			dev = glbMsbtable[i].s_dev;
			nr = ROOT_INO;
			mi = glbMinodetable;
			/* ���Ҹ��ļ�ϵͳ�ĸ��ڵ�, ��Ϊ<���ص�>�ȼ���<���ڵ�> */
			continue;
		}
		if (empty)
			MiPut(empty);
		return mi;
	}
	/* û�����ڴ�����ҵ�����Ҫ���minode,��ô���豸�϶�ȡ */
	if(!empty)
		return 0;
	mi = empty;
	mi->i_dev = (unsigned short)dev;
	mi->i_num = (unsigned short)nr;
	ReadfromDinode(mi);
	return mi;
}


/*=======================================================*
 *F: ���豸�϶�ȡdinode --> minode;
 *I: stMinode *mi
 *O: void
 *=======================================================*/
void ReadfromDinode(struct stMinode *mi)
{
	struct stMSuperBlock *sb;  /* �ڴ��еĳ�����ָ�� */
	struct stBuf_Head *bh;
	int block;

	LockMinode(mi);
	/* ���ĳ���豸��fs�ĳ����� */
	if(!(sb = GetSuperblock(mi->i_dev)))
		panic("ReadfromDinode(): super not exist!");
	/* �����dinode�ڴ����ϵ��߼���� */
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(mi->i_num -1) / INODES_PER_BLOCK;
	if(!(bh = fnread_bufblk(mi->i_dev, block)))
		panic("ReadfromDinode(): bh is 0");
	/* ����dinode --> minode */
	*(struct stDinode *)mi = 
		((struct stDinode *)bh->pb_data)[(mi->i_num - 1) % INODES_PER_BLOCK];
	fnRelse_bufblk(bh);
	UnlockMinode(mi);
}


/*=======================================================*
 *F: ���豸��д��minode --> dinode(minode ---> bh ---> �豸)
 *I: stMinode *mi
 *O: void
 *=======================================================*/
void WritetoDinode(struct stMinode *mi)
{
	struct stMSuperBlock *sb;  /* �ڴ��еĳ�����ָ�� */
	struct stBuf_Head *bh;
	int block;

	LockMinode(mi);
	if(!mi->i_dirt || !mi->i_dev)
	{
		UnlockMinode(mi);
		return;
	}
	/* ���ĳ���豸��fs�ĳ����� */
	if(!(sb = GetSuperblock(mi->i_dev)))
		panic("ReadfromDinode(): super not exist!");
	/* �����dinode�ڴ����ϵ��߼���� */
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(mi->i_num -1) / INODES_PER_BLOCK;
	if(!(bh = fnread_bufblk(mi->i_dev, block)))
		panic("ReadfromDinode(): bh is 0");
	/* ����dinode --> minode */
	((struct stDinode *)bh->pb_data)[(mi->i_num - 1) % INODES_PER_BLOCK]
		= *(struct stDinode *)mi;
	bh->uc_dirt = 1;  /* ��Ҫд�̸��� */
	mi->i_dirt = 0;
	fnRelse_bufblk(bh);
	UnlockMinode(mi);
}


/*=======================================================*
 *F: ���ļ��е��߼����ݿ�--->�豸�ϵ��߼����ݿ�
 *I: stMinode *mi, fileblk, createflg
 *O: �豸�ϵ��߼����ݿ��
 *C: ���������־��λ,���ڶ�Ӧ���߼��鲻����ʱ�������´��̿�
 *=======================================================*/
int BlkmapFiletoDev(struct stMinode *mi, int fileblk, int createflg)
{
	struct stBuf_Head *bh;
	int i;

	if(fileblk < 0)
		panic("BlkmapFiletoDev(): fileblk < 0");
	if(fileblk >= 7 + 512 + 512 * 512)
		panic("BlkmapFiletoDev(): fileblk >= 7 + 512 + 512 * 512");  /* >=����ļ����� */
	if(fileblk < 7)  /* ���ļ���ʹ��ֱ�ӿ��ʾ */
	{
		if(createflg && !mi->i_zone[fileblk])
			if(mi->i_zone[fileblk] = (unsigned short)NewDataBlock(mi->i_dev))
			{
				mi->i_ctime = 0;
				mi->i_dirt = 1;
			}
		return mi->i_zone[fileblk];
	}
	/* ���fileblk >=7 && < 7 + 512 ��ʹ��һ�μ�ӿ� */
	fileblk -= 7;
	if(fileblk < 512)
	{
		if(createflg && !mi->i_zone[7])  /* ����һ�μ�ӿ� */
			if(mi->i_zone[7] = (unsigned short)NewDataBlock(mi->i_dev))
			{
				mi->i_ctime = 0;
				mi->i_dirt = 1;
			}
		if(!mi->i_zone[7])  /* ����һ�μ�ӿ�ʧ�� */
			return 0;
		/* ��ȡһ�μ�ӿ� */
		if(!(bh = fnread_bufblk(mi->i_dev, mi->i_zone[7])))
			return 0;
		i = ((unsigned short *)(bh->pb_data))[fileblk];
		/* ����Ƿ���Ҫ�����´��̿� */
		if(createflg && !i)
			if(i = NewDataBlock(mi->i_dev))
			{
				((unsigned short *)(bh->pb_data))[fileblk] = (unsigned short)i;
				bh->uc_dirt = 1;
			}
		/* �ͷ�bh,�����̿�� */
		fnRelse_bufblk(bh);
		return i;
	}
	/* �����Ƕ��μ�ӿ�Ĵ��� */
	fileblk -= 512;
	if(createflg && !mi->i_zone[8])
		if(mi->i_zone[8] = (unsigned short)NewDataBlock(mi->i_dev))
		{
			mi->i_ctime = 0;
			mi->i_dirt = 1;
		}
	if(!mi->i_zone[8])  /* �������ʧ��,����Ҫ���� */
		return 0;
	if(!(bh = fnread_bufblk(mi->i_dev, mi->i_zone[8])))
		return 0;
	/* ȡ���μ�ӿ��ϵĵ�һ�μ�ӿ�[fileblk/512]��ֵ */
	i = ((unsigned short *)(bh->pb_data))[fileblk >> 9];
	if(createflg && !i)
		if(i = NewDataBlock(mi->i_dev))
		{
			((unsigned short *)(bh->pb_data))[fileblk >> 9] = (unsigned short)i;
			bh->uc_dirt = 1;
		}
	/* �ͷ�bh */
	fnRelse_bufblk(bh);
	if(!i)  /* ������μ�ӿ�ʧ��,���贴�� */
		return 0;
	if(!(bh = fnread_bufblk(mi->i_dev, i)))
		return 0;
	/* ȡ�ڶ�����ӿ��еĶ�Ӧ��ֵ */
	i = ((unsigned short *)(bh->pb_data))[fileblk & 511];
	if(createflg && !i)
		if(i = NewDataBlock(mi->i_dev))
		{
			((unsigned short *)(bh->pb_data))[fileblk & 511] = (unsigned short)i;
			bh->uc_dirt = 1;
		}
	/* �ͷ�bh */
	fnRelse_bufblk(bh);
	return i;
}


/*=======================================================*
 *F: �Ǵ����ػ���ļ����ݿ�Ĵ��̿��
 *I: mi-�ļ��ڴ�ڵ�ָ��, fileblk-�ļ����
 *O: �豸�ϵ��߼����ݿ��
 *C: ��Ӧ���߼��鲻����ʱ�ͷ���0
 *=======================================================*/
int Bmap(struct stMinode *mi, int fileblk)
{
	return BlkmapFiletoDev(mi, fileblk, 0);
}


/*=======================================================*
 *F: �����ػ���ļ����ݿ�Ĵ��̿��
 *I: mi-�ļ��ڴ�ڵ�ָ��, fileblk-�ļ����
 *O: �豸�ϵ��߼����ݿ��
 *C: ��Ӧ���߼��鲻����ʱ�������ʧ�ܾͷ���0
 *=======================================================*/
int CreateBmap(struct stMinode *mi, int fileblk)
{
	return BlkmapFiletoDev(mi, fileblk, 0);
}























