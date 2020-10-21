/*---------------------------------------------------*
 * 从这里开始进行内核初始化工作。并且创建第一个进程0: idle process
 *                                    huangheshui
 *                                    2007-4-3 ,22:10
 * 修改:删除测试阶段的代码            2007-05-16, 20:57
 *---------------------------------------------------*/
#include  "..\include\process.h"
#include "..\include\color.h"

extern void fnInit_Mem();  //初始化物理内存的管理信息
extern void fnInit_Idt();  //初始化中断描述符表
extern void fnFloppy_init();
extern void fnHd_init();
extern void	fnInit_Buffer();
extern void fnInitKeyboard();
extern unsigned int fnCreateProcess(unsigned int ueip, short prior);
extern void printk(unsigned long color, const char *lpstr);
extern void fnInit_IdleProcess();


/*================================================*
 *进行系统的初始化工作
 *================================================*/
void Start_Main()
{
	fnInit_Mem();  //初始化物理内存的管理信息
	fnInit_Idt();  //初始化中断描述符表
	fnFloppy_init();
	fnHd_init();
	fnInit_Buffer();
	fnInitKeyboard();
	//初始化第一个进程
	fnInit_IdleProcess();
 	printk(LIGHTMAGENTA, "Init Idle ok\n");
	fnCreateProcess(0x401000, 20);  //创建进程1
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
