;˵���� setup����ROM BIOS�ж϶�ȡ����ϵͳ���ݣ�������Щϵͳ���ݱ��浽
;0X9000��0000 ��ʼ��λ��(���ǵ���boot�������ڵĵط�), ����Щ�������֮��
;���ǾͲ���Ҫʹ��ROM�е��ж��ˣ����Խ�system �ƶ��� 0000:0000 ��
;	huangheshui  2007��3��20�� ,17:59:32
;
;--------------------------------------------------------------------
%include "pm.inc"

org	0h

INITSEG    EQU	 9000h  ;boot ---> 9000:0000 ��
SETUPSEG   EQU	 9020h  ;setup.bin �����ص� 9020:0000
SYSTEMSEG  EQU	 1000h  ;system ģ�鱻���ص� 1000:0000

[SECTION .setup]
[BITS 16]
start:

;------------------------���Ĵ���----------------------------
;����ͨ��BIOS ���жϣ�����һЩ��ǰϵͳ����Ϣ
	mov  ax ,INITSEG
	mov  ds ,ax
;���浱ǰ�Ĺ��λ��
;INT 10h / AH = 03h - get cursor position and size.
;input:
;	BH = page number.
;return:
;	DH = row.
;	DL = column.
;	CH = cursor start line.
;	CL = cursor bottom line.
	mov  ah ,03h
	mov  bl ,0h
	int  10h
	mov  [0], dx ; dl = column ,dh = row
	
;��ȡ��չ�ڴ�Ĵ�С, >1MB �Ĳ���
;int 15h ,ah = 88h .����: ax = ��0x100000(1M)����ʼ����չ�ڴ��С(KB)
;	mov  ah ,088h
;	int  15h
;	mov  [2] ,ax  ;�ڴ���չ,��λ==KB
	mov  ax ,0e801h
	int  15h
;	jc   Error
	mov  [100h] ,cx	;cx = 1M--16M
	mov  [102h] ,dx ;dx = 16M-- 4G
;��ȡ�Կ��ĵ�ǰģʽ,
;int 10h , ah = 0fh ,
;����: ah = �ַ����� ,al = ��ʾģʽ ;bh = ��ǰ��ʾҳ.
	mov ah ,0fh
	int 10h
	mov [4] ,bh
	mov [6] ,ax 

;�����ʾ��ʽ(EGA/VGA)��ȡ����
;int 10h ,ah =12h ,bl = 10h 
;����: bh = ��ʾ״̬ , bl = ��װ����ʾ�ڴ� ,cx = �Կ����Բ���.
	mov ah ,12h
	mov bl ,10h
	int 10h
	mov [8] ,ax
	mov [10] ,bx
	mov [12] ,cx

;Get  hd0 data
;��һ��Ӳ�̲��������׵�ַ���ж�����0X41������ֵ ,
;��2��Ӳ�̲��������׵�ַ���ж�����0X46������ֵ ; ���ĳ���== 16 bytes
; 0ffset�͵�ַ,segment�ߵ�ַ
	mov  ax ,0000h	    ;�ж���������
	mov  ds ,ax
	lds  si ,[4 * 41h]  ; load ds:4*41h �����ݵ�t�Ĵ��� ds:si
	mov  ax ,INITSEG
	mov  es ,ax
	mov  di ,0080h
	mov  cx ,10h	    ;16 bytes
	rep  movsb

;Get hd1 data
	mov  ax ,0000h
	mov  ds ,ax
	lds  si ,[4 * 46h] ; load to ds:si
	mov  ax ,INITSEG
	mov  es ,ax
	mov  di ,0090h
	mov  cx ,10h	    ;16 bytes
	rep  movsb

;�ж��Ƿ���ڵ�2��Ӳ��,������������ ��2��������!
;  int 13h ,ah = 0x15 ;ȡ�����͹���
;input:
;	dl = ��������(0x80X��Ӳ��,0x80 ָ��1��Ӳ��,0x81 ��2��Ӳ��).
;output:
;	ah = ������ ;00h- û�����Ӳ��,CF��λ; 01- ����,û��change-line ֧��;
;	02 - ����(���������ƶ��豸),��change-line֧�� ;03 -��Ӳ��

	mov ah ,15h
	mov dl ,81h
	int 13h
	jc  no_hd1
	cmp ah ,03h
	je  has_hd1
no_hd1:   ;û�е�2��Ӳ�� ,��յ�2��Ӳ�̼�¼
	mov  ax ,INITSEG
	mov  es ,ax
	mov  di ,0090h
	xor  al ,al
	mov  cx ,10h	    ;16 bytes
	rep  stosb
	
has_hd1:
;��ʾ "In the setup"
	mov  ax ,SETUPSEG
	mov  es ,ax
	mov  ax , szPromt
	mov  bp , ax			; ES:BP = ����ַ
	mov  cx , szPromtLen		; CX = ������
	mov  ax , 01301h		; AH = 13,  AL = 01h
	mov  bx , 000ah			; ҳ��Ϊ0(BH = 0) �ڵ�����(BL = 0ah,����)
	mov  dx , 0100h			; row = 3 ,column = 0
	int  10h			; 10h ���ж�
