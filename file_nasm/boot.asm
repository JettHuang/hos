;   �����ҵ�һ�ο�ʼ����д����ϵͳ���ҽ�������Ϊ Hos ,ϣ���ڷ���linux 0.11��ͬʱ���ܹ����
;��������ϵͳ����ƺ�ʵ�֣���ʶ�����ۺ�ʵ���ľ����ԡ�
;   Ӧ��֪������·�Ƿǳ������ģ���Ը���ܹ������ȥ��
;			huangehsui  2007��3��18�� / 17:38:36	
;----------------------------------------------------------------------------------------------

;boot.asm �������ǣ� 1,���Լ���0000:7c00 ��ת�Ƶ� 9000:0000 ,Ȼ�����setupģ�鵽 9000:0200 ,��
;systemģ����ص� 1000:0000 ����
;
;���� ������1.44M 


org	0h		;�� couter = 0

;����һЩ����
SETUPLEN   EQU   4      ;setup.bin��ռ4 sectors
BOOTSEG	   EQU	 07c0h  ;boot �����ڵĶ� ,���뱻���ص� 0000:7c00h ��	
INITSEG    EQU	 9000h  ;boot ---> 9000:0000 ��
SETUPSEG   EQU	 9020h  ;setup.bin �����ص� 9020:0000
SYSTEMSEG  EQU	 1000h  ;system ģ�鱻���ص� 1000:0000
SYSLEN	   EQU	 192 * 2 ;��λsectors ,�����ں��ܴ�С == 192 KB

[SECTION .start]
[BITS 16]
start:	
	mov  ax ,BOOTSEG
	mov  ds ,ax
	mov  ax ,INITSEG 
	mov  es ,ax
	xor  si ,si		;Դ��ַ 7c00:0000
	xor  di ,di		;Ŀ�ĵ� 9000:0000
	mov  cx ,256		;256��word
	rep  movsw		;һ���ƶ� 256 * 2 Bytes	  	
	jmp  INITSEG:_go		
_go:
	
	mov  ax ,cs
	mov  ds ,ax
	mov  es ,ax
	;���ö�ջ at 9000:fA00h
	mov  ss ,ax
	mov  sp ,0fA00h		;���� setup���Ǻܴ�
	
;���濪ʼ����setup.bin�� 9000:0200�� ,�����̵ĵ�2��������ʼ ���� 90200h ��
;INT 13h / AH = 02h - read disk sectors into memory.
;������
;	al = number of sectors to read/write (must be nonzero)
;	ch = cylinder number (0..79).
;	cl = sector number (1..18).
;	dh = head number (0..1).
;	dl = drive number (0..3 , for the emulator it depends on quantity of floppy_ files).
;	es:bx points to data buffer.

load_setup:
	mov  dx ,0000h
	mov  cx ,0002h 
	mov  bx ,0200h	;es:bx == 9000:0200 
	mov  ax ,0200h + SETUPLEN
	int  13h
	jnc  ok_load_setup    ;���سɹ�!
	xor  dx ,dx
	xor  ax ,ax
	int  13h
	jmp  load_setup

ok_load_setup:
	mov  ax ,SYSTEMSEG
	mov  es ,ax
;׼������ , 64KB == 128 sectors
	mov  word[dwCurrentSec] ,5      ;��5������ʼ,base ==0

read_next_64KB:
	cmp  word[dwSysLen], 128
	jb   read_below_128		;word[dwSysLen] < 128
	; read  64Kb data == 128 sectors
	sub  word[dwSysLen], 128	;word[dwSysLen] -= 128
	mov  ax ,word[dwCurrentSec]		
	mov  cl ,128
	xor  bx ,bx
	call _fnReadSectors		;load system
	add  word[dwCurrentSec],128
	mov  ax ,es 
	add  ax	,1000h		;������ + 64KB
	mov  es ,ax
	jmp  read_next_64KB
