/*--------------------------------------------------------*
 *   ���豸ͷ�ļ�, Ϊ�˱�����linux��һ�£���Щ�ط���ȫ��
 *ʹ���ˡ�
 *
 *--------------------------------------------------------*/
#ifndef __BLK_H
#define __BLK_H

#include "fs\buffer.h"

#define NR_BLK_DEV	  7     /* ���豸������ */
#define NR_REQUEST	  48    /* һ����48�������� */

/* ������ṹ���� */
struct stRequestItem{
	int dev;	                        /* ʹ�ø�����豸��, -1��ʾ���� */
	int cmd;	                        /* READ OR WRITE���� */
	int errors;                         /* ����ʱ�����Ĵ������ */
	unsigned int begin_sector;          /* ��ʼ���� */
	unsigned int nr_sectors;            /* ����д������ */
	unsigned char *pdata_buf;           /* ���ݻ����� */
	struct PCB *waiting;                /* ������û���õ� */
	struct stBuf_Head *bh;              /* ��������head */
	struct stRequestItem *pnext;        /* ��һ�������� */
};

/* ���豸�ṹ,������<�豸��������>�����豸��I/O������� */
struct stBlk_dev_IO{
	void (*pfn_doRequest)();                /* �豸I/O����Ĵ����� */
	struct stRequestItem *current_request;  /* ��Ӧ��������� */
};


/*
 * ���ݵ����㷨�����ȱȽ�ԭ��, cmd --> dev --> sector
 * s1������s2,�򷵻�true
 * value read < value write
 */
#define IN_ORDER(s1, s2) \
	((s1)->cmd < (s2)->cmd || (s1)->cmd == (s2)->cmd && \
		((s1)->dev < (s2)->dev || \
		((s1)->dev == (s2)->dev && (s1)->begin_sector < (s2)->begin_sector)))

#define MAJOR(a) (((unsigned short)(a)) >> 8)  /* ���ֽ����豸�� */
#define MINOR(a) ((a) & 0xff)			       /* ���ֽڴ��豸�� */

#endif /*__BLK_H*/



























