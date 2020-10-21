/*----------------------------------------------------------*
 *说明:  该文件定义一些关于“进程控制结构”的结构和常量。
 *                      huangheshui  2007-06-17 15:10
 *----------------------------------------------------------*/
#ifndef __PROCESS_H
#define __PROCESS_H

#include "../file_c/fs/fs.h"
//定义进程寄存器上下文结构,
//现场保护堆栈kernel_stack_one
//
struct RegistersGroup{
	//segment registers
	unsigned long gs;
	unsigned long fs;
	unsigned long es;
	unsigned long ds;
	//general registers
	unsigned long ebp;	
	unsigned long esi;
	unsigned long edi;
	unsigned long edx;
	unsigned long ecx;
	unsigned long ebx;
	unsigned long eax;
	//int 压入
	unsigned long eip;
	unsigned long cs;
	unsigned long eflags;
	unsigned long esp;  //有特权级变换
	unsigned long ss;   
};

//进程控制块结构的定义
//进程的registors保存在内核堆栈中
//
struct PCB{
	//进程内核堆栈ss0, esp0
	unsigned long ss0;
	unsigned long esp0; 
	unsigned long cr3;
	unsigned long pid;
	char pname[32];
	unsigned long  status;	/* process's status */
	//进程优先级与运行时间
	short counter;	        /* counter = counter/2 + priority */
	short priority;
	unsigned long kticks;
	unsigned long uticks;
	struct PCB *lpnext;
	//进程运行时系统信息
	struct stMinode *pwd;   //进程的工作目录Mi
	struct stMinode *root;  //根目录Mi
	struct stMinode *exefile;  //可执行文件的Mi
	struct TTY *ptty; 
};


//进程状态标志常量
//
#define	PROC_READY	1000
#define	PROC_SLEEP	2000

//段选择符常量
//
#define	SA_RPL0		0	
#define	SA_RPL1		1
#define	SA_RPL2		2
#define	SA_RPL3		3
#define	SA_TIG		0
#define	SA_TIL		4

#define	Select_Local_Code32		(0x00 + SA_RPL3 + SA_TIL)
#define	Select_Local_Data32		(0x08 + SA_RPL3 + SA_TIL)
#define	Select_Local_Stack32	(0x10 + SA_RPL3 + SA_TIL)
#define	Select_Kernel_Code32    0x08 
#define	Select_Kernel_Data32    0x10 
#define	Select_Kernel_Stack32   0x18 

#endif //__PROCESS_H











