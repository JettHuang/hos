;说明： 该模块重新设置中断描述符。
;时间：2007年3月28日 ，13:09:35
;	黄河水
;----------
;中断时，堆栈切换到 内核堆栈中去;必须保证第一次中断一定是ring3 -----> ring0
;
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		.386p
		.model flat ,c
		option casemap:none


;声明一些外部定义的函数
extrn   fndo_DivideError:proto
extrn   fndo_Int3:proto
extrn   fndo_Nmi:proto
extrn   fndo_Overflow:proto
extrn   fndo_Bounds:proto
extrn   fndo_Invalid_Op:proto
extrn   fndo_Coprocessor_SegmentOver:proto
extrn   fndo_Reserved:proto
extrn   fndo_Double_fault:proto
extrn   fndo_Invalid_Tss:proto
extrn   fndo_Segment_absence:proto
extrn   fndo_Stack_Error:proto
extrn   fndo_General_Protection:proto
extrn   fndo_clock:proto
extrn   fndo_Pagefault:proto
extrn	fndo_keyboard:proto
extrn   pfnDo_floppy:dword
extrn   pfnDo_hd:dword
extrn   MountRootfs:proto
extrn   fnUnknown_interrupt:proto
extrn   glbErrEip:dword

;声明一些给外部引用的 “符号”
public  divide_error 
public  debug
public  nmi
public  int3
public  overflow
public  bounds
public  invalid_op
public  coprocessor_segment_overrun
public  reserved
public  double_fault
public  invalid_TSS
public  segment_not_present
public  stack_segment_error
public  general_protection
public  clock
public  page_fault
public  keyboard
public  floppy_interrupt
public  hd_interrupt
public  system_calls

SelectData32	equ	010h

.code
divide_error:
	push  fndo_DivideError	;将除法异常处理地址入栈

no_error_code:			;这些中断没有 errorcode
	xchg  eax ,[esp]	;此时[esp]是 处理函数的地址，将它-->eax
	push  ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs		;现场已经保护好了,eax == 处理函数地址
;@@@@@@@@@@@@@@@@@@
	push  eax
	mov   ax ,SelectData32	;内核数据段
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	pop   eax
	sti
	push  0			; error_code = 0 ,我们设定的
	call  eax		;调用中断处理函数
	add   esp ,4
	cli
;@@@@@@@@@@@@@@@@@@	
	pop   gs
	pop   fs
	pop   es
	pop   ds
	pop   ebp
	pop   esi
	pop   edi
	pop   edx
	pop   ecx
	pop   ebx
	pop   eax		;现场已经恢复了
	iretd			;注意：是iretd 而不是 iret 啊!!

debug:				; 调试 t 
	push  fndo_Int3
	jmp   no_error_code

nmi:
	push  fndo_Nmi		; 非屏蔽中断
	jmp   no_error_code
	
int3:
	push  fndo_Int3		; int 3 断点
	jmp   no_error_code

overflow:
	push  fndo_Overflow	;溢出出错处理
	jmp   no_error_code

bounds:
	push  fndo_Bounds	;边界检查出错中断
	jmp   no_error_code

invalid_op:			;非法操作指令
	push  fndo_Invalid_Op
	jmp   no_error_code

;设备不可用（没有写） int 7
coprocessor_segment_overrun:	;int 9 协处理器段超出
	push  fndo_Coprocessor_SegmentOver
	jmp   no_error_code

reserved:	;int 15 intel保留
	push  fndo_Reserved
	jmp   no_error_code


;--------------------------------------
;接下来处理具有error code 的中断或异常:  int 8  ,int 10 - 14 ;
;出错码将被压栈，在eip 之后 。
;error code 需要自己从栈上弹出
double_fault:
	push  fndo_Double_fault

error_code:
	xchg  eax ,[esp + 4]    ; ebx 中将保存 error code 
	xchg  ebx ,[esp]	;此时[esp]是 处理函数的地址，将它-->ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs		;现场已经保护好了,ebx == 处理函数地址
;@@@@@@@@@@@@@@@@@@
	mov   ecx, [esp + 44]
	mov   glbErrEip, ecx    ;保存引起错误的eip
	push  eax
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	pop   eax
	sti
	push  eax		; error_code = eax ,我们设定的
	call  ebx		;调用中断处理函数
	add   esp ,4
	cli
