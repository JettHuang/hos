/*-----------------------------------------------------------*
 *bitmap.c�Ĺ����ǲ������豸�ϵ�<λͼ>,�������豸�Ĵ洢�ռ�.
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
 *F: ���ô�ָ����ַ��ʼ, ƫ��ֵoffset����bit(bit <- 1);
 *I: startaddr, offset
 *O: ��bit��ԭֵ(0|1)
 *C: ע��߽�ֵ��û�м����
 *=======================================================*/
int Setbit(unsigned char *startaddr, unsigned int offset)
{
	unsigned int ibyte;   /* ��λ���ڵ��ֽ�����ֵ */
	unsigned int tmp;
	
	if(!startaddr)
		return 1;  //ע��ͨ�����ַ�ʽ����
	ibyte = offset / (sizeof(char) * 8);
    offset &= (sizeof(char) * 8) -1;
	tmp = startaddr[ibyte] & (0x01 << offset);
	startaddr[ibyte] |= (0x01 << offset);
	if(tmp)
		return 1;
	return 0;
}


/*=======================================================*
 *F: ��λ��ָ����ַ��ʼ, ƫ��ֵoffset����bit(bit <- 0);
 *I: startaddr, offset
 *O: ��bit��ԭֵ(0|1)
 *=======================================================*/
int Resetbit(unsigned char *startaddr, unsigned int offset)
{
	unsigned int ibyte;   /* ��λ���ڵ��ֽ�����ֵ */
	unsigned int tmp;
	
	if(!startaddr) 
		return 0;  //ע��ͨ�����ַ�ʽ����
	ibyte = offset / (sizeof(char) * 8);
    offset &= (sizeof(char) * 8) -1;
	tmp = startaddr[ibyte] & (0x01 << offset);
	startaddr[ibyte] &= ~(0x01 << offset);
	if(tmp)
		return 1;
	return 0;
}

/*=======================================================*
 *F: ���Դ�ָ����ַ��ʼ, ƫ��ֵoffset����bitֵ;
 *I: startaddr, offset
 *O: ��bit��ֵ(0|1)
 *C: ������Բ����ڵ�bit��ô������1(ռ�ñ�־)
 *=======================================================*/
int Testbit(unsigned char *startaddr, unsigned int offset)
{
	unsigned int ibyte;   /* ��λ���ڵ��ֽ�����ֵ */
	unsigned int tmp;
	
	if(!startaddr) 
		return 1;  //ע��ͨ�����ַ�ʽ����
	ibyte = offset / (sizeof(char) * 8);
    offset &= (sizeof(char) * 8) -1;
	tmp = startaddr[ibyte] & (0x01 << offset);
	if(tmp)
		return 1;
	return 0;
}


/*=======================================================*
 *F: ���Ҵ�ָ����ַ��ʼ,��һ��Ϊ0��bit
 *I: startaddr
 *O: ��bit��ƫ��ֵ.
 *=======================================================*/
int FindFirstZerobit(unsigned char *startaddr)
{
	unsigned int offset;

	/* startaddrָ����ַ�������1kb */
	for(offset = 0; offset < 8192; offset++)
		if(0 == (startaddr[offset / (sizeof(char) * 8)] & (0x01 << (offset % (sizeof(char) * 8)))))
			break;
	return offset;
}


/*=======================================================*
 *F: ��մ�ָ����һ�����ݿ�
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
 *F: ����һ���ڴ������
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
 *F: �ͷ��豸dev���������е��߼���block
 *I: dev, block
 *O: void
 *=======================================================*/
void FreeDataBlock(unsigned short dev, int block)
{
	struct stMSuperBlock *sb;  /* �ڴ��еĳ�����ָ�� */
	struct stBuf_Head *bh;

	/* ���ĳ���豸��fs�ĳ����� */
	if(!(sb = GetSuperblock(dev)))
		panic("FreeDataBlock(): super not exist!");
	/* ���block�Ƿ�Ϊ���������߼���� */
	if(block < sb->s_firstdatazone || block >= sb->s_nzones)
		panic("FreeDataBlock():block is not in datazone");
	/* ����������д��ڸ�block,���ͷ�bh */
	bh = fnGet_bufblk_inhash(dev, block);
	if(bh)
	{
		if(bh->us_count !=1)
			panic("FreeDataBlock():bh->us_count !=1");
		/* ��bh��Ӧ��block�Ѿ����ͷ�,��bhҲ�Ͳ�Ӧ�ô��� */
		bh->uc_dirt = 0;
		bh->uc_valid = 0;
		fnRelse_bufblk(bh);
	}
	/* �޸���������bitmap */
	block = block - sb->s_firstdatazone + 1;  /* �������еĵ�һ��blk�����1������0 */
	if(0 == Resetbit(sb->s_zmap[block / 8192]->pb_data, block & 8191))
		panic("Resetbit() return 0");
	sb->s_zmap[block / 8192]->uc_dirt = 1;    /* ��Ӧ��bh��Ҫд�� */
}


/*=======================================================*
 *F: �����豸dev���������е�һ���߼���
 *I: dev-�豸��
 *O: block-�߼����
 *C: ֮ǰ�������п϶��������·���Ŀ�
 *=======================================================*/
