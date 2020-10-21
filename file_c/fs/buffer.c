/*--------------------------------------------------------------------------*
 * ˵�������ļ�ʵ�ֶ�file_buffer�Ĺ������ϲ��ṩ��ȡ���ݿ����
 *                                      huangheshui   06/04/07  20:24
 * ��ע��
 *	   �ӽ��̵Ĺ۵�����������Ҫ�������ݿ鶼�ڡ��ļ��������С�������
 * ���������ݴ��������������ǲ��ò��ĵġ�
 *	  ���ļ����������ӵ����̵����������Ѱ��Ŀǰ���������Ƿ����ƥ��
 * �ĺϸ�����ݿ顣��������ڣ���ô�����豸�������ݿ飬���豸��������
 * ���������������ˡ�
 *
 * �ڲ������б�
 * 1��fnInit_Buffer(void *start, void *end);
 * 2, fnFind_bufBlock(unsigned int dev, unsigned int block);
 * 3, fnGet_bufblk_inhash( unsigned int dev, unsigned int block);
 * 4, fnGet_bufblk(unsigned int dev, unsigned int block);
 *
 * �ӿں���(���ϲ��ṩ����)��
 * 1, fnRelse_bufblk(struct stBuf_Head *bh);
 * 2, fnread_bufblk(unsigned int dev, unsigned int block);
 *
 *�޸�: ����豸ͬ������                    2007-06-18  18:48
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

/*-----------------------�����Ǹ�ģ���еĺ�������---------------------------*/
void fnInit_Buffer();
void fnRemove_from_Queues(struct stBuf_Head *bh);
void fnInsert_to_Queues(struct stBuf_Head *bh);
struct stBuf_Head * fnFind_bufBlock(unsigned short dev, unsigned int block);
void WaitonBuffer(struct stBuf_Head *bh);
struct stBuf_Head * fnGet_bufblk_inhash(unsigned short dev, unsigned int block);
struct stBuf_Head * fnGet_bufblk(unsigned short dev, unsigned int block);
void fnRelse_bufblk(struct stBuf_Head *bh);
struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block);
/*-----------------------�����Ǹ�ģ���еĺ�������---------------------------*/

#define	COMPLIER_BASEADDR  0xC0000000
#define	NR_HASH  32 
#define	IndexHash(dev, blknr)	((unsigned int)((dev) ^ (blknr)) % NR_HASH)
#define	Hash(dev, blknr)	glbHash_table[IndexHash(dev, blknr)]
	
/* ���ڹ���buffer��ȫ�ֱ��� */
struct stBuf_Head *	glbHash_table[NR_HASH];
struct stBuf_Head *glbFree_list;   /* ���п�����ͷָ�� */
struct PCB *glbWait_Bufblk;
void *start, *end;
int NR_HEARDS = 0;
 
/*==========================================================*
 *F: ָ��һ���ڴ���Ϊfile buffer,�ú�����ʼ��buffer
 *I: void
 *O: void
 *==========================================================*/
void fnInit_Buffer()
{
	struct stBuf_Head *bh;
	unsigned int pblk, i;

	//��������kernel pagetable�� ~ 0x400000ǰ
	NR_HEARDS = 0;
	start = (void *)(uglbTotal_PmPages * sizeof(int) + COMPLIER_BASEADDR + 0x100000);
    end = (void *)(COMPLIER_BASEADDR + 0x400000);
	bh = (struct stBuf_Head *)start;
	pblk = (unsigned int)end;
	/* blk�Ŀ��� >= (bh + 1)�Ŀ��� */
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
		bh->bh_next_free = bh + 1;  /* �������һ��bh��Ҫ�޸� */
		bh->bh_prev_free = bh - 1;  /* ���ڵ�һ��bh��Ҫ�޸� */
		bh++;
		NR_HEARDS++;
	}
	bh--;	/* bh--�����һ��buf_head */
	glbFree_list = (struct stBuf_Head *)start;
	glbFree_list->bh_prev_free = bh;
	bh->bh_next_free = glbFree_list;  /* ������ѭ������ */
	/* ��ʼ��hash�� */
	for(i = 0; i < NR_HASH; i++)
		glbHash_table[i] = 0;
}


/*==========================================================*
 *F: ��bh��hash���free_listɾ��
 *I: struct stBuf_Head * 
 *O: void
 *==========================================================*/
