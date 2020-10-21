/*****************************************
*	宏汇编的定义
*	2007-4-3
*	huangheshui
*****************************************/


/*开启分页机制*/
#define		TurnOn_Paging() \
__asm	mov  eax ,cr0 \
__asm	or   eax ,80000000h	\
__asm	mov  cr0 ,eax
/*end macro */


/*关闭分页机制*/
#define		TurnOff_Paging() \
__asm	mov  eax ,cr0 \
__asm	and  eax ,7fffffffh	\
__asm	mov  cr0 ,eax
/*end macro */

/*开启中断*/
#define		Sti()	__asm  sti

/*关闭中断*/
#define		Cli()	__asm  cli

/*向I/O端口发送字节*/
#define	 Out_Byte( port ,value) \
__asm	mov  al ,value \
__asm   mov  dx ,port \
__asm   out  dx ,al \
__asm   nop \
__asm   nop
/*end macro */

/*从I/O端口读取字节*/
#define	 In_Byte( port ,retval) \
__asm   mov  dx ,port \
__asm   in   al ,dx \
__asm	mov  retval ,al 
/*end macro */


/* 发送 EOI,硬件中断需要发送 */
#define	Send_EOI() \
__asm	mov al, 20h \
__asm	out 20h, al	
/*end macro */

/*中断屏蔽 主8259A*/
#define Mask_8259AA(MASK_CODE) \
__asm  mov al,MASK_CODE \
__asm  out 021h ,al \
__asm  nop \
__asm  nop \
__asm  nop
/*end macro */

/*中断屏蔽 从8259A*/
#define Mask_8259AB(MASK_CODE) \
__asm  mov al,MASK_CODE \
__asm  out 0A1h ,al \
__asm  nop \
__asm  nop \
__asm  nop
/*end macro */

/*空操作*/
#define NOP() \
__asm  nop
/*end macro */







