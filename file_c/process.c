/*-------------------------------------------------------------*
 *说明:  关于进程的一些控制函数
 *       进程的一些registors被保护在内核堆栈上,而其他的一些与
 *    registors无关的信息保存在PCB中。
 *                               2007-06-17 13:38
 *-------------------------------------------------------------*/
#include  "..\include\process.h"
#include  "..\include\color.h"

extern void * fnGet_VmkPage();
extern void * fnVmk_alloc(unsigned int length);
extern void fnVmkCpy(char *dest, char *sour, unsigned int size);
extern void fnVmk_free(void *lpblock, unsigned int length);
extern unsigned int strcpy(char *dest, char *sour);
extern void printk(unsigned long color, const char *lpstr);
extern int CreateTTY(struct PCB *p);


#define	COMPLIER_BASEADDR  0xC0000000
//本模块中的函数声名
//
unsigned int fnWriteProcVM(struct PCB *pcb, unsigned int pstart,
						   char *pbuf, unsigned int ulen);
void switch_to(struct PCB *to);
void Scheduler();

//关于进程链表的全局变量,
//
//
struct PCB *pAllProcess;       //包含系统中的所有进程
struct PCB *pIdle;	           //进程0-idle指针
struct PCB *pCurrent;          //当前正在运行的进程
unsigned int uTotalProc = 0;   //当前系统中进程的总数
unsigned int glbss0, glbesp0;  //通过全局变量方便临时传递ss, sep


/*========================================================*
 *F: 初始化进程idle process 0
 *I: void
 *O: void
 *C: 在StartMain()中调用
 *========================================================*/
void fnInit_IdleProcess()
{
	struct PCB * p;
	int jmpcode = 0xFEEB;  //jmp $

	p = (struct PCB *)fnVmk_alloc(sizeof(struct PCB));
	p->cr3 = 0;
	p->ss0 = Select_Kernel_Stack32;
	p->esp0 = 0x80000000;
	p->pid = 0;
	strcpy(p->pname, "idle process");
	p->counter = 0;
	p->priority = 0;  //优先级别
	p->uticks = p->kticks = 0;
	uTotalProc = 1;
	pIdle = p;
	pAllProcess = 0;
	pCurrent = p;
	//添加到pAllProcess链表
	p->lpnext = pAllProcess;
	pAllProcess = p;
	//为内核分配一个页面
	fnWriteProcVM(p, 0x80000000 - 64, "kernel", 6);
	//向0x00401000写入代码 jmp $
	fnWriteProcVM(p, 0x00401000, (char*)&jmpcode, 2);
}


/*========================================================*
 *F: 创建一个新的进程
 *I: ueip-起始代码指针, prior-进程优先权
 *O: 0-创建失败, !0-进程id
 *C: 注意第一个进程idle的id是0
 *========================================================*/
