/*-----------------------------------------------------------------*
 *说明：处理物理内存页面与虚拟内存页面之间的影射关系。包括：
 *	  1,进程的页(目录)表操作;
 *	  2,处理虚拟内存缺页中断;
 *	  3,虚拟内存页面访问错误;
 *
 *备注：注意物理地址与线性地址的使用!
 *
 *修改时间：04/11/07  13:45
 *-----------------------------------------------------------------*/
#include  "..\include\process.h"

extern void fnFree_VmkPage(void * addrpage);
extern void * fnGet_VmkPage();
extern void fnSleepOn(struct PCB * *pwait);
extern void panic(char *szerror);
extern void vsprintf(char *buf, char *fmt, ...);
extern struct PCB *pCurrent;

#define	COMPLIER_BASEADDR   0XC0000000
//全局变量,等待可用的物理内存页面
//
struct PCB *glbWaitMMPage = 0;


/*=============================================================*
 *F: 释放页目录项，同时释放对应的页表和物理页面。
 *I: addrDir-页目录地址(内核中的线性),
 *   indexDirEntry-页页表在目录中的索引,
 *   numEntries-页表个数
 *O: void 
 *C: 主要用于“进程”退出时回收内存。注意numEntries的值!
 *=============================================================*/
void fnFree_PageDirEntries(void * addrDir, unsigned int indexDirEntry,
						   unsigned int numEntries)
{
	unsigned int i, j;
	
	if(!indexDirEntry || indexDirEntry >= 512)
		return;	 //0~4Mb属于保留虚拟空间, 0x80000000~0xffffffff属于内核空间,所以不能释放
	if(indexDirEntry + numEntries > 512)
		numEntries = 512 - indexDirEntry;
	//根据当前活动进程的页目录表
	for(i = 0; i < numEntries; i++)
	{
		//检查该目录项对应的页表是否存在
		if(((unsigned int *)addrDir)[indexDirEntry] & 0x00000001)
		{
			unsigned int * addr_pagetable; //转化成内核线性地址
			addr_pagetable = (unsigned int *) 
				(COMPLIER_BASEADDR + (((unsigned int *)addrDir)[indexDirEntry] & 0xfffff000 ));//取页表地址
			//检测页表项，并释放存在的pageFrame
			for(j = 0; j < 1024; j++)
			{
				if(addr_pagetable[j] & 0x00000001)
				{
					fnFree_VmkPage((void *)
						(COMPLIER_BASEADDR + (addr_pagetable[j] & 0xfffff000)));
					addr_pagetable[j] = 0; //页表项清零
				}
			}
			//释放页表占用的4Kb
			fnFree_VmkPage((void *)addr_pagetable);
			((unsigned int *)addrDir)[indexDirEntry] = 0;//页目录项清零
		}
		indexDirEntry++;
	}	
}


/*=============================================================*
 *F: 缺页异常处理函数
 *I: error_code-错误码, line_addr-发生缺页地址
 *O: void 
 *C: 通过检查error code和引起中断的线性地址，作出决定。
 *   没有尝试着与其它进程共享某些物理页面,没有使用虚拟存储技术.
 *=============================================================*/
void Handle_No_Page(unsigned int error_code, unsigned int line_addr)
{
	unsigned int *page_addr, *temp_addr;
	unsigned int *Current_PageDir;  //当前活动进程的页目录地址

_repeat:
	__asm{
		mov  eax, cr3
		mov  Current_PageDir, eax
	}
	//转化成内核线性地址
	Current_PageDir = (unsigned int *)((unsigned int)Current_PageDir + COMPLIER_BASEADDR); 
	line_addr &= 0xfffff000;  //获得发生缺页的页面地址
	//判断页目录项不存在
	if((Current_PageDir[line_addr >> 22] & 0x00000001) == 0)
	{
		if(!(page_addr = (unsigned int *)fnGet_VmkPage()))
		{
			//没有分配到物理内存页, 挂起当前进程
			fnSleepOn(&glbWaitMMPage);
			goto _repeat;
		}
		//修改对应的页目录项, 首先内核线性地址转化成物理地址
		page_addr = (unsigned int *)((unsigned int)page_addr - COMPLIER_BASEADDR);
		Current_PageDir[line_addr >> 22] = (unsigned int )page_addr + 7;																
	}
	//获取该线性地址的页表的物理地址
	page_addr = (unsigned int *)(Current_PageDir[line_addr >> 22] & 0xfffff000);
	//转化成内核线性地址
	page_addr = (unsigned int *)((unsigned int)page_addr + COMPLIER_BASEADDR); 
	if(!(temp_addr = (unsigned int *)fnGet_VmkPage()))
	{
		//没有分配到物理内存页, 挂起当前进程
		fnSleepOn(&glbWaitMMPage);
		goto _repeat;
	}
	//转化成物理地址
	temp_addr = (unsigned int *)((unsigned int)temp_addr - COMPLIER_BASEADDR );
	page_addr[(line_addr >> 12) & 0x03FF] = (unsigned int)temp_addr 
		+ ((line_addr >= (0x80000000 - 4*1024 ) || line_addr <= 4*1024*1024) ? 3 : 7);
}


/*===========================================================*
 *F: 页面保护异常处理函数
 *I: 
 *O: void 
 *C: 目前只做卤莽的处理
 *===========================================================*/
void Handle_Protect_Page(unsigned int error_code, unsigned int line_addr)
{
	char szErr[128];

	vsprintf(szErr, "panic:Page Protect! line_addr= 0x%x, pid= 0x%x\n",
		line_addr, pCurrent->pid); 
	panic(szErr);
}
