/*----------------------------------------------------------*
 *˵��:  ���ļ�����һЩ���ڡ����̿��ƽṹ���Ľṹ�ͳ�����
 *                      huangheshui  2007-06-17 15:10
 *----------------------------------------------------------*/
#ifndef __PROCESS_H
#define __PROCESS_H

#include "../file_c/fs/fs.h"
//������̼Ĵ��������Ľṹ,
//�ֳ�������ջkernel_stack_one
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
	//int ѹ��
	unsigned long eip;
	unsigned long cs;
	unsigned long eflags;
	unsigned long esp;  //����Ȩ���任
	unsigned long ss;   
};

//���̿��ƿ�ṹ�Ķ���
//���̵�registors�������ں˶�ջ��
//
struct PCB{
	//�����ں˶�ջss0, esp0
	unsigned long ss0;
	unsigned long esp0; 
	unsigned long cr3;
	unsigned long pid;
	char pname[32];
	unsigned long  status;	/* process's status */
	//�������ȼ�������ʱ��
	short counter;	        /* counter = counter/2 + priority */
	short priority;
	unsigned long kticks;
	unsigned long uticks;
	struct PCB *lpnext;
	//��������ʱϵͳ��Ϣ
	struct stMinode *pwd;   //���̵Ĺ���Ŀ¼Mi
	struct stMinode *root;  //��Ŀ¼Mi
	struct stMinode *exefile;  //��ִ���ļ���Mi
	struct TTY *ptty; 
};


//����״̬��־����
//
#define	PROC_READY	1000
#define	PROC_SLEEP	2000

//��ѡ�������
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











