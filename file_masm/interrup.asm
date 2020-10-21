;˵���� ��ģ�����������ж���������
;ʱ�䣺2007��3��28�� ��13:09:35
;	�ƺ�ˮ
;----------
;�ж�ʱ����ջ�л��� �ں˶�ջ��ȥ;���뱣֤��һ���ж�һ����ring3 -----> ring0
;
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		.386p
		.model flat ,c
		option casemap:none


;����һЩ�ⲿ����ĺ���
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

;����һЩ���ⲿ���õ� �����š�
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
	push  fndo_DivideError	;�������쳣�����ַ��ջ

no_error_code:			;��Щ�ж�û�� errorcode
	xchg  eax ,[esp]	;��ʱ[esp]�� �������ĵ�ַ������-->eax
	push  ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs		;�ֳ��Ѿ���������,eax == ��������ַ
;@@@@@@@@@@@@@@@@@@
	push  eax
	mov   ax ,SelectData32	;�ں����ݶ�
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	pop   eax
	sti
	push  0			; error_code = 0 ,�����趨��
	call  eax		;�����жϴ�����
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
	pop   eax		;�ֳ��Ѿ��ָ���
	iretd			;ע�⣺��iretd ������ iret ��!!

debug:				; ���� t 
	push  fndo_Int3
	jmp   no_error_code

nmi:
	push  fndo_Nmi		; �������ж�
	jmp   no_error_code
	
int3:
	push  fndo_Int3		; int 3 �ϵ�
	jmp   no_error_code

overflow:
	push  fndo_Overflow	;���������
	jmp   no_error_code

bounds:
	push  fndo_Bounds	;�߽�������ж�
	jmp   no_error_code

invalid_op:			;�Ƿ�����ָ��
	push  fndo_Invalid_Op
	jmp   no_error_code

;�豸�����ã�û��д�� int 7
coprocessor_segment_overrun:	;int 9 Э�������γ���
	push  fndo_Coprocessor_SegmentOver
	jmp   no_error_code

reserved:	;int 15 intel����
	push  fndo_Reserved
	jmp   no_error_code


;--------------------------------------
;�������������error code ���жϻ��쳣:  int 8  ,int 10 - 14 ;
;�����뽫��ѹջ����eip ֮�� ��
;error code ��Ҫ�Լ���ջ�ϵ���
double_fault:
	push  fndo_Double_fault

error_code:
	xchg  eax ,[esp + 4]    ; ebx �н����� error code 
	xchg  ebx ,[esp]	;��ʱ[esp]�� �������ĵ�ַ������-->ebx
	push  ecx 
	push  edx
	push  edi
	push  esi
	push  ebp
	push  ds
	push  es
	push  fs
	push  gs		;�ֳ��Ѿ���������,ebx == ��������ַ
;@@@@@@@@@@@@@@@@@@
	mov   ecx, [esp + 44]
	mov   glbErrEip, ecx    ;������������eip
	push  eax
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	pop   eax
	sti
	push  eax		; error_code = eax ,�����趨��
	call  ebx		;�����жϴ�����
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
	pop   eax		;�ֳ��Ѿ��ָ��� 
	iretd			;ע�⣺��iretd ������ iret ��!!

invalid_TSS:			;int 10 ��Ч������״̬��
	push  fndo_Invalid_Tss
	jmp   error_code

segment_not_present:		;int 11 �β�����
	push  fndo_Segment_absence
	jmp   error_code

stack_segment_error:			;int 12 ��ջ����, �����ڻ� Խ��
	push  fndo_Stack_Error
	jmp   error_code

general_protection:		;int 13 û�з���80386����ģʽ���ƣ���Ȩ�����Ĳ�������
	push  fndo_General_Protection
	jmp   error_code

;int 14 ҳ������mm �ж���
page_fault:
	push  fndo_Pagefault
	jmp   error_code
;һЩӲ�����ж�����Ӧ���ļ��д���
clock:;ʱ���ж�
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
	push  gs				;�ֳ��Ѿ���������
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	mov   eax ,  dword ptr[esp + 48]	;get cs that be interrupted
	and   eax ,03h
	push  eax				;push cpl
;����EOI,������ǰ��Ϊfndo_clock()���ܷ��������л���������û����send EIO!,ע������û�п����ж�sti
	mov al, 20h				
	out 20h, al
	call  fndo_clock			;����ʱ�Ӵ�����
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
	pop   eax		;�ֳ��Ѿ��ָ���
	iretd			;ע�⣺��iretd ������ iret ��!!
;======================================================================
;�����жϴ������
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
	push  gs				;�ֳ��Ѿ���������
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	sti	
	call  fndo_keyboard			;���ü��̴�����
	cli
	mov al, 20h				;����EOI
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
	pop   eax		;�ֳ��Ѿ��ָ���
	iretd			;ע�⣺��iretd ������ iret ��!!
;=========================================================================
;�����жϴ������
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
	push  gs				;�ֳ��Ѿ���������
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
	call  eax				;�������̴�����
	cli
	mov al, 20h				;����EOI
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
	pop   eax		;�ֳ��Ѿ��ָ���
	iretd			;ע�⣺��iretd ������ iret ��!!
;=========================================================================
;Ӳ���жϴ������
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
	push  gs				;�ֳ��Ѿ���������
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	sti	
	mov   eax, pfnDo_hd
	call  eax				;����Ӳ�̴�����
	cli
	mov al, 20h				;����EOI����Ƭ
	out 0A0h, al
	nop
	nop
	mov al, 20h				;����EOI����Ƭ
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
	pop   eax		;�ֳ��Ѿ��ָ���
	iretd			;ע�⣺��iretd ������ iret ��!!
;------------------------------------------------------------------------*
;ϵͳ����Sys_MountRoot---int 80h
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
	push  gs				;�ֳ��Ѿ���������
;@@@@@@@@@@@@@@@@@@
	mov   ax ,SelectData32
	mov   ds ,ax
	mov   es ,ax
	mov   gs ,ax
	sti	
	call  MountRootfs			;���ظ��ļ�ϵͳ
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
	pop   eax		;�ֳ��Ѿ��ָ���
	iretd			;ע�⣺��iretd ������ iret ��!!
END 















