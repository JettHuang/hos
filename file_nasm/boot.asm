;   这是我第一次开始动手写操作系统，我将它命名为 Hos ,希望在分析linux 0.11的同时，能够充分
;地理解操作系统的设计和实现，认识到理论和实践的距离性。
;   应该知道这条路是非常艰辛的，但愿我能够坚持下去！
;			huangehsui  2007年3月18日 / 17:38:36	
;----------------------------------------------------------------------------------------------

;boot.asm 的作用是： 1,将自己从0000:7c00 处转移到 9000:0000 ,然后加载setup模块到 9000:0200 ,将
;system模块加载到 1000:0000 处。
;
;假设 软盘是1.44M 


org	0h		;将 couter = 0

;定义一些常量
SETUPLEN   EQU   4      ;setup.bin的占4 sectors
BOOTSEG	   EQU	 07c0h  ;boot 的所在的段 ,代码被加载到 0000:7c00h 处	
INITSEG    EQU	 9000h  ;boot ---> 9000:0000 处
SETUPSEG   EQU	 9020h  ;setup.bin 被加载到 9020:0000
SYSTEMSEG  EQU	 1000h  ;system 模块被加载到 1000:0000
SYSLEN	   EQU	 192 * 2 ;单位sectors ,假设内核总大小 == 192 KB

[SECTION .start]
[BITS 16]
start:	
	mov  ax ,BOOTSEG
	mov  ds ,ax
	mov  ax ,INITSEG 
	mov  es ,ax
	xor  si ,si		;源地址 7c00:0000
	xor  di ,di		;目的地 9000:0000
	mov  cx ,256		;256个word
	rep  movsw		;一共移动 256 * 2 Bytes	  	
	jmp  INITSEG:_go		
_go:
	
	mov  ax ,cs
	mov  ds ,ax
	mov  es ,ax
	;设置堆栈 at 9000:fA00h
	mov  ss ,ax
	mov  sp ,0fA00h		;假设 setup不是很大
	
;下面开始加载setup.bin到 9000:0200处 ,从软盘的第2个扇区开始 读到 90200h 处
;INT 13h / AH = 02h - read disk sectors into memory.
;参数：
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
	jnc  ok_load_setup    ;加载成功!
	xor  dx ,dx
	xor  ax ,ax
	int  13h
	jmp  load_setup

ok_load_setup:
	mov  ax ,SYSTEMSEG
	mov  es ,ax
;准备参数 , 64KB == 128 sectors
	mov  word[dwCurrentSec] ,5      ;第5扇区开始,base ==0

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
	add  ax	,1000h		;缓冲区 + 64KB
	mov  es ,ax
	jmp  read_next_64KB
read_below_128:
	mov  ax ,word[dwCurrentSec]		
	mov  cl ,byte[dwSysLen]
	xor  bx ,bx
	call _fnReadSectors		
	call _fnKill_Motor		;关闭马达
;显示"Loading System..."
	mov  ax ,INITSEG
	mov  es ,ax
	mov  ax , szPromt
	mov  bp , ax			; ES:BP = 串地址
	mov  cx , szPromtLen		; CX = 串长度
	mov  ax , 01301h		; AH = 13,  AL = 01h
	mov  bx , 000ah			; 页号为0(BH = 0) 黑底绿字(BL = 0ah,高亮)
	mov  dx , 0000h			; row = 0 ,column = 0
	int  10h			; 10h 号中断

	; long jmp to setup.bin
	jmp  SETUPSEG: 0		

;=======================================================================================
;下面定义一些函数，用来读取System
;
;功能：从第ax个扇区 开始，将cl个扇区读入es:bx 中。
;备注：不保存任何寄存器 ,扇区号 base 0
;	也要注意缓冲区的大小，防止发生 +++ 段溢出 +++
_fnReadSectors:
	push  bp
	mov   bp ,sp
	sub   sp ,6
	mov   word[bp-2] ,ax	
	mov   byte[bp-4] ,cl
	mov   word[bp-6] ,bx

read_nextsec:
	cmp   byte[bp-4] ,0
	je    end_readsec	;如果读完了，就跳转结束
	;计算逻辑扇区的物理地址
	mov   ax ,word[bp-2]
	mov   cl,18		;sectors/trace == 18
	div   cl		;ah = 余数R ,al =商Q
	inc   ah		
	mov   cl ,ah		;得到在一个柱面上的扇区号 ，1---18
	mov   ch ,al
	shr   ch ,1		; Q/2 == 柱面号
	mov   dh ,al
	and   dh ,00000001b     ;Q & 1 == 磁头号，即软盘的正/反面
	xor   dl ,dl		;准备读 A 盘
;    goon_read_error:
	mov  ah ,02h
	mov  al ,19
	sub  al ,cl		;计算每次应该读 al 个扇区
	cmp  al ,byte[bp-4]
	jb   al_below
	mov  al ,byte[bp-4]
al_below:
	mov  bx ,word[bp-6]
	int  13h		; al = 所读的 sector 个数 , ah =0 
	jnc   read_one_ok	;CF == 1 出现错误，继续读
	mov  dx ,0
	mov  ax ,0
	int  13h
	jmp  read_nextsec	;出错重读!!
read_one_ok:
	add  word[bp -2] ,ax
	sub  byte[bp -4] ,al
	shl  ax ,9
	add  word[bp-6] ,ax		;缓冲区增加 al 扇区
	jmp  read_nextsec

    end_readsec:
	mov  sp ,bp
	pop  bp
	ret
;End  _fnReadSectors
;功能：关闭马达
_fnKill_Motor:
	push  dx
	mov  dx ,3f2h	;软驱控制卡的驱动端口，只写
	mov  al ,0      ;A 驱动器，关闭FDC，禁止DMA和中断请求，关闭电动机
	out  dx ,al
	pop  dx
	ret
;End '_kill_motor'
dwSysLen:   dw  SYSLEN
dwCurrentSec:  dw  0
szPromt:   db  0dh ,0ah,'Loading System...' ;共19个
szPromtLen   equ  $ - szPromt
fill_zero:	times   510 -($-$$)  db  0
boot_flag:	dw  0AA55h


;总结： 调试成功!
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
;	结束
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