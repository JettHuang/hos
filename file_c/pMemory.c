/*-------------------------------------------------------------*
 * 说明：  pMemory.c 文件实现的物理内存管理的一些函数，包括：
 *	    1，分配一个物理内存页面
 *	    2，释放一个物理内存页面
 *	    对于不足一个页面的内存的操作有：
 *	    3，申请指定大小的内存块 fnPm_alloc(...)
 *	    4，释放指定位置的内存块 fnPm_free(...)
 *      还有其它的一些辅助性函数。
 * 备注：  上面的函数都是在内核空间中分配内存的，所得到的内存地址
 *      是线性地址，若转化为实际的物理地址需要 retValue - COMPLIER_BASEADDR
 *	    Pm ---physical memory 
 *	    Vmk ---virtual Kernel memory / line Kernel memory	 
 * 应该记住：
 *	   物理内存0 -- 256M  =======>  内核地址 0xC0000000 -- 0xD0000000
 *
 * 修改时间：04/10/07  20:30 
 *           2007-06-16 21:03
 *-------------------------------------------------------------*/
#include "..\include\pMemory.h"
#include "..\include\color.h"
#include "..\include\process.h"

extern void vsprintf(char *buf, char *fmt, ...);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);
extern struct PCB *glbWaitMMPage;

#define	PAGE_SIZE	        4096	
#define	KERNEL_PAGES	    64  //假设目前内核占用64pages
#define	COMPLIER_BASEADDR	0xC0000000
#define BITS_CHAR	(sizeof(unsigned char) * 8) 
//说明： 物理内存页面使用分配位图。
//	  每个bit表示一个物理页面，bit == 1 表示被占用；bit == 0 表示没占用。
//    可表示的物理内存大小是 8 * 1024 * 8 * 4k = 256MB
//
unsigned char glbPm_Bitmap[8*1024];	
unsigned int uglbTotal_PmPages;   //物理内存页面总量


/*=============================================*
 *F: 申请一个物理内存页面
 *I: void 
 *O: 0-申请失败，!0-内核中对应的线性地址
 *=============================================*/
void * fnGet_VmkPage()
{
	unsigned int i, *addrpage;

	for(i = 0; i < uglbTotal_PmPages; i++)
	{
		if(0 == (glbPm_Bitmap[i / BITS_CHAR] & (1 << (i & (BITS_CHAR - 1)))))
		{
			//更新bitmap
			glbPm_Bitmap[i / BITS_CHAR] |= (1 << (i & (BITS_CHAR - 1)));	 
			//计算物理页面地址映射的内核地址
			addrpage = (unsigned int *)(i * 1024 * 4 + COMPLIER_BASEADDR);		
			//在内核空间初始化物理内存 zero!
			for(i =0; i < PAGE_SIZE/sizeof(unsigned int); i++)
				addrpage[i] = 0; 
			return (void *)(addrpage);
		}
	}
	return  0;
}


/*=============================================*
 *F: 释放一个物理内存页面
 *I: 物理页面在内核中映射地址
 *O: void
 *C: 对于失败的情况没有作出过激的处理。 
 *=============================================*/
void fnFree_VmkPage(void * addrpage)
{
	unsigned int  uPageId;

	uPageId = ((unsigned int)addrpage - COMPLIER_BASEADDR) >> 12;
	//判断地址是否合法
	if((uPageId < KERNEL_PAGES) || (uPageId >= uglbTotal_PmPages))
		return;
	//更新bitmap
	glbPm_Bitmap[uPageId / BITS_CHAR] &= (1 << (uPageId & (BITS_CHAR - 1))) ^ 0xFF;
	fnWakeUp(&glbWaitMMPage);
}


/*----------------------------------------------------------------------* 
 *插曲:    为了使内核能够申请少量物理内存，我们采取“存储桶”法来实现少量
 *     内存的分配和释放。
 *	   mallock(unsigned int size);  
 *	   mfreek(void *lpblock);
 *----------------------------------------------------------------------*/

/*=======================================================================*
 *F: 建立并初始化“空闲描述符链表”.
 *I: void
 *O: 0-失败，!0-成功
 *C: 当lpfree_bucket_desc == NULL时，说明此时没有可用的“桶描述符”来描述
 *   一桶(一页)内存，这种情况可能发生在第一次调用mallock(..),或者是以前的
 *   都被用光了；申请一个内存页面来创造“桶描述符”，用来描述<桶>。
 *=======================================================================*/
short  fnInit_Bucket_Desc()
{
	struct bucket_desc *lptemp, *lpfirst;
	unsigned int i;

	//先申请一个物理页面PAGE_SIZE
	lpfirst = lptemp = (struct bucket_desc *)fnGet_VmkPage();
	if(!lpfirst)
		return  0 ;
	//初始化该页面，构成桶描述符链表
	for(i = PAGE_SIZE / sizeof(struct bucket_desc); i > 1; i--)
	{
		lptemp->lpnext = lptemp + 1; //紧接着
		lptemp++;
	}
	lptemp->lpnext = lpfree_bucket_desc; //最后一个桶描述符的lpnext
	lpfree_bucket_desc = lpfirst;
	return  1;
}


/*=====================================================*
 *F: 申请<=4KB的内存。
 *I: 内存大小(单位:字节)
 *O: 0-失败，!0-成功，且为小块内存在内核地址空间的首地址。
 *=====================================================*/
