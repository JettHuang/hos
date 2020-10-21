;现在我们进入32位保护模式下的编程了， 该文件是通过win32汇编程序编译的。
;在 head.asm 模块中，我们做了如下工作：
;	1，重新设置gdt 和 idt ；
;	2，检查A20是否打开，否则死机；
;	3，检查X87数学协处理器；
;
;备注：编译base = 0xC0000000

;最后修改时间： 2007年4月13日


		.386p
		.model	flat ,c
		option	casemap:none	
		
include pmmasm32.inc
include color.inc

extrn  Start_Main:proto
extrn  uglbTotal_PmPages:dword

public  printk
public  label_des_idt
public  Label_Idtr
public  dwTSS_esp0


COMPLIER_BASEADDR	EQU	0C0000000h
VIDEO_BUFFER		EQU	COMPLIER_BASEADDR  + 0B8000h
.data
;备注:对于硬件中断和内部异常,CPU忽略对DPL的检查,  但是对int n 引起的异常则要进行检查.
;可以参考intel cpu reference
;▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
;中断描述符表
label_des_idt	byte   256 * 8  dup(0)			
Label_Idtr	dw	$ - label_des_idt - 1   ;界限,长度要-1
		dd	OFFSET label_des_idt	;线性基地址

ALIGN 4 
;全局描述符表▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓	
Gdt_desc_Null	  Descriptor  <0 ,0 ,0 ,0 ,0>
Gdt_desc_Code32   Descriptor  <0ffffh ,0 ,0 , DA_C  + DA_32 + DA_LIMIT_4K  + 0F00h >      
Gdt_desc_Data32   Descriptor  <0ffffh ,0 ,0 , DA_DRW + DA_LIMIT_4K  + 0F00h >
Gdt_Desc_Stack32  Descriptor  <0ffffh ,0 ,0 , DA_DRW + DA_LIMIT_4K + DA_32  + 0F00h >;对于ss来说 DA_32 这很重要
Gdt_Desc_LDT	  Descriptor  <0 ,0 ,0 ,0 ,0> ;需要初始化,DPL = 3
Gdt_Desc_TSS	  Descriptor  <0 ,0 ,0 ,0 ,0> ;需要初始化,DPL = 0
;获取段选择子，即索引
SelectCode32	equ  Gdt_desc_Code32 - Gdt_desc_Null    ;8
SelectData32	equ  Gdt_desc_Data32  - Gdt_desc_Null   ;16
SelectStack32	equ  Gdt_Desc_Stack32  - Gdt_desc_Null  ;24
SelectLDT	equ  Gdt_Desc_LDT  - Gdt_desc_Null      ;DPL = 0\3 
SelectTss	equ  Gdt_Desc_TSS - Gdt_desc_Null
;描述符表结束
GdtLen        equ  $ - Gdt_desc_Null     ;GDT的长度
Label_Gdtr    dw   GdtLen - 1            ;GDT段界
	      dd   OFFSET  Gdt_desc_Null ;GDT基地址

ALIGN 4 
;局部段描述符表 ,DPL = ring3 ▓▓▓▓▓▓▓▓▓▓▓
Label_Local_Code32	Descriptor  <0ffffh ,0 ,0 , DA_C  + DA_32 + DA_LIMIT_4K + DA_DPL3 + 0F00h >
Label_Local_Data32	Descriptor  <0ffffh ,0 ,0 , DA_DRW + DA_LIMIT_4K  + DA_DPL3 + 0F00h >
Label_Local_Stack32	Descriptor  <0ffffh ,0 ,0 , DA_DRW + DA_LIMIT_4K + DA_32 + DA_DPL3 + 0F00h >
Lacal_TableLen	 equ   $ - Label_Local_Code32
;局部段的描述符选择子 RPL = 3
Select_Local_Code32	equ	Label_Local_Code32 - Label_Local_Code32  +  SA_TIL + SA_RPL3
Select_Local_Data32	equ	Label_Local_Data32 - Label_Local_Code32  +  SA_TIL + SA_RPL3
Select_Local_Stack32	equ	Label_Local_Stack32 - Label_Local_Code32 +  SA_TIL + SA_RPL3

;▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
;TSS 段，作用是提供(发生特权级转换时)“中断”切换所需要的 ss ,esp 。全靠它了!
Label_TSS_Segment  \
		DD	0			; Back
dwTSS_esp0	DD	80000000h		; 0 级堆栈 esp0 ,某个进程的“上下文stack's  esp”
_3		DD	SelectStack32		;          ss0
_4		DD	0			; 1 级堆栈
_5		DD	0			; 
_6		DD	0			; 2 级堆栈
_7		DD	0			; 
_8		DD	0			; CR3
_9		DD	0			; EIP
_10		DD	0			; EFLAGS
_11		DD	0			; EAX
_12		DD	0			; ECX
_13		DD	0			; EDX
_14		DD	0			; EBX
_15		DD	0			; ESP
_16		DD	0			; EBP
_17		DD	0			; ESI
_18		DD	0			; EDI
_19		DD	0			; ES
_20		DD	0			; CS
_21		DD	0			; SS
_22		DD	0			; DS
_23		DD	0			; FS
_24		DD	0			; GS
_25		DD	0			; LDT
_26		DW	0			; 调试陷阱标志
_27		DW	$ - Label_TSS_Segment + 2	; I/O位图基址
_io		DB	0ffh			; I/O位图结束标志

TssLen		equ	$ - Label_TSS_Segment


;▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
IndexShow     dword  0
szPageUp      byte   "page setup!",0ah ,0
szIgnore      byte   "Ignore Int take place ",0ah ,0
szDebug	      byte   "Debug...",0ah ,0
szPrompt      byte   0ah ,0ah,0ah ,"Start Up 32",0ah ,0
dwSys_stack   dword   512   DUP(0)	;2Kb ,系统堆栈，内核在初始化的时候使用。
SYS_STACKTOP	equ	$


.code
;程序入口在这儿 ,同时以后page_table 会将它覆盖 ,page_dir 放在0x00000000
main:
Startup_32:
	mov  ax ,SelectData32	  ;setup中的 数据段选择符
	mov  ds ,ax
	mov  es ,ax
	mov  fs ,ax
	mov  gs ,ax
	mov  ax ,SelectStack32
	mov  ss ,ax
	mov  esp ,SYS_STACKTOP   
	
	push OFFSET szPrompt
	push yellow		
	call printk
	add  esp ,8

;通过向0X100000处 循环写入写 i++ ,如果 [0X00000] == [0X100000] ，那么继续，直到
; 不相等，否则说明A20没有打开，造成死机!
	xor eax ,eax
	mov edi ,100000h + COMPLIER_BASEADDR
goon_cmp:
	inc eax
	mov dword ptr[edi],eax
	cmp eax ,dword ptr[COMPLIER_BASEADDR]
	je  goon_cmp

;===============================================================================
	call Setup_idt		  ;设置中断描述符表
	call Init_pagetable	  ;设置内核页表
	xor  eax ,eax	;页目录BaseAddr == 0x00000000,物理地址噢
	mov  cr3 ,eax	;刷新PTB
	;重新加载gdtr ,下面四条指令非常微妙，也非常重要！全靠它们了，希望顺利通过。
	lgdt fword ptr[Label_Gdtr]  ;gdtr has no buffer 
	mov  eax ,cr0
	or   eax ,80000000h		;PG = 1
	mov  cr0 ,eax		;H: 分页机制立即生效，并清空指令缓冲队列,是分页机制自动清除的
;描述：40000000h  + eip ---> 产生地址回绕(结果等于指令所在的物理地址)  ---(页表)---> 物理地址(正是下一条指令)
	;jmp  08h:after		