int NewDataBlock(unsigned short dev)
{
	struct stMSuperBlock *sb;  /* �ڴ��еĳ�����ָ�� */
	struct stBuf_Head *bh;
	int i, offset;

	/* ���ĳ���豸��fs�ĳ����� */
	if(!(sb = GetSuperblock(dev)))
		panic("FreeDataBlock(): super not exist!");
	/* ɨ�����������߼���λͼ, Ѱ�ҵ�һ�����еĿ�(bit = 0) */
	offset = 8192;
	for(i = 0; i < 8; i++)
	{
		if(0 != (bh = sb->s_zmap[i]))
			if((offset = FindFirstZerobit(bh->pb_data)) < 8192)
				break;
	}
	if(i >= 8 || offset >= 8192)
		return 0;  /* û���ҵ����п� */
	offset += i * 8192 + sb->s_firstdatazone - 1;  /* offset�����ݿ���߼��� */
	if(offset >= sb->s_nzones)
		return 0;
	/* ������Ӧ��bit */
	if(Setbit(bh->pb_data, offset))
		panic("NewDataBlock(): Setbit return 1");
	bh->uc_dirt = 1;
	/* Ѱ��һ�����õ�bh,������ݿ�����ʾ�����µ�(��Ӧ���豸��Ҳ�����µ��);
	   ������������,����û���� fnread_bufblk() */
	if(!(bh = fnGet_bufblk(dev, offset)))
		panic("NewDataBlock(): get bh falide");
	/* �����֤���»�õĿ������ʱ,���������ʹ�� */
	if(bh->us_count !=1)
		panic("NewDataBlock(): bh->us_count !=1");
	ClearBlock((unsigned int *)bh->pb_data);
	bh->uc_valid = 1;
	bh->uc_dirt = 1;
	fnRelse_bufblk(bh);
	return offset;
}


/*=======================================================*
 *F: �ͷ�ָ����dinode,��λ��Ӧ�ļ�ϵͳ��dinodeλͼbit
 *I: minode
 *O: void
 *C: Ϊʲô������minode��?��Ϊminode�������µ�dinode��Ϣ,
 *   ӵ�и���ı�ʹ����Ϣ. minode--->�������е�dinode--->
 *   �����ϵ�dinode.
 *=======================================================*/
void FreeDinode(struct stMinode *mi)
{
	struct stMSuperBlock *sb;  /* �ڴ��еĳ�����ָ�� */
	struct stBuf_Head *bh;

	if(!mi)
		return;
	/* ���mi->i_dev == 0:˵����minodeû�б�ʹ�� */
	if(!mi->i_dev)
		return;
	/* ����minode�Ƿ��Ա�����ʹ�� */
	if(mi->i_count > 1)
		panic("FreeDinode():mi->i_count > 1");
	if(mi->i_nlinks)
		panic("FreeDinode():mi->i_nlinks !=0");  /* ������Ŀ¼��ʹ��(ӳ��) */
	/* ��ȡ��inode���ڵ��ļ�ϵͳ������ */
	if(!(sb = GetSuperblock(mi->i_dev)))
		panic("FreeDinode(): sb not exist");
	if(mi->i_num < 1 || mi->i_num > sb->s_ninodes)
		panic("FreeDinode():mi->i_num invalid");
	/* ��Ӧ��dinode��λͼ������(������û��, ���ڻ�������) */
	if(!(bh = sb->s_imap[mi->i_num >> 13]))
		panic("FreeDinode():sb->s_imap not exist");
	if(0 == Resetbit(bh->pb_data, mi->i_num & 8191))
		panic("FreeDinode():Resetbit return 0");
	bh->uc_dirt = 1;  /* ��λͼ��Ҫд�� */
	/* ��ո�minode�� */
	SetMem((unsigned char *)mi, sizeof(struct stMinode), 0);
}


/*=======================================================*
 *F: ���豸dev������һ����dinode.
 *I: dev- �豸��
 *O: minode(���ǰ�dinode���ص��ڴ�����)
 *C: Ϊʲôһ��Ҫ���ص��ڴ�����?��Ϊ�Ƿ���һ����dinoe,����
 *   �ڴ�����Ӧ�ò����ڶ�Ӧ��minode;�����ϵ�dinode����Ч����,
 *   ������NewDataBlock()һ��.
 *=======================================================*/
struct stMinode * NewDinode(int dev)
{
	struct stMSuperBlock *sb;  /* �ڴ��еĳ�����ָ�� */
	struct stBuf_Head *bh;
	struct stMinode * mi;
	int i, offset;

	/* ��minode���л��һ�������� */
	if(!(mi = GetEmptyMinode()))
		return 0;
	if(!(sb = GetSuperblock(dev)))
		panic("NewDinode():sb not exist");
	/* ɨ��dinodeλͼ,Ѱ�ҿ���dinode */
	offset = 8192;
	for(i = 0; i < 8; i++)
		if(0 != (bh = sb->s_imap[i]))
			if((offset = FindFirstZerobit(bh->pb_data)) < 8192)
				break;
	/* û���ҵ�����dinode */
	if(offset >= 8192 || (offset + i * 8192) > sb->s_ninodes)
	{
		MiPut(mi);  /* �ͷŸ�minode */
		return 0;
	}
	/* ����dinodeλͼ����Ӧ��bit */
	if(Setbit(bh->pb_data, offset))
		panic("NewDinode(): Setbit return 1");
	bh->uc_dirt = 1;
	/* ��ʼ����mi */
	mi->i_count = 1;
	mi->i_nlinks = 1;
	mi->i_dev = (unsigned short)dev;
	mi->i_gid = 0;   /* û��ʹ�� */
	mi->i_uid = 0;
	mi->i_dirt = 1;  /* ��Ҫд�̸���dinode */
	mi->i_num = (unsigned short)(offset + i * 8192);
	mi->i_atime = mi->i_ctime = mi->i_time = 0;  /* ����ʱ�� */
	return mi;
}