unsigned int fnCreateProcess(unsigned int ueip, short prior)
{	
	struct RegistersGroup regs;
	struct PCB * p;
	unsigned short opcodes[2];

	opcodes[0] = 0x80cd;  //int 80h
	opcodes[1] = 0xFEEB;  //jmp $	
	//初始化进程的内核堆栈,存放被中断的用户模式context
	regs.cs = Select_Local_Code32;
	regs.ds = Select_Local_Data32;
	regs.es = Select_Local_Data32;
	regs.fs = Select_Local_Data32;
	regs.gs = Select_Local_Data32;
	regs.ss = Select_Local_Stack32; //用户模式的ss3
	regs.esp = 0x40000000;			//esp3
	regs.eflags = 0x200;
	regs.eip = ueip;
	
	if(!(p = (struct PCB *)fnVmk_alloc( sizeof(struct PCB ))))
		return 0;
	if(!(p->cr3 = (unsigned int)fnGet_VmkPage()))
	{
		fnVmk_free(p, sizeof(struct PCB ));
		return 0;
	}
	//将进程idle的页目录的0x80000000 --0xffffffff复制给p->cr3
	fnVmkCpy((char *)((unsigned int*)p->cr3 + (0x80000000>>22)),
		(char *)((unsigned int*)COMPLIER_BASEADDR + (0x80000000>>22)), 
		512 * sizeof(unsigned int));

	p->cr3 = p->cr3 - COMPLIER_BASEADDR;  //计算物理地址
	p->ss0 = Select_Kernel_Stack32;
	p->esp0 = 0x80000000 - sizeof(struct RegistersGroup);
	p->pid = uTotalProc++;
	strcpy(p->pname, "taskA");
	p->counter = 10;
	p->priority = prior;  //优先级别
	p->uticks = p->kticks = 0;
	p->status = PROC_READY;
	//添加到pAllProcess链表
	p->lpnext = pAllProcess;
	pAllProcess = p;
	//为内核堆栈分配一个页面
	fnWriteProcVM(p, p->esp0, (char*)&regs, sizeof(struct RegistersGroup));
	//向0x00401000写入代码 jmp $
	fnWriteProcVM(p, 0x00401000, (char*)opcodes, 4);
    CreateTTY(p);
	return p->pid;
}


/*========================================================*
 *F: 当前进程主动进入不可中断睡眠状态,等待唤醒
 *I: pwait-需要睡眠的进程挂在其上
 *O: void
 *========================================================*/
void fnSleepOn(struct PCB * *pwait)
{
	struct PCB *plast;

	if(!pwait)
		return;
	plast = *pwait;	 //将前一个等待进程存放在current的堆栈上
	*pwait = pCurrent;
	pCurrent->status = PROC_SLEEP;
	Scheduler();
	//醒来后将唤醒上一个等待进程
	if(plast)
		plast->status = PROC_READY;
}


/*========================================================*
 *F: 唤醒某个睡眠队列上的进程
 *I: pwait - 睡眠进程队列所在指针
 *O: void
 *========================================================*/
void fnWakeUp(struct PCB * *pwait)
{
	if(pwait && *pwait)
	{
		(*pwait)->status = PROC_READY;
		*pwait = 0;
	}	
}


/*========================================================*
 *F: 对一个进程的虚拟空间进行写操作
 *I: pcb-目标进程控制块
 *   pstart-起始地址
 *   pbuf-书写内容缓冲区地址
 *   ulen-内容长度(单位:字节)
 *O: 0-失败, !0-成功
 *C: 没有对写的空间范围限制
 *========================================================*/
int fnWriteProcVM(struct PCB *pcb, unsigned int pstart,
				  char *pbuf, unsigned int ulen)
{
	unsigned int *pagetab, *page, lineaddr; 
	unsigned int *pagedir; //进程的页目录地址

	if(!ulen || !pcb || (pcb->pid && !pcb->cr3))
		return 0;
	//通过页目录表和页表对地址进行转换: vaddr--->paddr--->kaddr
	pagedir = (unsigned int *)(pcb->cr3 + COMPLIER_BASEADDR);  //转化成内核线性地址
_nextpage:
	lineaddr = pstart & 0xfffff000;  //获得将写页面的地址
	//页目录项不存在,申请一个页表
	if((pagedir[lineaddr >> 22] & 0x00000001) == 0)
	{
		if(!(pagetab = (unsigned int *)fnGet_VmkPage()))
		{
			printk(RED, "PageFrame exhaust!");
			return  0;
		}
		//修改对应的页目录项
		pagetab = (unsigned int *)
			((unsigned int)pagetab - COMPLIER_BASEADDR);  //转化成物理地址
		pagedir[lineaddr >> 22] = (unsigned int )pagetab + 7 ;	
	}
	//获取该线性地址的页表的物理地址
	pagetab = (unsigned int *)(pagedir[lineaddr >> 22] & 0xfffff000);
	pagetab = (unsigned int *)
		((unsigned int)pagetab + COMPLIER_BASEADDR); //转化成内核线性地址

	//开始操作页表
	//页表项不存在(缺页)
	if((pagetab[(lineaddr >> 12) & 0x03FF] & 0x00000001) == 0)
	{
		if(!(page=(unsigned int *)fnGet_VmkPage()))
		{
			printk( RED ,"PageFrame exhaust!");
			return 0;
		}
		//此时page是内核线性地址
		pagetab[(lineaddr>>12) & 0x03FF] = ((unsigned int)page - COMPLIER_BASEADDR)
				+ ((pstart >= (0x80000000 - 4*1024) || pstart <= 4*1024*1024) ? 3:7);
	}
	else
	{
		page = (unsigned int *)(pagetab[(lineaddr>>12) & 0x03FF] & 0xfffff000); 
		page = (unsigned int *)
			((unsigned int)page + COMPLIER_BASEADDR); //转化成内核线性地址
	}
	//开始写入物理内存page
	do {
		*((char*)page + (pstart & 0xfff)) = *pbuf++;
		pstart++;
		ulen--;
	}while((pstart & 0xfff) && ulen);
	//判断是否写完,否则写下一页
	if(ulen)
	{
		goto _nextpage;
	}
	return 1;
}


