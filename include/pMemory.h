/*---------------------------------------------------------------*
 *说明：声明和定义一些结构，这些数据结构是用于管理物理存储器的。	
 *      最后修改时间：04/10/07	20:13
 *                    2007-6-17 15:20
 *---------------------------------------------------------------*/
#ifndef __PMEMORY_H
#define __PMEMORY_H
//存储桶描述符结构16 bytes
//
struct bucket_desc{ 
	void *lppage;			    //该描述符所描述页面的地址
	struct bucket_desc *lpnext;	//下一个桶描述符
	void *lpfree;			    //指向本桶中空闲内存小块链表头部的指针
	unsigned short refcount;    //已经被分配掉小块的个数
	unsigned short bucket_size;	//该桶的大小，单位:bytes
};

//存储桶目录项的结构8 bytes
//
struct bucket_dir_entry{
	unsigned short size;		 //该小组桶分配单位(size 个bytes)
	struct bucket_desc *chain;   //“桶描述符”链表的头结点指针
};

//下面是存储桶目录表，描述了可以申请到的内存小块的信息。
//可以一次性申请到的最大内存块是4096 bytes ,也就是整个桶了。
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
	{0,    (struct bucket_desc *)0  } //0表示结束
};

//空闲“桶描述符”链表的头指针
//
struct bucket_desc *lpfree_bucket_desc = (struct bucket_desc *)0;

#endif //__PMEMORY_H