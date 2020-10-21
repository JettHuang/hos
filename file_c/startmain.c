/*---------------------------------------------------*
 * �����￪ʼ�����ں˳�ʼ�����������Ҵ�����һ������0: idle process
 *                                    huangheshui
 *                                    2007-4-3 ,22:10
 * �޸�:ɾ�����Խ׶εĴ���            2007-05-16, 20:57
 *---------------------------------------------------*/
#include  "..\include\process.h"
#include "..\include\color.h"

extern void fnInit_Mem();  //��ʼ�������ڴ�Ĺ�����Ϣ
extern void fnInit_Idt();  //��ʼ���ж���������
extern void fnFloppy_init();
extern void fnHd_init();
extern void	fnInit_Buffer();
extern void fnInitKeyboard();
extern unsigned int fnCreateProcess(unsigned int ueip, short prior);
extern void printk(unsigned long color, const char *lpstr);
extern void fnInit_IdleProcess();


/*================================================*
 *����ϵͳ�ĳ�ʼ������
 *================================================*/
void Start_Main()
{
	fnInit_Mem();  //��ʼ�������ڴ�Ĺ�����Ϣ
	fnInit_Idt();  //��ʼ���ж���������
	fnFloppy_init();
	fnHd_init();
	fnInit_Buffer();
	fnInitKeyboard();
	//��ʼ����һ������
	fnInit_IdleProcess();
 	printk(LIGHTMAGENTA, "Init Idle ok\n");
	fnCreateProcess(0x401000, 20);  //��������1
	printk(LIGHTMAGENTA, "fnCreateProcess() pid = 1\n");
	// ring0 --> ring3
	__asm{
		push Select_Local_Stack32
		push 40000000h
		pushfd
		pop eax
		or  eax ,200h
		push eax	//push eflags -- sti
		push Select_Local_Code32
		push 401000h
		iretd
		}
	for(;;)
		;
}
