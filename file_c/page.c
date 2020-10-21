/*-----------------------------------------------------------------*
 *˵�������������ڴ�ҳ���������ڴ�ҳ��֮���Ӱ���ϵ��������
 *	  1,���̵�ҳ(Ŀ¼)�����;
 *	  2,���������ڴ�ȱҳ�ж�;
 *	  3,�����ڴ�ҳ����ʴ���;
 *
 *��ע��ע�������ַ�����Ե�ַ��ʹ��!
 *
 *�޸�ʱ�䣺04/11/07  13:45
 *-----------------------------------------------------------------*/
#include  "..\include\process.h"

extern void fnFree_VmkPage(void * addrpage);
extern void * fnGet_VmkPage();
extern void fnSleepOn(struct PCB * *pwait);
extern void panic(char *szerror);
extern void vsprintf(char *buf, char *fmt, ...);
extern struct PCB *pCurrent;

#define	COMPLIER_BASEADDR   0XC0000000
//ȫ�ֱ���,�ȴ����õ������ڴ�ҳ��
//
struct PCB *glbWaitMMPage = 0;


/*=============================================================*
 *F: �ͷ�ҳĿ¼�ͬʱ�ͷŶ�Ӧ��ҳ�������ҳ�档
 *I: addrDir-ҳĿ¼��ַ(�ں��е�����),
 *   indexDirEntry-ҳҳ����Ŀ¼�е�����,
 *   numEntries-ҳ�����
 *O: void 
 *C: ��Ҫ���ڡ����̡��˳�ʱ�����ڴ档ע��numEntries��ֵ!
 *=============================================================*/
void fnFree_PageDirEntries(void * addrDir, unsigned int indexDirEntry,
						   unsigned int numEntries)
{
	unsigned int i, j;
	
	if(!indexDirEntry || indexDirEntry >= 512)
		return;	 //0~4Mb���ڱ�������ռ�, 0x80000000~0xffffffff�����ں˿ռ�,���Բ����ͷ�
	if(indexDirEntry + numEntries > 512)
		numEntries = 512 - indexDirEntry;
	//���ݵ�ǰ����̵�ҳĿ¼��
	for(i = 0; i < numEntries; i++)
	{
		//����Ŀ¼���Ӧ��ҳ���Ƿ����
		if(((unsigned int *)addrDir)[indexDirEntry] & 0x00000001)
		{
			unsigned int * addr_pagetable; //ת�����ں����Ե�ַ
			addr_pagetable = (unsigned int *) 
				(COMPLIER_BASEADDR + (((unsigned int *)addrDir)[indexDirEntry] & 0xfffff000 ));//ȡҳ���ַ
			//���ҳ������ͷŴ��ڵ�pageFrame
			for(j = 0; j < 1024; j++)
			{
				if(addr_pagetable[j] & 0x00000001)
				{
					fnFree_VmkPage((void *)
						(COMPLIER_BASEADDR + (addr_pagetable[j] & 0xfffff000)));
					addr_pagetable[j] = 0; //ҳ��������
				}
			}
			//�ͷ�ҳ��ռ�õ�4Kb
			fnFree_VmkPage((void *)addr_pagetable);
			((unsigned int *)addrDir)[indexDirEntry] = 0;//ҳĿ¼������
		}
		indexDirEntry++;
	}	
}


/*=============================================================*
 *F: ȱҳ�쳣������
 *I: error_code-������, line_addr-����ȱҳ��ַ
 *O: void 
 *C: ͨ�����error code�������жϵ����Ե�ַ������������
 *   û�г��������������̹���ĳЩ����ҳ��,û��ʹ������洢����.
 *=============================================================*/
void Handle_No_Page(unsigned int error_code, unsigned int line_addr)
{
	unsigned int *page_addr, *temp_addr;
	unsigned int *Current_PageDir;  //��ǰ����̵�ҳĿ¼��ַ

_repeat:
	__asm{
		mov  eax, cr3
		mov  Current_PageDir, eax
	}
	//ת�����ں����Ե�ַ
	Current_PageDir = (unsigned int *)((unsigned int)Current_PageDir + COMPLIER_BASEADDR); 
	line_addr &= 0xfffff000;  //��÷���ȱҳ��ҳ���ַ
	//�ж�ҳĿ¼�����
	if((Current_PageDir[line_addr >> 22] & 0x00000001) == 0)
	{
		if(!(page_addr = (unsigned int *)fnGet_VmkPage()))
		{
			//û�з��䵽�����ڴ�ҳ, ����ǰ����
			fnSleepOn(&glbWaitMMPage);
			goto _repeat;
		}
		//�޸Ķ�Ӧ��ҳĿ¼��, �����ں����Ե�ַת���������ַ
		page_addr = (unsigned int *)((unsigned int)page_addr - COMPLIER_BASEADDR);
		Current_PageDir[line_addr >> 22] = (unsigned int )page_addr + 7;																
	}
	//��ȡ�����Ե�ַ��ҳ��������ַ
	page_addr = (unsigned int *)(Current_PageDir[line_addr >> 22] & 0xfffff000);
	//ת�����ں����Ե�ַ
	page_addr = (unsigned int *)((unsigned int)page_addr + COMPLIER_BASEADDR); 
	if(!(temp_addr = (unsigned int *)fnGet_VmkPage()))
	{
		//û�з��䵽�����ڴ�ҳ, ����ǰ����
		fnSleepOn(&glbWaitMMPage);
		goto _repeat;
	}
	//ת���������ַ
	temp_addr = (unsigned int *)((unsigned int)temp_addr - COMPLIER_BASEADDR );
	page_addr[(line_addr >> 12) & 0x03FF] = (unsigned int)temp_addr 
		+ ((line_addr >= (0x80000000 - 4*1024 ) || line_addr <= 4*1024*1024) ? 3 : 7);
}


/*===========================================================*
 *F: ҳ�汣���쳣������
 *I: 
 *O: void 
 *C: Ŀǰֻ��±ç�Ĵ���
 *===========================================================*/
void Handle_Protect_Page(unsigned int error_code, unsigned int line_addr)
{
	char szErr[128];

	vsprintf(szErr, "panic:Page Protect! line_addr= 0x%x, pid= 0x%x\n",
		line_addr, pCurrent->pid); 
	panic(szErr);
}