op_jmp    byte   0EAh		;更新段选择缓冲区的刷新
off_addr  dword  after
seg_sel   word   08h

after:
	mov  ax ,SelectData32		;reload all segment registers ,fresh their buffer
	mov  ds ,ax
	mov  es ,ax
	mov  fs ,ax
	mov  gs ,ax
	mov  ax ,SelectStack32
	mov  ss ,ax
	mov  esp ,SYS_STACKTOP   
	;将CR3对应的目录第0项清空
	xor  eax ,eax
	mov  edi ,COMPLIER_BASEADDR
	stosd
;接下来准备转向 Main()
	;设置LDT 描述符
	mov  eax ,OFFSET Label_Local_Code32 
	mov  word ptr[Gdt_Desc_LDT] , Lacal_TableLen - 1
	mov  word ptr[Gdt_Desc_LDT + 2] ,ax
	shr  eax ,16
	mov  byte ptr[Gdt_Desc_LDT + 4] ,al
	mov  byte ptr[Gdt_Desc_LDT + 7] ,ah
	mov  word ptr[Gdt_Desc_LDT + 5] , DA_LDT	;DPL = 0  与  SelectLDT 中的 RPL 可能不作检测。
	;Tss 描述符 
	mov  eax ,OFFSET Label_TSS_Segment
	mov  word ptr[Gdt_Desc_TSS ] , TssLen - 1
	mov  word ptr[Gdt_Desc_TSS + 2] ,ax
	shr  eax ,16
	mov  byte ptr[Gdt_Desc_TSS + 4] ,al
	mov  byte ptr[Gdt_Desc_TSS + 7] ,ah
	mov  word ptr[Gdt_Desc_TSS + 5] ,DA_386TSS	;DPL = 0
	;load ldtr
	mov  ax ,SelectLDT
	lldt  ax 
	;load TSS
	mov  ax ,SelectTss
	ltr  ax
;下面部分将放在Start_Main()中			
	push OFFSET szDebug
	push 15		;第9行
	call printk
	add  esp ,8

	call Start_Main
	jmp $				;转向 Start_Main()
;▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
;▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓		****************				****************		 ▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓				      进入 "Start Main()" 吧					 ▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓				***********	     ***********				 ▓▓▓
;▓▓▓					****************					 ▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓												 ▓▓▓
;▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
;▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

;备注:第一次调试的时候这儿出现了错误.
ALIGN 2
;************************************
;初始化内核空间页表函数		    |
;我们将Page table 放置在物理内存0x100000开始处
;************************************
Init_pagetable  proc
	;获取物理内存的大小,0---X
	xor  eax ,eax
	mov  edi ,0c0090100h
	mov  ax ,word ptr[edi]	;1M---16M ,单位: KB
	shr  eax ,2
	xor  ecx ,ecx
	add  edi ,2
	mov  cx ,word ptr[edi]  ;16M--4G  ,单位:64KB
	shl  ecx ,4	;2exp4 == 16
	add  eax ,ecx
	add  eax ,256	;基本内存 1MB
	;保存eax ,物理内存页面个数
	cmp  eax ,64 * 1024	;256M = 64 * 1024 * 4KB
	jbe   below256
	mov  eax ,64 * 1024
below256:
	mov  uglbTotal_PmPages ,eax

	;对页目录表进行清零0xC0000000
	mov  ecx ,1024
	xor  eax ,eax
	mov  edi ,COMPLIER_BASEADDR 
	cld 
	rep  stosd	;将 eax --> es:edi
;注意：为了顺利过渡，在启用分页机制之后的过渡代码段，仍要维持线性地址等同于物理地址.
;     所以这儿将 index = 0也映射到了 page0 ; 有了这部分代码，我终于成功了!!!!!
	mov  eax ,100000h + 3
	mov  edi ,COMPLIER_BASEADDR
	stosd
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	;根据物理内存页面个数设置页目录表
	mov  eax ,uglbTotal_PmPages
	add  eax ,1023
	shr  eax ,10	; 2exp10 == 1024
	;从0xC0000000对应的目录index进行初始化
	mov ecx ,eax
	mov edi ,COMPLIER_BASEADDR + 300h * 4
	mov eax ,100000h + 3		;p =1 ,r/w = 1 , u/s = 0(系统级)
