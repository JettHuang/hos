/*-------------------------------------------------------------*
 *˵��:  ���ڽ��̵�һЩ���ƺ���
 *       ���̵�һЩregistors���������ں˶�ջ��,��������һЩ��
 *    registors�޹ص���Ϣ������PCB�С�
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
//��ģ���еĺ�������
//
unsigned int fnWriteProcVM(struct PCB *pcb, unsigned int pstart,
						   char *pbuf, unsigned int ulen);
void switch_to(struct PCB *to);
void Scheduler();

//���ڽ��������ȫ�ֱ���,
//
//
struct PCB *pAllProcess;       //����ϵͳ�е����н���
struct PCB *pIdle;	           //����0-idleָ��
struct PCB *pCurrent;          //��ǰ�������еĽ���
unsigned int uTotalProc = 0;   //��ǰϵͳ�н��̵�����
unsigned int glbss0, glbesp0;  //ͨ��ȫ�ֱ���������ʱ����ss, sep


/*========================================================*
 *F: ��ʼ������idle process 0
 *I: void
 *O: void
 *C: ��StartMain()�е���
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
	p->priority = 0;  //���ȼ���
	p->uticks = p->kticks = 0;
	uTotalProc = 1;
	pIdle = p;
	pAllProcess = 0;
	pCurrent = p;
	//��ӵ�pAllProcess����
	p->lpnext = pAllProcess;
	pAllProcess = p;
	//Ϊ�ں˷���һ��ҳ��
	fnWriteProcVM(p, 0x80000000 - 64, "kernel", 6);
	//��0x00401000д����� jmp $
	fnWriteProcVM(p, 0x00401000, (char*)&jmpcode, 2);
}


/*========================================================*
 *F: ����һ���µĽ���
 *I: ueip-��ʼ����ָ��, prior-��������Ȩ
 *O: 0-����ʧ��, !0-����id
 *C: ע���һ������idle��id��0
 *========================================================*/
unsigned int fnCreateProcess(unsigned int ueip, short prior)
{	
	struct RegistersGroup regs;
	struct PCB * p;
	unsigned short opcodes[2];

	opcodes[0] = 0x80cd;  //int 80h
	opcodes[1] = 0xFEEB;  //jmp $	
	//��ʼ�����̵��ں˶�ջ,��ű��жϵ��û�ģʽcontext
	regs.cs = Select_Local_Code32;
	regs.ds = Select_Local_Data32;
	regs.es = Select_Local_Data32;
	regs.fs = Select_Local_Data32;
	regs.gs = Select_Local_Data32;
	regs.ss = Select_Local_Stack32; //�û�ģʽ��ss3
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
	//������idle��ҳĿ¼��0x80000000 --0xffffffff���Ƹ�p->cr3
	fnVmkCpy((char *)((unsigned int*)p->cr3 + (0x80000000>>22)),
		(char *)((unsigned int*)COMPLIER_BASEADDR + (0x80000000>>22)), 
		512 * sizeof(unsigned int));

	p->cr3 = p->cr3 - COMPLIER_BASEADDR;  //���������ַ
	p->ss0 = Select_Kernel_Stack32;
	p->esp0 = 0x80000000 - sizeof(struct RegistersGroup);
	p->pid = uTotalProc++;
	strcpy(p->pname, "taskA");
	p->counter = 10;
	p->priority = prior;  //���ȼ���
	p->uticks = p->kticks = 0;
	p->status = PROC_READY;
	//��ӵ�pAllProcess����
	p->lpnext = pAllProcess;
	pAllProcess = p;
	//Ϊ�ں˶�ջ����һ��ҳ��
	fnWriteProcVM(p, p->esp0, (char*)&regs, sizeof(struct RegistersGroup));
	//��0x00401000д����� jmp $
	fnWriteProcVM(p, 0x00401000, (char*)opcodes, 4);
    CreateTTY(p);
	return p->pid;
}


/*========================================================*
 *F: ��ǰ�����������벻���ж�˯��״̬,�ȴ�����
 *I: pwait-��Ҫ˯�ߵĽ��̹�������
 *O: void
 *========================================================*/
void fnSleepOn(struct PCB * *pwait)
{
	struct PCB *plast;

	if(!pwait)
		return;
	plast = *pwait;	 //��ǰһ���ȴ����̴����current�Ķ�ջ��
	*pwait = pCurrent;
	pCurrent->status = PROC_SLEEP;
	Scheduler();
	//�����󽫻�����һ���ȴ�����
	if(plast)
		plast->status = PROC_READY;
}


/*========================================================*
 *F: ����ĳ��˯�߶����ϵĽ���
 *I: pwait - ˯�߽��̶�������ָ��
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
 *F: ��һ�����̵�����ռ����д����
 *I: pcb-Ŀ����̿��ƿ�
 *   pstart-��ʼ��ַ
 *   pbuf-��д���ݻ�������ַ
 *   ulen-���ݳ���(��λ:�ֽ�)
 *O: 0-ʧ��, !0-�ɹ�
 *C: û�ж�д�Ŀռ䷶Χ����
 *========================================================*/