void fnRemove_from_Queues(struct stBuf_Head *bh)
{
	if(!bh)
		return ;
	/* ���ȴ�hash��ɾ��,hash�ϵĵ�������ѭ���� */
	if(bh->bh_prev)
		bh->bh_prev->bh_next = bh->bh_next;
	if(bh->bh_next)
		bh->bh_next->bh_prev = bh->bh_prev;
	/* �ж�bh�Ƿ�Ϊ��һ��Ԫ�� */
	if(Hash(bh->usdev_id, bh->uldev_blknr) == bh)
		Hash(bh->usdev_id, bh->uldev_blknr) = bh->bh_next;
	if(!(bh->bh_prev_free) || !(bh->bh_next_free))  /* ���ǲ����ܵ� */
		panic("fnRemove_from_Queues(): free hd just only one");
	/* ��free_list��ɾ��,ע����б��ϴ�ʱӵ�����е�buf_head(��ʱ����,Ȼ���ְ���), ���Ǹ�ѭ������ */
	bh->bh_prev_free->bh_next_free = bh->bh_next_free;
	bh->bh_next_free->bh_prev_free = bh->bh_prev_free;
	if(glbFree_list == bh)
		glbFree_list = bh->bh_next_free;
}


/*==========================================================*
 *F: ��bh���뵽hash���ͷ����free_list��β��.
 *I: struct stBuf_Head * 
 *O: void
 *C: �˲��뷽��������RLU����
 *==========================================================*/
void fnInsert_to_Queues(struct stBuf_Head *bh)
{
	if(!bh)
		return;
	/* ����free_listβ��,��Ϊ��������滻��RLU */
	bh->bh_prev_free = glbFree_list->bh_prev_free;
	bh->bh_next_free = glbFree_list;
	glbFree_list->bh_prev_free->bh_next_free = bh;
	glbFree_list->bh_prev_free = bh;
	bh->bh_prev = 0;
	bh->bh_next = 0;
	/* ����hash��ͷ��, 0�豸���� */
	if(!bh->usdev_id)
		return;
	bh->bh_next = Hash(bh->usdev_id, bh->uldev_blknr);
	Hash(bh->usdev_id, bh->uldev_blknr) = bh;
	if(bh->bh_next != 0)
		bh->bh_next->bh_prev = bh;
}


/*==========================================================*
 *F: ��HASH�������ָ���豸��blk.
 *I: dev, block
 *O: �ҵ�����bh��ָ�룬����0.
 *C: �ú���ֻ������Ҫ���bh�Ƿ����.
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
 *�ȴ�ָ����bh����
 *==========================================================*/
void WaitonBuffer(struct stBuf_Head *bh)
{
	while(bh->uc_lock)
		fnSleepOn(&bh->task_wait);
}


/*==========================================================*
 *F: ��HASH������һ������ʹ�õ�(dev,blk)�����.
 *I: dev, block
 *O: �����÷���bh��ָ��,����0.
 *C: ���øú����Ľ��̿��ܻ���ȴ�����������.
 *==========================================================*/
struct stBuf_Head * fnGet_bufblk_inhash(unsigned short dev, unsigned int block)
{
	struct stBuf_Head *bh;
		
	/* �ȼ���Ƿ���� */
	if(!(bh = fnFind_bufBlock(dev, block)))
		return  0;
	bh->us_count++;
	/* �ȴ�������,�����������sleep״̬ */
	WaitonBuffer(bh);
	/* ����Ϊû�б�Ҫ�ټ�� dev��block ����ȷ����,��Ϊbh->us_count++; */
	return bh;
}


/*==========================================================*
 *F: ������buffer�л��һ������ʹ�õ�(dev,blk)�����.
 *I: dev, block
 *O: ��÷���bh��ָ��
 *C:    �ú������ܻ���ȴ�����������,������hash��Ѱ���Ѿ����ڵ�buffer,
 *  ����Ҳ����Ļ�����ȥfree_list�л��һ�����п飬���п��ܻ���һ��blk
 *  д��(��д),���free_listû�п��п�, ��ô����ȴ�buffer��sleep. ��õ�
 *  bufblk�Ƿ����ֱ��ʹ��,��Ҫ���bh->isvalid.
 *===========================================================*/