;@@@@@@@@@@@@@@@@@@
	pop   gs
	pop   fs
	pop   es
	pop   ds
	pop   ebp
	pop   esi
	pop   edi
	pop   edx
	pop   ecx
	pop   ebx
	pop   eax		;现场已经恢复了 
	iretd			;注意：是iretd 而不是 iret 啊!!

invalid_TSS:			;int 10 无效的任务状态段
	push  fndo_Invalid_Tss
	jmp   error_code

segment_not_present:		;int 11 段不存在
	push  fndo_Segment_absence
	jmp   error_code

stack_segment_error:			;int 12 堆栈错误, 不存在或 越界
	push  fndo_Stack_Error
	jmp   error_code

general_protection:		;int 13 没有符合80386保护模式机制（特权级）的操作引起
	push  fndo_General_Protection
	jmp   error_code

;int 14 页错误在mm 中定义
page_fault:
	push  fndo_Pagefault
	jmp   error_code
;一些硬件的中断在相应的文件中处理。
clock:;时钟中断
	push  eax 
	push  ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs				;现场已经保护好了
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	mov   eax ,  dword ptr[esp + 48]	;get cs that be interrupted
	and   eax ,03h
	push  eax				;push cpl
;发送EOI,必须提前因为fndo_clock()可能发生进程切换，将导致没有人send EIO!,注意我们没有开启中断sti
	mov al, 20h				
	out 20h, al
	call  fndo_clock			;调用时钟处理函数
	add   esp ,4
;@@@@@@@@@@@@@@@@@@
	pop   gs
	pop   fs
	pop   es
	pop   ds
	pop   ebp
	pop   esi
	pop   edi
	pop   edx
	pop   ecx
	pop   ebx
	pop   eax		;现场已经恢复了
	iretd			;注意：是iretd 而不是 iret 啊!!
;======================================================================
;键盘中断处理程序
keyboard:
	push  eax 
	push  ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs				;现场已经保护好了
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	sti	
	call  fndo_keyboard			;调用键盘处理函数
	cli
	mov al, 20h				;发送EOI
	out 20h, al
	nop
	nop
;@@@@@@@@@@@@@@@@@@
	pop   gs
	pop   fs
	pop   es
	pop   ds
	pop   ebp
	pop   esi
	pop   edi
	pop   edx
	pop   ecx
	pop   ebx
	pop   eax		;现场已经恢复了
	iretd			;注意：是iretd 而不是 iret 啊!!
;=========================================================================
;软盘中断处理程序
floppy_interrupt:
	push  eax 
	push  ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs				;现场已经保护好了
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	sti	
	xor   eax, eax
	xchg  eax, pfnDo_floppy
	test  eax, eax
	jne   @f
	mov   eax, fnUnknown_interrupt
@@:
	call  eax				;调用软盘处理函数
	cli
	mov al, 20h				;发送EOI
	out 20h, al
	nop
	nop
;@@@@@@@@@@@@@@@@@@
	pop   gs
	pop   fs
	pop   es
	pop   ds
	pop   ebp
	pop   esi
	pop   edi
	pop   edx
	pop   ecx
	pop   ebx
	pop   eax		;现场已经恢复了
	iretd			;注意：是iretd 而不是 iret 啊!!
;=========================================================================
;硬盘中断处理程序
hd_interrupt:
	push  eax 
	push  ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs				;现场已经保护好了
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	sti	
	mov   eax, pfnDo_hd
	call  eax				;调用硬盘处理函数
	cli
	mov al, 20h				;发送EOI给从片
	out 0A0h, al
	nop
	nop
	mov al, 20h				;发送EOI给主片
	out 20h, al
	nop
	nop
;@@@@@@@@@@@@@@@@@@
	pop   gs
	pop   fs
	pop   es
	pop   ds
	pop   ebp
	pop   esi
	pop   edi
	pop   edx
	pop   ecx
	pop   ebx
	pop   eax		;现场已经恢复了
	iretd			;注意：是iretd 而不是 iret 啊!!
;------------------------------------------------------------------------*
;系统调用Sys_MountRoot---int 80h
system_calls:
	push  eax 
	push  ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs				;现场已经保护好了
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	sti	
	call  MountRootfs			;加载根文件系统
	cli
;@@@@@@@@@@@@@@@@@@
	pop   gs
	pop   fs
	pop   es
	pop   ds
	pop   ebp
	pop   esi
	pop   edi
	pop   edx
	pop   ecx
	pop   ebx
	pop   eax		;现场已经恢复了
	iretd			;注意：是iretd 而不是 iret 啊!!
END 