void * fnVmk_alloc(unsigned int length)
{
	struct bucket_dir_entry *bdir;
	struct bucket_desc *bdesc;
	void *retval;
	unsigned int i, temp;

	//搜索整个“存储桶目录”寻找合适的桶
	for(bdir = Bucket_Dir; bdir->size; bdir++)
	{
		if(bdir->size >= length)
			break; //找到了
	}
	if(!bdir->size)
		return 0;
	//现在开始进入该“桶描述符”链表，进行地毯式搜索。
	for(bdesc = bdir->chain; bdesc; bdesc = bdesc->lpnext)
	{
		if(bdesc->lpfree)
			break ; //找到一桶可以提供小块内存!
	}	
	if(!bdesc)
	{
		//该链表上的所有桶都被用光了,申请“桶描述符”和“桶”
		if(!lpfree_bucket_desc) //桶描述符也光了
		{
			if(!fnInit_Bucket_Desc())
				return 0;
		}
		//从空闲桶描述符链表前端取下一个描述符
		bdesc = lpfree_bucket_desc;
		lpfree_bucket_desc = bdesc->lpnext;
		//初始化这个描述符
		bdesc->refcount = 0;
		bdesc->bucket_size = bdir->size;
		bdesc->lppage = bdesc->lpfree = (void *)fnGet_VmkPage();
		temp = (unsigned int)(bdesc->lppage);
		if(!bdesc->lppage)
			return 0;

		//对该桶进行分割，与“空闲描述符”的初始化类似。
		for(i = PAGE_SIZE / bdir->size; i >1; i--)
		{
			*((unsigned int *)temp) = temp + bdir->size;
			temp += bdir->size;
		}
		*((unsigned int *)temp) = 0;  //最后一块指向NULL
		//上面建立了“描述符”与“桶”的映射，接下来将该描述符加入桶目录项的链表头部。
		bdesc->lpnext = bdir->chain;
		bdir->chain = bdesc;		
	}
	//条件允许了,开始分配内存object
	retval = (void *)(bdesc->lpfree);
	bdesc->lpfree = *(void * *)retval;  //内存小块的前4bytes point to next free block.
	bdesc->refcount++;
	return retval;
}


/*=====================================================*
 *F: 释放<=4KB的内存。
 *I: 内存小块的addr, 内存大小(单位:字节)
 *O: void 
 *C: 与mallock(..)呼应，如果能够在参数中提供块的length,
 *   将释放的更快。如果不能请set length = 0 ,我们将地毯式
 *   搜索; 对于失败的情况没有作出过激的处理。 
 *=====================================================*/
void fnVmk_free(void *lpblock, unsigned int length)
{
	void *page;
	struct bucket_dir_entry *bdir;
	struct bucket_desc *bdesc, *prev;

	//计算该object所在的页面
	page = (void *)((unsigned int)lpblock & 0xfffff000);
	//循环搜索“桶目录项”,寻找page
	for(bdir = Bucket_Dir; bdir->size; bdir ++)
	{
		prev = 0;
		//if length==0 ,then 不会执行continue
		if(bdir->size < length)
			continue;	
		//此时,length ==0,那么将地毯式搜索page
		for(bdesc = bdir->chain; bdesc; bdesc = bdesc->lpnext)
		{
			if(bdesc->lppage == page)  //一个“桶描述符”对应一个“页面”
				goto _found;
			prev = bdesc;
		}
	}
	//没有找到就返回
	return;
_found:
	//将这个小块加入到“空闲队列头部”
	*(void **)lpblock = bdesc->lpfree;
	bdesc->lpfree = lpblock;
	bdesc->refcount--;  //引用次数减一
	//如果引用次数==0，那么我们释放该“桶描述符”和对应的“桶”
	if(bdesc->refcount == 0)
	{
		if(prev)
			prev->lpnext = bdesc->lpnext;
		else
			bdir->chain = bdesc->lpnext;
		//删除“桶”,并将描述符加入到空闲“桶描述符”头部
		fnFree_VmkPage((void *)bdesc->lppage);
		bdesc->lpnext = lpfree_bucket_desc;
		lpfree_bucket_desc = bdesc;
	}
	return;
}


/*=====================================================*
 *F: 将一块内存信息复制到 另一个块。
 *I: char * dest, char * sour, unsigned int size(bytes)
 *O: void
 *=====================================================*/
void fnVmkCpy(char *dest, char *sour, unsigned int size)
{
	unsigned int i;
	for(i=0; i< size; i++)
		*dest++ = *sour++; 
}

/*=====================================================*
 *F: 初始化物理内存管理信息。
 *C: 在StratMain()中调用
 *=====================================================*/
void fnInit_Mem()
{
	unsigned int i;
	char szbuf[128];

	for(i = 0; i < 8 * 1024; i++)
		 glbPm_Bitmap[i] = 0;
	//标志内核占用区 0 -- (KERNEL_PAGES - 1)
	for(i = 0; i < KERNEL_PAGES; i++)
	{
		glbPm_Bitmap[i / BITS_CHAR] |= (1 << (i & (BITS_CHAR-1)));
	}
	//标志视频和BIOS,页表,FileBuffer占用区(舍去了一些),范围0x0A0000 ~ 0x400000
	for(i = (0x0A0000 >>12); i < (0x400000 >> 12); i++)
	{
		glbPm_Bitmap[i / BITS_CHAR] |= (1 << (i & (BITS_CHAR - 1)));
	}
	//显示物理内存大小
	vsprintf(szbuf, "size of RAM is: 0x%x * 4kb\n", uglbTotal_PmPages);
	printk(WHITE, szbuf);
}