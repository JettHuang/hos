/*---------------------------------------------------------------*
 *˵���������Ͷ���һЩ�ṹ����Щ���ݽṹ�����ڹ�������洢���ġ�	
 *      ����޸�ʱ�䣺04/10/07	20:13
 *                    2007-6-17 15:20
 *---------------------------------------------------------------*/
#ifndef __PMEMORY_H
#define __PMEMORY_H
//�洢Ͱ�������ṹ16 bytes
//
struct bucket_desc{ 
	void *lppage;			    //��������������ҳ��ĵ�ַ
	struct bucket_desc *lpnext;	//��һ��Ͱ������
	void *lpfree;			    //ָ��Ͱ�п����ڴ�С������ͷ����ָ��
	unsigned short refcount;    //�Ѿ��������С��ĸ���
	unsigned short bucket_size;	//��Ͱ�Ĵ�С����λ:bytes
};

//�洢ͰĿ¼��Ľṹ8 bytes
//
struct bucket_dir_entry{
	unsigned short size;		 //��С��Ͱ���䵥λ(size ��bytes)
	struct bucket_desc *chain;   //��Ͱ�������������ͷ���ָ��
};

//�����Ǵ洢ͰĿ¼�������˿������뵽���ڴ�С�����Ϣ��
//����һ�������뵽������ڴ����4096 bytes ,Ҳ��������Ͱ�ˡ�
//
struct bucket_dir_entry Bucket_Dir[] = {
	{16,   (struct bucket_desc *)0  },
	{32,   (struct bucket_desc *)0  },
	{64,   (struct bucket_desc *)0  },
	{128,  (struct bucket_desc *)0  },
	{256,  (struct bucket_desc *)0  },
	{512,  (struct bucket_desc *)0  },
	{1024, (struct bucket_desc *)0  },
	{2048, (struct bucket_desc *)0  },
	{4096, (struct bucket_desc *)0  },
	{0,    (struct bucket_desc *)0  } //0��ʾ����
};

//���С�Ͱ�������������ͷָ��
//
struct bucket_desc *lpfree_bucket_desc = (struct bucket_desc *)0;

#endif //__PMEMORY_H