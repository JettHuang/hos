/*****************************************
*	����Ķ���
*	2007-4-3
*	huangheshui
*****************************************/


/*������ҳ����*/
#define		TurnOn_Paging() \
__asm	mov  eax ,cr0 \
__asm	or   eax ,80000000h	\
__asm	mov  cr0 ,eax
/*end macro */


/*�رշ�ҳ����*/
#define		TurnOff_Paging() \
__asm	mov  eax ,cr0 \
__asm	and  eax ,7fffffffh	\
__asm	mov  cr0 ,eax
/*end macro */

/*�����ж�*/
#define		Sti()	__asm  sti

/*�ر��ж�*/
#define		Cli()	__asm  cli

/*��I/O�˿ڷ����ֽ�*/
#define	 Out_Byte( port ,value) \
__asm	mov  al ,value \
__asm   mov  dx ,port \
__asm   out  dx ,al \
__asm   nop \
__asm   nop
/*end macro */

/*��I/O�˿ڶ�ȡ�ֽ�*/
#define	 In_Byte( port ,retval) \
__asm   mov  dx ,port \
__asm   in   al ,dx \
__asm	mov  retval ,al 
/*end macro */


/* ���� EOI,Ӳ���ж���Ҫ���� */
#define	Send_EOI() \
__asm	mov al, 20h \
__asm	out 20h, al	
/*end macro */

/*�ж����� ��8259A*/
#define Mask_8259AA(MASK_CODE) \
__asm  mov al,MASK_CODE \
__asm  out 021h ,al \
__asm  nop \
__asm  nop \
__asm  nop
/*end macro */

/*�ж����� ��8259A*/
#define Mask_8259AB(MASK_CODE) \
__asm  mov al,MASK_CODE \
__asm  out 0A1h ,al \
__asm  nop \
__asm  nop \
__asm  nop
/*end macro */

/*�ղ���*/
#define NOP() \
__asm  nop
/*end macro */