read_below_128:
	mov  ax ,word[dwCurrentSec]		
	mov  cl ,byte[dwSysLen]
	xor  bx ,bx
	call _fnReadSectors		
	call _fnKill_Motor		;�ر����
;��ʾ"Loading System..."
	mov  ax ,INITSEG
	mov  es ,ax
	mov  ax , szPromt
	mov  bp , ax			; ES:BP = ����ַ
	mov  cx , szPromtLen		; CX = ������
	mov  ax , 01301h		; AH = 13,  AL = 01h
	mov  bx , 000ah			; ҳ��Ϊ0(BH = 0) �ڵ�����(BL = 0ah,����)
	mov  dx , 0000h			; row = 0 ,column = 0
	int  10h			; 10h ���ж�

	; long jmp to setup.bin
	jmp  SETUPSEG: 0		

;=======================================================================================
;���涨��һЩ������������ȡSystem
;
;���ܣ��ӵ�ax������ ��ʼ����cl����������es:bx �С�
;��ע���������κμĴ��� ,������ base 0
;	ҲҪע�⻺�����Ĵ�С����ֹ���� +++ ����� +++
_fnReadSectors:
	push  bp
	mov   bp ,sp
	sub   sp ,6
	mov   word[bp-2] ,ax	
	mov   byte[bp-4] ,cl
	mov   word[bp-6] ,bx

read_nextsec:
	cmp   byte[bp-4] ,0
	je    end_readsec	;��������ˣ�����ת����
	;�����߼������������ַ
	mov   ax ,word[bp-2]
	mov   cl,18		;sectors/trace == 18
	div   cl		;ah = ����R ,al =��Q
	inc   ah		
	mov   cl ,ah		;�õ���һ�������ϵ������� ��1---18
	mov   ch ,al
	shr   ch ,1		; Q/2 == �����
	mov   dh ,al
	and   dh ,00000001b     ;Q & 1 == ��ͷ�ţ������̵���/����
	xor   dl ,dl		;׼���� A ��
;    goon_read_error:
	mov  ah ,02h
	mov  al ,19
	sub  al ,cl		;����ÿ��Ӧ�ö� al ������
	cmp  al ,byte[bp-4]
	jb   al_below
	mov  al ,byte[bp-4]
al_below:
	mov  bx ,word[bp-6]
	int  13h		; al = ������ sector ���� , ah =0 
	jnc   read_one_ok	;CF == 1 ���ִ��󣬼�����
	mov  dx ,0
	mov  ax ,0
	int  13h
	jmp  read_nextsec	;�����ض�!!
read_one_ok:
	add  word[bp -2] ,ax
	sub  byte[bp -4] ,al
	shl  ax ,9
	add  word[bp-6] ,ax		;���������� al ����
	jmp  read_nextsec

    end_readsec:
	mov  sp ,bp
	pop  bp
	ret
;End  _fnReadSectors
;���ܣ��ر����
_fnKill_Motor:
	push  dx
	mov  dx ,3f2h	;�������ƿ��������˿ڣ�ֻд
	mov  al ,0      ;A ���������ر�FDC����ֹDMA���ж����󣬹رյ綯��
	out  dx ,al
	pop  dx
	ret
;End '_kill_motor'
dwSysLen:   dw  SYSLEN
dwCurrentSec:  dw  0
szPromt:   db  0dh ,0ah,'Loading System...' ;��19��
szPromtLen   equ  $ - szPromt
fill_zero:	times   510 -($-$$)  db  0
boot_flag:	dw  0AA55h


;�ܽ᣺ ���Գɹ�!
;---------------------------------------------------------
;  the algorithm  for  reading  system 
;
;Goon:
;if( numOfSector < 64*1024 /512)
;{
;	cl = numOfsector 
;	bx = bx 
;	call _fnReadSectors 
;	cl = 0 ;
;	����
;}
;else
;{
;	cl = 64*1024 /512 
;	bx = bx 
;	call _fnReadSectors 
;	cl -= 64*1024 /512 
;	es = nextSeg
;	jmp Goon ;
;	
;}
;-----------------------------------------------------------