;��system �� 0x10000 �ƶ��� 0x00000��
	mov ax ,0000h
	cld
do_move:
	mov es ,ax	;Ŀ���		ds:si ---> es:di
	add ax ,1000h   
	cmp ax ,9000h	;��512KB,0x10000 ~ 0x8ffff ---> 0x00000 ~ 0x7ffff
	je  end_move
	mov ds ,ax	;��Դ��
	xor si ,si
	xor di ,di
	mov cx ,8000h	;8000h ���� == 64KB
	rep movsw	;Debug ... ,�ǳ�������ֵ���bochs 2.3������Ῠס ,��Ϊ������ȫģ��Ӳ����
	jmp do_move	;����������У�������ܷ�����ס����

end_move:

;�����ڿ�ʼ,������Ҳ����BIOS�ṩ�Ĺ���, ���ǽ��߽�32λ�ı���ģʽ
;�����ζ���Ժ�Ĵ��뽫�� 32 λ�Ĵ���.
	
	;����gdtr
	mov  ax ,SETUPSEG
	mov  ds ,ax
	lgdt [GdtPtr]

	;����ϣ��Ӧ����������...
	;���ж�
	cli
	;��ʼ������ģʽ�µ�8259A ,ϣ��ջӦ��û�д���
funInit8259A:
;������ȫǶ��ģʽ��
;��ʼ����ICW (Initialization Command Word )
	mov  al, 11h
	out  20h,al		;��Ƭ8259��ICW1
	call io_delay

	out 0A0h, al		;��Ƭ8259, ICW1
	call io_delay

	mov al, 020h		;IRQ0 ��Ӧ�ж�����0X20
	out 21h, al		;��Ƭ8259��ICW2
	call io_delay

	mov al,028h		;IRQ8 ��Ӧ�ж�����0X28
	out 0A1h, al		;��Ƭ8259��ICW2
	call io_delay

	mov al,04h		;IR2������Ƭ
	out 21h, al		;��Ƭ8259 ICW3
	call io_delay		

	mov al,02h		;��Ƭ������Ƭ��IR2
	out 0A1h, al		;��Ƭ8259 ICW3
	call io_delay

	mov al, 001h
	out 21h ,al		;��Ƭ8259 ICW4
	call io_delay

	out 0A1h,al		;��Ƭ8259 ICW4
	call io_delay
;��ʼ����OCW1( operation Control Word )
	mov al,11111100b       
	out 21h ,al		;������Ƭ8259�����ж� ,����ʱ��,����
	call io_delay 

	mov al,11111111b
	out 0A1h ,al		;���δ�Ƭ8259�����ж�
	call io_delay
;����8253������/��ʱ����ʱ���ж���Ϊ10msһ��
	mov al ,00110100b	; counter0 ,mode2 , lmb/hmb
	out 43h ,al
	mov al ,09bh
	out 40h, al
	mov al ,02eh
	out 40h, al
;------------------------------------------------------------------
	;�򿪵�ַ��A20
	in  al, 92h
	or  al, 00000010b
	out 92h, al
	
	;׼���л�������ģʽ
	mov  eax, cr0
	or   eax, 1
	mov  cr0, eax   ;���ñ���ģʽ����

	;���Ԥȡָ����У�����ת���뱣��ģʽ^_^
	jmp dword SelectCode32:(SETUPSEG * 16 + start32 + 0c0000000h) ;��SelectCode32 װ��cs ,����ת��SelectCode32:0 ��
;�ӳٺ���
io_delay:
	nop
	nop
	nop
	nop
	ret

;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
[SECTION .code32]
[BITS  32]
start32:
	mov ax ,SelectData32
	mov ds ,ax
	jmp  SelectCode32:( 1000h + 0c0000000h )	;��ת��system.exe �е�head


align  3
;ȫ����������			  �λ�ַ,       �ν���     , ����
Gdt_desc_Null:	  Descriptor       0 ,            0 ,         0
Gdt_desc_Code32:  Descriptor       40000000h ,    0fffffh ,      DA_C  + DA_32 + DA_LIMIT_4K
Gdt_desc_Data32:  Descriptor       40000000h ,    0fffffh ,      DA_DRW + DA_LIMIT_4K   
Gdt_desc_Stack32: Descriptor       40000000h ,    0fffffh ,      DA_DRW + DA_LIMIT_4K + DA_32 ;����ss��˵ DA_32 �����Ҫ
;��ȡ��ѡ���ӣ�������
SelectCode32	equ  Gdt_desc_Code32 - Gdt_desc_Null  ;8
SelectData32	equ  Gdt_desc_Data32  - Gdt_desc_Null  ;16

;������������
align 2
GdtLen    equ  $ - Gdt_desc_Null   ;GDT�ĳ���
GdtPtr:    dw   GdtLen - 1          ;GDT�ν�
	  dd   SETUPSEG * 16 + Gdt_desc_Null 	;GDT����ַ

szPromt:    db   0dh ,0ah ,'in the setup !' ,0dh ,0ah 
szPromtLen   equ   $ - szPromt