int fnWriteProcVM(struct PCB *pcb, unsigned int pstart,
				  char *pbuf, unsigned int ulen)
{
	unsigned int *pagetab, *page, lineaddr; 
	unsigned int *pagedir; //���̵�ҳĿ¼��ַ

	if(!ulen || !pcb || (pcb->pid && !pcb->cr3))
		return 0;
	//ͨ��ҳĿ¼���ҳ��Ե�ַ����ת��: vaddr--->paddr--->kaddr
	pagedir = (unsigned int *)(pcb->cr3 + COMPLIER_BASEADDR);  //ת�����ں����Ե�ַ
_nextpage:
	lineaddr = pstart & 0xfffff000;  //��ý�дҳ��ĵ�ַ
	//ҳĿ¼�����,����һ��ҳ��
	if((pagedir[lineaddr >> 22] & 0x00000001) == 0)
	{
		if(!(pagetab = (unsigned int *)fnGet_VmkPage()))
		{
			printk(RED, "PageFrame exhaust!");
			return  0;
		}
		//�޸Ķ�Ӧ��ҳĿ¼��
		pagetab = (unsigned int *)
			((unsigned int)pagetab - COMPLIER_BASEADDR);  //ת���������ַ
		pagedir[lineaddr >> 22] = (unsigned int )pagetab + 7 ;	
	}
	//��ȡ�����Ե�ַ��ҳ��������ַ
	pagetab = (unsigned int *)(pagedir[lineaddr >> 22] & 0xfffff000);
	pagetab = (unsigned int *)
		((unsigned int)pagetab + COMPLIER_BASEADDR); //ת�����ں����Ե�ַ

	//��ʼ����ҳ��
	//ҳ�������(ȱҳ)
	if((pagetab[(lineaddr >> 12) & 0x03FF] & 0x00000001) == 0)
	{
		if(!(page=(unsigned int *)fnGet_VmkPage()))
		{
			printk( RED ,"PageFrame exhaust!");
			return 0;
		}
		//��ʱpage���ں����Ե�ַ
		pagetab[(lineaddr>>12) & 0x03FF] = ((unsigned int)page - COMPLIER_BASEADDR)
				+ ((pstart >= (0x80000000 - 4*1024) || pstart <= 4*1024*1024) ? 3:7);
	}
	else
	{
		page = (unsigned int *)(pagetab[(lineaddr>>12) & 0x03FF] & 0xfffff000); 
		page = (unsigned int *)
			((unsigned int)page + COMPLIER_BASEADDR); //ת�����ں����Ե�ַ
	}
	//��ʼд�������ڴ�page
	do {
		*((char*)page + (pstart & 0xfff)) = *pbuf++;
		pstart++;
		ulen--;
	}while((pstart & 0xfff) && ulen);
	//�ж��Ƿ�д��,����д��һҳ
	if(ulen)
	{
		goto _nextpage;
	}
	return 1;
}


/*========================================================*
 *F: ���̵��ȳ���
 *I: void
 *O: void
 *C: �ο�linux0.11
 *========================================================*/
void 
Scheduler()
{
	struct PCB *tmp, *cand;
	int max;
	
	while(1)
	{
		max = -1;
		cand = pIdle;	//�Ƚ���ѡ��������Ϊidle
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
		//ִ�е�����˵�����о������̵�ʱ��Ƭ���Ѿ�����, ����
		//����ÿ�����̵�counter, Ȼ�����±Ƚ�ѡ��candidater
		for(tmp = pAllProcess; tmp ;tmp = tmp->lpnext)
		{
			tmp->counter = (short)((tmp->counter >> 1) + tmp->priority);
		}
	}//end while
	switch_to(cand);
}


/*========================================================*
 *F: ��CPU��ʹ��Ȩ�ӵ�ǰ�����л���Ŀ�����
 *I: to-Ŀ����̵Ŀ��ƿ�ָ��
 *O: void
 *C: ע�����л�ʱ,������Ȩ��ת�Ƶ����,�磺һ���½����Ľ���
 *   Ͷ������
 *========================================================*/
void switch_to(struct PCB *to)
{
	unsigned int dir;

	if(pCurrent == to)
		return;
	//������ǰ���н���pCurrent���ں˼Ĵ�����������(kernel context)
	__asm{
		pushfd
		push cs
		push OFFSET _restart  //�����ٴα��л�����ʱ, �ʹ�_restart����������
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
	//���浱ǰ���̵�ss0��esp0
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
	//��ҳĿ¼�Ĵ���cr3���ó�Ŀ����̵�ҳĿ¼��ַ
	pCurrent = to;
	dir = to->cr3;
	__asm{
		mov eax, dir
		mov cr3, eax  //ҳ���Ѿ�����ˢ��,��ջ�л���Ŀ����̵��ں˶�ջ(kernel stack)
		mov eax, glbss0
		mov ss, ax
		mov eax, glbesp0
		mov esp, eax
		//�ָ�Ŀ����̵ļĴ���������
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