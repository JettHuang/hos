/*---------------------------------------------------------------------*
 *˵��:  ��������truncate����, ��ģ���еĺ��������ͷ�ָ��Minode(Dinode)
 *     ���豸��ռ�õ������߼���, ����һ��ֱ�ӿ�, һ�μ�ӿ�Ͷ��μ�ӿ顣
 *     ���������ļ�ռ�õ��豸�ռ��ͷŵ�, �ļ�����Ϊ0��ע���Minode(Dinode)
 *     ����������Ĵ��ڵġ�����ĺ���������linux0.11���ʵ��޸�һ��.
 *                                   2007-06-18  18:03
 *---------------------------------------------------------------------*/
#include "fs.h"
#include "buffer.h"

extern struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block);
extern void FreeDataBlock(unsigned short dev, int block);
extern void fnRelse_bufblk(struct stBuf_Head *bh);


/*=======================================================*
 *F: �ͷ�һ�μ�ӿ�
 *I: dev-�豸��, block-�߼����
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
 *F: �ͷŶ��μ�ӿ�
 *I: dev-�豸��, block-�߼����
 *O: void
 *=======================================================*/
static void free_dind(unsigned int dev, unsigned int block)
{
	struct stBuf_Head *bh;
	unsigned short *p;
	int i;

	if(!block)
		return;
	if((bh = fnread_bufblk((unsigned short)dev, block))) //��ȡ���μ�ӿ��еĵ�һ�μ�ӿ�
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
 *F: ��ָ�����ļ��ڵ��Ӧ���ļ����ݽ�Ϊ0
 *I: mi-�ڴ�ڵ�ָ��
 *O: void
 *=======================================================*/
void Truncate(struct stMinode *mi)
{
	int i;

	//���ǳ����ļ���Ŀ¼�ļ��򷵻�
	if(!(S_ISREG(mi->i_mode) || S_ISDIR(mi->i_mode)))
		return;
	//�ͷ�ֱ�ӿ�
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














