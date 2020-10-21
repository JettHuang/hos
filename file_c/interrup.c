/*------------------------------------------------------*
 *˵����Ϊunsigned int.asm �е��ж�,�����жϴ�������
 *                         2007��3��28�� ,18:18:42
 *                         2007-6-17  19:04
 *------------------------------------------------------*/
#include "..\include\interrup.h"
#include "..\include\color.h"

//������������
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

//�ⲿ��������
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
extern void clock();//ʱ���ж�
extern void keyboard();//�����ж�
extern void system_calls();
extern void * Label_Idtr;
extern void printk(unsigned int color, const char * lpstr);
extern void panic(char *szerror);
extern void Handle_No_Page(unsigned int error_code, unsigned int line_addr);
extern void Handle_Protect_Page(unsigned int error_code, unsigned int line_addr);

//��������eip
//
unsigned int glbErrEip;


/*===================================================*
 *F: ������������ж�
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_DivideError(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "divide_error\n");
}


/*===================================================*
 *F: �ϵ��жϴ���
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Int3( unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Int3\n");
}


/*===================================================*
 *F: �������ж�
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Nmi(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED , "Nmi\n");
}


/*===================================================*
 *F: ����ж�
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Overflow(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Overflow\n");
}


/*===================================================*
 *F: �߽�������ж�
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Bounds(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Bounds\n");
}


/*===================================================*
 *F: ��Ч����ָ���ж�
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Invalid_Op(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Invalid_Op\n");
}


/*===================================================*
 *F: Э�������γ����ж�
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Coprocessor_SegmentOver(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Coprocessor_SegmentOver\n");
}


/*===================================================*
 *F: intel ����
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Reserved(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Reserved\n");
}


/*===================================================*
 *F: ˫���ϳ���
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Double_fault(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Double_fault\n");
}


/*===================================================*
 *F: CPU�л�ʱ����TSS��Ч
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Invalid_Tss(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Invalid_Tss\n");
}


/*===================================================*
 *F: ��������ָ�Ķβ�����
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Segment_absence(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Segment_absence\n");
}


/*===================================================*
 *F: ��ջ�β����ڻ�ѰַԽ����ջ��
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_Stack_Error(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	printk(RED, "Stack_Error\n");
}


/*===================================================*
 *F: ����û�з���80386����ģʽ����(��Ȩ��)�Ĳ���������ж�
 *I: error_code-������
 *O: void
 *===================================================*/
void fndo_General_Protection(unsigned int error_code)
{
	error_code = 0; //������ֻ��Ϊ����������
	panic("panic: General_Protection\n");
}


/*===================================================*
 *F: ����ҳ������ж�
 *I: error_code-������
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
 *F: ��ʼ���ж���������
 *I: void
 *O: void
 *C: ��StartMain()�е���, �������ж��������������
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
	
	//17~255����Ϊreserver
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