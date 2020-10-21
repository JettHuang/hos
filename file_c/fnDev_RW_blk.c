/*---------------------------------------------------*
 *˵��: Ϊ�ϲ�buffer�ṩ�ײ��豸�Ķ�д����
 *
 *                      huangheshui  2007-06-18  12:38
 *---------------------------------------------------*/
#include "..\include\process.h"
#include "..\include\blk.h"
#include "fs\buffer.h"
#include "..\include\asm.h"
#include "..\include\color.h"


extern void fnSleepOn(struct PCB * *pwait);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);

/* ���豸���� */
struct stBlk_dev_IO  glbBlk_devs[NR_BLK_DEV] = {
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0},
		{0, 0}
}; 
struct stRequestItem  glbRequest_items[NR_REQUEST]; /*IO������������*/
struct PCB * glbWaitReqitem; /*���صȴ�������Ľ���*/


/*=======================================================*
 *F: �ͷ������Ļ�������bh
 *I: bh-��������������ָ��
 *O: void
 *=======================================================*/
void unlock_blkbuf(struct stBuf_Head *bh)
{
	if(!bh->uc_lock)
		printk(RED, "warning: free buffer being unlocked!");
	bh->uc_lock = 0;
	fnWakeUp(&bh->task_wait); /* ���ѵȴ�bh�Ľ��� */
}


/*=======================================================*
 *F: ����ָ���Ļ�������bh
 *I: bh-��������������ָ��
 *O: void
 *=======================================================*/
void lock_blkbuf(struct stBuf_Head *bh)
{
	Cli()
	while(bh->uc_lock)
		fnSleepOn(&bh->task_wait);
	bh->uc_lock = 1;
	Sti()
}


/*=======================================================*
 *F: ��ָ�����豸IO�����һ��������
 *I: pdev-���豸�ṹָ��, preq-������ָ��
 *O: void
 *=======================================================*/
void add_request(struct stBlk_dev_IO *pdev, struct stRequestItem *preq)
{
	struct stRequestItem *tmp;

	preq->pnext = 0;
	//���ж�, ��ֹ���豸IO���
	Cli()
	if(preq->bh)
		preq->bh->uc_dirt = 0;          /* ���"��"��־ */
	if(!(tmp = pdev->current_request))  /* ֮ǰû�������� */
	{
		pdev->current_request = preq;
		Sti()
		(pdev->pfn_doRequest)();        /* �����豸IO���� */
		return;
	}
	/* �Ѿ��������������ڵȴ�,���õ��ݵ����㷨 */
	for(; tmp->pnext; tmp = tmp->pnext)
	{
		if((IN_ORDER(tmp, preq) || !IN_ORDER(tmp, tmp->pnext)) &&
			IN_ORDER(preq, tmp->pnext)
		  )
		  break;  /* �ҵ��˲���� */
	}
	preq->pnext = tmp->pnext;
	tmp->pnext = preq;
	Sti()
}


/*=======================================================*
 *F: ��������������������
 *I: major-���豸��, rwcmd-��д������, bh-��������������ָ��
 *O: void
 *=======================================================*/
void make_request(int major, int rwcmd, struct stBuf_Head *bh)
{
	struct stRequestItem *req;

	if(!bh)
	{
		printk(RED, "warning:make_request() bh==0!");
		return;
	}
	if(rwcmd != READ && rwcmd != WRITE)
	{
		printk(RED, "error:make_request() rwcmd is error!");
		return;
	}
	lock_blkbuf(bh);
	/*����Ƿ��б�Ҫ��д*/
	if((rwcmd == WRITE && !bh->uc_dirt) || (rwcmd == READ && bh->uc_valid))
	{
		unlock_blkbuf(bh);
		return;
	}
_repeat:
	if(rwcmd == READ)
		req = glbRequest_items + NR_REQUEST;  /* ������ӵ�е�������� */
	else
		req = glbRequest_items + ((NR_REQUEST * 2) / 3);  /* д������1/3 */
	/* ���������� */
	while(--req <= glbRequest_items)
		if(req->dev < 0)
			break;
	if(req < glbRequest_items)  /* û���ҵ� */
	{
		fnSleepOn(&glbWaitReqitem);
		goto _repeat;
	}
	/* ��д������ṹ�е�������Ϣ */
	req->dev = bh->usdev_id;
	req->cmd = rwcmd;
	req->errors = 0;
	req->begin_sector = bh->uldev_blknr << 1;  /* ��ʼ���� */
	req->nr_sectors = 2;
	req->pdata_buf = bh->pb_data;
	req->waiting = 0;  /* ����û��ʹ�� */
	req->bh = bh;
	req->pnext = 0;
	add_request(glbBlk_devs + major, req);  /* ������Ӧ�豸��������� */
}


/*=======================================================*
 *F: ����һ��IO������
 *I: major-���豸��, update-ˢ�±�־
 *O: void
 *=======================================================*/
void fnEnd_request(int major, int update)
{
	if(glbBlk_devs[major].current_request->bh)
	{
		glbBlk_devs[major].current_request->bh->uc_valid = (unsigned char)update;
		unlock_blkbuf(glbBlk_devs[major].current_request->bh);
	}
	if(!update)
		printk(RED, "fnEnd_request(): update ==0\n");
	/* �ͷ�requestitem */
	glbBlk_devs[major].current_request->dev = -1;
	glbBlk_devs[major].current_request = glbBlk_devs[major].current_request->pnext;
	fnWakeUp(&glbWaitReqitem);
}


/*=======================================================*
 *F: ��дһ�����豸�ϵ����ݿ�,��������������.
 *I: rwcmd-��д������, bh-��������������ָ��
 *O: void
 *C: Ϊ�ϲ㻺�����ṩ����ӿ�.
 *=======================================================*/
void fnDev_RW_blk(int rwcmd, struct stBuf_Head *bh)
{
	unsigned int major;  /* ���豸�� */

	if((major = MAJOR(bh->usdev_id)) > NR_BLK_DEV ||
		!(glbBlk_devs[major].pfn_doRequest))
	{
		printk(RED, "warning:fnDev_RW_blk()try to read nonexistent blkdev!\n");
		return;
	}
	make_request(major, rwcmd, bh);  /* �����д */
}


/*=======================================================* 
 *F: ��ʼ����������.
 *I: void
 *O: void
 *C: ��StartMain()�е���
 *=======================================================*/
void blk_dev_init()
{
	int i;

	for(i = 0; i < NR_REQUEST; i++)
	{
		glbRequest_items[i].dev = -1;
		glbRequest_items[i].pnext = 0;
	}
}





