struct stBuf_Head * fnGet_bufblk(unsigned short dev, unsigned int block)
{
	struct stBuf_Head *bh, *tmp;

_repeat:
	/* ��������hash */
	if(bh = fnGet_bufblk_inhash(dev, block))
		return bh;
	/* ��free_list�ϻ��һ��bufblk */
	tmp = glbFree_list;
	do{
		if(tmp->us_count != 0)
			continue;	/* ���������һ��bh */
		bh = tmp;
		break;
	}while((tmp = tmp->bh_next_free) != glbFree_list);
    /* û�п��е�buf_blk */
	if(!bh)
	{
		fnSleepOn(&glbWait_Bufblk);  /* �ȴ�����bh */
		goto _repeat;
	}
	/* �ȴ����������� */
	WaitonBuffer(bh);
	if(bh->us_count)  /* ���ܱ�������������ռ��(��������ǰ��) */
		goto _repeat;
	if(bh->uc_dirt)
	{
		fnDev_RW_blk(WRITE, bh);  /* д�� */
		WaitonBuffer(bh);  
		/* ���ܱ�����������ռ��, �������̿���ִ�е������WaitonBuffer(bh); */
		if(bh->us_count)
			goto _repeat;
	}
	/* ���ڸղ�˯�ߵ�ʱ��, ���п����������̻���˸ý������ҵ�bh(H���ǿ��ܷ�����) */
	if(fnFind_bufBlock(dev, block))
	{
		fnRelse_bufblk(bh);  /* �ͷ��µõ�bh */
		goto _repeat;
	}
	/* �������ǻ����һ�����õĿ�,init it! */
	bh->us_count = 1;
	bh->uc_dirt = 0;
	bh->uc_valid = 0;
	/* ��ʱɾ�� */
	fnRemove_from_Queues(bh);
	/* �����豸�ź�blknr */
	bh->usdev_id = dev;
	bh->uldev_blknr = block;
	/* ���²��� */
	fnInsert_to_Queues(bh);
	return bh;
}


/*==========================================================*
 *F: ͬ��ָ���豸�Ļ����������豸�߼���.
 *I: dev-�豸��
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
	SyncMinodes(); //ͬ���ļ�Minode
	//�ٴ�ͬ��, ����©��ͬ���ڵ�
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
 *F: ͬ���������е����п����豸�߼���.
 *I: void
 *O: void
 *==========================================================*/
void SyncAllDev()
{
	struct stBuf_Head *bh;
	int i;

	SyncMinodes(); //ͬ���ļ�Minode
	//�ٴ�ͬ��, ����©��ͬ���ڵ�
	bh = (struct stBuf_Head *)start;
	for(i = 0; i < NR_HEARDS; i++, bh++)
	{
		WaitonBuffer(bh);
		if(bh->uc_dirt)
			fnDev_RW_blk(WRITE, bh);
	}
}


/*==========================================================*
 *F: ��ָ�����豸�ڻ������е����ݿ���Ч.
 *I: dev-�豸��
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
			bh->uc_valid = bh->uc_dirt = 0;  //��Ч��־
	}
}


/*------------------------�����������������ϲ��ṩ����Ľӿں���-------------------*/
/*==========================================================*
 *F: �ͷ�ָ���Ļ����.
 *I: struct stBuf_Head * bh
 *O: void
 *==========================================================*/
void fnRelse_bufblk(struct stBuf_Head *bh)
{
	if(!bh)
		return;
	/* �ȴ����������� */
	WaitonBuffer(bh);
	if(!(bh->us_count--))
		panic("fnRelse_bufblk(): bh->us_count is 0");
	/* ���ѵȴ�����bh�Ľ��� */
	fnWakeUp(&glbWait_Bufblk);
}


/*==========================================================*
 *F: ���豸�ϵ����ݿ�.
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
	if(bh->uc_valid)  /* ���ݿ���Ȼ��Ч */
		return bh;
	/* �����豸���� */
	fnDev_RW_blk(READ, bh);  /* ��д��ʱ������ */
	/* �ȴ����������� */
	WaitonBuffer(bh);
	if(bh->uc_valid)
		return bh;
	fnRelse_bufblk(bh);  /* ����ʧ��,�ͷ�bh */
	printk(RED, "fnread_bufblk(): can't get valid bh\n");
	return 0;
}