setdir:
	stosd				; eax --> es:edi
	add  eax ,1024 * 4		; 4KB 因为页表是连续放置的
	loop setdir

	;开始初始化0xC0000000 + 1M开始的页表
	mov  edi ,COMPLIER_BASEADDR + 100000h
	mov  ecx ,uglbTotal_PmPages 	
	mov  eax ,00000000h + 3		;p =1 ,r/w = 1 , u/s = 0(系统级)
setpages:
	stosd				; eax --> es:edi
	add  eax ,1024 * 4		;4KB
	loop setpages
	;剩余的一部分没有设置，应该不会有问题的.保证内核不会访问这儿
;至此所有的页表项设置完了
	ret
Init_pagetable  endp

ALIGN 2
;************************************
;加载中断描述符表函数		    |
;************************************
Setup_idt  proc  
	;修改idt ,设置ignore_int
	mov edx ,OFFSET Ignore_int
	mov eax ,080000h	;将代码段选择符 08h ,放在eax的高16位
	mov ax ,dx		;低16位放置 中断处理程序地址的低16位
	mov dx ,08E00h		; 386中断门 ,DPL = 0 , present
				; 此时edx -- eax 构成了中断描述符
	mov edi ,OFFSET label_des_idt
	mov ecx ,256
rp_sidt:
	mov [edi],eax
	mov [edi+4],edx
	add edi ,8
	loop rp_sidt
	lidt fword ptr[Label_Idtr]
	ret
Setup_idt  endp
;************************************
;临时的中断处理程序		    |
;************************************
ALIGN 2
Ignore_int proc	;中断处理程序
	push  OFFSET szIgnore
	push  red
	call  printk
	add   esp ,8
	; 发送 EOI,硬件中断需要发送
	mov al, 20h
	out 20h, al				
	iretd			 ;注意是iretd 而不是 iret ,切记! 2007年3月31日 ,13:48:09
Ignore_int endp

;打印一个字符串
;参数：row ,lpstr
printk  proc color:DWORD ,lpstr:DWORD		
	pushad
	mov  edi ,IndexShow	;指定显示地方
	mov  eax ,color
	mov  ah ,al		;ah = 颜色
	mov  esi ,lpstr
	dec  esi		; sizeof(byte)
shownext:
	;判断是否需要ScrollUpWindow
	cmp  edi ,80 * 25 * 2
	jb   neednot_scroll
	push esi
	push edi
	push eax
	mov  edi ,VIDEO_BUFFER
	mov  esi ,VIDEO_BUFFER  + 80 * 2
	mov  ecx ,80 * 24		; 80 列 ,需要向上移动24行
	rep  movsw
	;屏幕最后一行清空
	mov  ecx ,80
	mov  ax ,black * 16 + 20h
	rep  stosw
	pop  eax
	pop  edi
	pop  esi
	sub  edi ,80 * 2
neednot_scroll:
	inc  esi
	mov  al ,byte ptr[esi]
	cmp  al ,0
	je   enddis
	cmp  al ,0ah			;回车键吗
	jne  not_return
	push eax			;save  color 
	mov  eax ,edi
	xor  edx ,edx
	mov  ebx ,80 * 2 
	div  ebx
	sub  edi ,edx			;edx = edi mod ebx 
	add  edi, 80 * 2
	pop  eax
	jmp  shownext	
not_return:
	mov  [edi + VIDEO_BUFFER] ,ax
	add  edi ,2
	jmp shownext
enddis:
	
	mov  IndexShow ,edi
	popad
	ret
printk  endp
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;*******************************************************************************
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
END  main


	