/*========================================================*
 *F: 进程调度程序
 *I: void
 *O: void
 *C: 参考linux0.11
 *========================================================*/
void 
Scheduler()
{
	struct PCB *tmp, *cand;
	int max;
	
	while(1)
	{
		max = -1;
		cand = pIdle;	//先将候选进程设置为idle
		for(tmp = pAllProcess; tmp; tmp = tmp->lpnext)
		{
			if((tmp != pIdle) && (tmp->status == PROC_READY) && (tmp->counter > max))
			{
				cand = tmp;
				max = tmp->counter;
			}
		}
		if(max)
			break;
		//执行到这里说明所有就绪进程的时间片都已经用完, 所以
		//更新每个进程的counter, 然后重新比较选出candidater
		for(tmp = pAllProcess; tmp ;tmp = tmp->lpnext)
		{
			tmp->counter = (short)((tmp->counter >> 1) + tmp->priority);
		}
	}//end while
	switch_to(cand);
}


/*========================================================*
 *F: 将CPU的使用权从当前进程切换到目标进程
 *I: to-目标进程的控制块指针
 *O: void
 *C: 注意在切换时,存在特权级转移的情况,如：一个新建立的进程
 *   投入运行
 *========================================================*/
void switch_to(struct PCB *to)
{
	unsigned int dir;

	if(pCurrent == to)
		return;
	//保护当前运行进程pCurrent的内核寄存器组上下文(kernel context)
	__asm{
		pushfd
		push cs
		push OFFSET _restart  //当它再次被切换回来时, 就从_restart处接着运行
		push eax 
		push ebx
		push ecx 
		push edx
		push edi
		push esi
		push ebp
		push ds
		push es
		push fs
		push gs
	}
	//保存当前进程的ss0、esp0
	__asm{
		xor eax, eax
		mov ax, ss
		mov glbss0, eax
		mov eax, esp
		mov glbesp0, eax
	}
	pCurrent->ss0 = glbss0;
	pCurrent->esp0 = glbesp0;
	glbss0 = to->ss0;
	glbesp0 = to->esp0;
	//将页目录寄存器cr3设置成目标进程的页目录地址
	pCurrent = to;
	dir = to->cr3;
	__asm{
		mov eax, dir
		mov cr3, eax  //页表已经更换刷新,堆栈切换到目标进程的内核堆栈(kernel stack)
		mov eax, glbss0
		mov ss, ax
		mov eax, glbesp0
		mov esp, eax
		//恢复目标进程的寄存器上下文
		pop gs
		pop fs
		pop es
		pop ds
		pop ebp
		pop esi
		pop edi
		pop edx
		pop ecx
		pop ebx
		pop eax
		iretd
_restart:
	}
}