/*------------------------------------------------------*
 *说明：为unsigned int.asm 中的中断,定义中断处理函数。
 *                         2007年3月28日 ,18:18:42
 *                         2007-6-17  19:04
 *------------------------------------------------------*/
#include "..\include\interrup.h"
#include "..\include\color.h"

//函数声明如下
//
void fndo_DivideError(unsigned int error_code);
void fndo_Int3(unsigned int error_code);
void fndo_Nmi(unsigned int error_code);
void fndo_Overflow(unsigned int error_code);
void fndo_Bounds(unsigned int error_code);
void fndo_Invalid_Op(unsigned int error_code);
void fndo_Coprocessor_SegmentOver(unsigned int error_code);
void fndo_Reserved(unsigned int error_code);
void fndo_Double_fault(unsigned int error_code);
void fndo_Invalid_Tss(unsigned int error_code);
void fndo_Segment_absence(unsigned int error_code);
void fndo_Stack_Error(unsigned int error_code);
void fndo_General_Protection(unsigned int error_code);

//外部引进函数
//
extern void divide_error();
extern void debug();
extern void nmi();
extern void int3();	
extern void overflow();
extern void bounds();
extern void invalid_op();
extern void double_fault();
extern void coprocessor_segment_overrun();
extern void reserved();		
extern void invalid_TSS();
extern void segment_not_present();
extern void stack_segment_error();
extern void general_protection();
extern void page_fault() ;
extern void clock();//时钟中断
extern void keyboard();//键盘中断
extern void system_calls();
extern void * Label_Idtr;
extern void printk(unsigned int color, const char * lpstr);
extern void panic(char *szerror);
extern void Handle_No_Page(unsigned int error_code, unsigned int line_addr);
extern void Handle_Protect_Page(unsigned int error_code, unsigned int line_addr);

//引起错误的eip
//
unsigned int glbErrEip;


/*===================================================*
 *F: 处理除法错误中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_DivideError(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "divide_error\n");
}


/*===================================================*
 *F: 断点中断处理
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Int3( unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Int3\n");
}


/*===================================================*
 *F: 非屏蔽中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Nmi(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED , "Nmi\n");
}


/*===================================================*
 *F: 溢出中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Overflow(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Overflow\n");
}


/*===================================================*
 *F: 边界检查出错中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Bounds(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Bounds\n");
}


/*===================================================*
 *F: 无效操作指令中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Invalid_Op(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Invalid_Op\n");
}


/*===================================================*
 *F: 协处理器段超出中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Coprocessor_SegmentOver(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Coprocessor_SegmentOver\n");
}


/*===================================================*
 *F: intel 保留
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Reserved(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Reserved\n");
}


/*===================================================*
 *F: 双故障出错
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Double_fault(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Double_fault\n");
}


/*===================================================*
 *F: CPU切换时发觉TSS无效
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Invalid_Tss(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Invalid_Tss\n");
}


/*===================================================*
 *F: 描述符所指的段不存在
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Segment_absence(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Segment_absence\n");
}


/*===================================================*
 *F: 堆栈段不存在或寻址越出堆栈段
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Stack_Error(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	printk(RED, "Stack_Error\n");
}


/*===================================================*
 *F: 处理没有符合80386保护模式机制(特权级)的操作引起的中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_General_Protection(unsigned int error_code)
{
	error_code = 0; //这样做只是为了消除警告
	panic("panic: General_Protection\n");
}


/*===================================================*
 *F: 处理页面错误中断
 *I: error_code-出错码
 *O: void
 *===================================================*/
void fndo_Pagefault(unsigned int error_code)
{
	unsigned int uCR2;
	char szErr[128];

	__asm{
		mov eax, cr2
		mov uCR2, eax
	}
	if(!(error_code & 0x00000001))	
	{
		Handle_No_Page(error_code, uCR2);
		vsprintf(szErr, "panic: no page! eip= 0x%x, uCR2= 0x%x\n", glbErrEip, uCR2); 
		panic(szErr);
	}
	else
		Handle_Protect_Page(error_code, uCR2);
}


/*===================================================*
 *F: 初始化中断描述符表
 *I: void
 *O: void
 *C: 在StartMain()中调用, 其它的中断项将有所需这设置
 *===================================================*/
void fnInit_Idt()
{
	unsigned int i;
	set_trap_gate(0, &divide_error);
	set_trap_gate(1, &debug);
	set_trap_gate(2, &nmi);
	set_system_gate(3, &int3);
	set_system_gate(4, &overflow);
	set_system_gate(5, &bounds);
	set_trap_gate(6, &invalid_op);
	set_trap_gate(8, &double_fault);
	set_trap_gate(9, &coprocessor_segment_overrun);
	set_trap_gate(10, &invalid_TSS);
	set_trap_gate(11, &segment_not_present);
	set_trap_gate(12, &stack_segment_error);
	set_trap_gate(13, &general_protection);
	set_trap_gate(14, &page_fault);
	set_trap_gate(15, &reserved);
	
	//17~255设置为reserver
	for(i = 17; i <= 255; i++)
	{
		set_trap_gate(i, &reserved);
	}
	set_trap_gate(32, &clock);
	set_trap_gate(33, &keyboard);
	set_system_gate(0x80, &system_calls);
	__asm{
		lidt fword ptr[Label_Idtr]
	}
	printk(GREEN, "Init idt over!\n");
}