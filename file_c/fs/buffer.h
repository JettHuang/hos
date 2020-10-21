#ifndef __BUFFER_H
#define __BUFFER_H

/* ����buffer_head�Ľṹ */
struct stBuf_Head{
	unsigned char *pb_data;     /* �������е����ݿ�ָ�� */
	unsigned long uldev_blknr;	/* ĳ�豸�ϵ��߼���� */
	unsigned short usdev_id;	/* �豸id */
	unsigned char uc_valid;     /* 0 - invalid,1- valid ��Ӧ���ݿ��Ƿ���Ч�������豸��ʧ��ʱ��Ч */
	unsigned char uc_dirt;	    /* 0 - clean , 1- dirty */
	unsigned short us_count;    /* ��ǰ�ÿ鱻���õĴ���, 0 -- free */
	unsigned char  uc_lock;	    /* 0 - not locked , 1 - locked ;��д��ʱ��Ҫ���� */
	struct PCB *task_wait;      /* �ȴ��ÿ�Ľ��� */
	struct stBuf_Head *bh_prev; 
	struct stBuf_Head *bh_next;
/* ʵ����,free_list ���������л���� */
	struct stBuf_Head *bh_prev_free;
	struct stBuf_Head *bh_next_free;
};

#define READ			0x1000
#define WRITE			0x2000
#define	BLOCK_SIZE	    1024	

#endif /*__BUFFER_H*/