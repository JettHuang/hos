/*--------------------------------------------------------
 *该文件存放scan code set1
 *"scan code" <--> "key" map
 *-------------------------------------------------------*/
#ifndef	__HOS_KEYMAP_H
#define	__HOS_KEYMAP_H

#include "..\include\logkey.h"


#define NUM_SINGLE (sizeof(stKM_Single) / sizeof(struct KeyMap))
#define NUM_DOUBLE (sizeof(stKM_Double) / sizeof(struct KeyMap))
#define NUM_TREBLE (sizeof(stKM_Treble) / sizeof(struct KeyMap))
/* Keymap for US MF-2 keyboard. 
 * 将扫描码分成三个组: NO_E0, Has_E0, Has_E1
 */
struct KeyMap{
	unsigned char  make_code;		/* pressed scan code, break = make + 0x80 */
	unsigned char  nature_lkey;     /* without any other system key pressed*/
	unsigned char  shift_lkey;      /* with shift key pressed but caps lock not pressed*/
	unsigned char  ctrl_lkey;        /* with only ctrl key pressed*/
}

/* scan code is only one byte */
stKM_Single[] = 
{
/* 0x00 - none		*/0x00, LK_NULL_KEY, LK_NULL_KEY, LK_NULL_KEY,
/* 0x01 - ESC		*/0x01, LK_ESCAPE,   LK_NULL_KEY,      LK_NULL_KEY,
/* 0x02 - '1'		*/0x02, LK_ONE,      LK_EXCALMATORY,   LK_NULL_KEY,		
/* 0x03 - '2'		*/0x03, LK_TWO,      LK_AT,            LK_NULL_KEY,		
/* 0x04 - '3'		*/0x04, LK_THREE,	 LK_WELL,          LK_NULL_KEY,	
/* 0x05 - '4'		*/0x05, LK_FOUR,	 LK_DOLLAR,        LK_NULL_KEY,	
/* 0x06 - '5'		*/0x06, LK_FIVE,	 LK_MODULE,        LK_NULL_KEY,	
/* 0x07 - '6'		*/0x07, LK_SIX, 	 LK_XOR,           LK_NULL_KEY,
/* 0x08 - '7'		*/0x08, LK_SEVEN,    LK_AND,           LK_NULL_KEY,		
/* 0x09 - '8'		*/0x09, LK_EIGHT,    LK_ASTERISK,      LK_NULL_KEY,		
/* 0x0A - '9'		*/0x0A, LK_NINE,	 LK_LROUND_BRAKET, LK_NULL_KEY,
/* 0x0B - '0'		*/0x0B, LK_ZERO,     LK_RROUND_BRAKET, LK_NULL_KEY,
/* 0x0C - '-'		*/0x0C, LK_SUBTRACTION, LK_BASELINE, LK_NULL_KEY,		
/* 0x0D - '='		*/0x0D, LK_EQUALSIGN, LK_PLUSSIGN,   LK_NULL_KEY,  	
/* 0x0E - BS		*/0x0E, LK_BACKSPACE, LK_NULL_KEY,   LK_NULL_KEY, 			
/* 0x0F - TAB		*/0x0F, LK_TABLE,     LK_TABLE,      LK_NULL_KEY,								
/* 0x10 - 'q'		*/0x10, LK_LOWERCASE_Q, LK_UPPERCASE_Q, LK_CTRL_KEYQ,		
/* 0x11 - 'w'		*/0x11, LK_LOWERCASE_W, LK_UPPERCASE_W, LK_CTRL_KEYW,
/* 0x12 - 'e'		*/0x12, LK_LOWERCASE_E, LK_UPPERCASE_E, LK_CTRL_KEYE,
/* 0x13 - 'r'		*/0x13, LK_LOWERCASE_R, LK_UPPERCASE_R, LK_CTRL_KEYR,
/* 0x14 - 't'		*/0x14, LK_LOWERCASE_T, LK_UPPERCASE_T, LK_CTRL_KEYT,
/* 0x15 - 'y'		*/0x15, LK_LOWERCASE_Y, LK_UPPERCASE_Y, LK_CTRL_KEYY,
/* 0x16 - 'u'		*/0x16, LK_LOWERCASE_U, LK_UPPERCASE_U, LK_CTRL_KEYU,
/* 0x17 - 'i'		*/0x17, LK_LOWERCASE_I, LK_UPPERCASE_I, LK_CTRL_KEYI,
/* 0x18 - 'o'		*/0x18, LK_LOWERCASE_O, LK_UPPERCASE_O, LK_CTRL_KEYO,		
/* 0x19 - 'p'		*/0x19, LK_LOWERCASE_P, LK_UPPERCASE_P, LK_CTRL_KEYP,
/* 0x1A - '['		*/0x1A, LK_LSQUARE_BRACKET, LK_LBRACE,  LK_NULL_KEY,
/* 0x1B - ']'		*/0x1B, LK_RSQUARE_BRACKET, LK_RBRACE,  LK_NULL_KEY,
/* 0x1C - CR/LF		*/0x1C, LK_RETURN,          LK_RETURN,  LK_NULL_KEY,
/* 0x1D - l. Ctrl	*/0x1D, SYS_LCONTROL,       SYS_LCONTROL, SYS_LCONTROL,  /*控制键很特别*/
/* 0x1E - 'a'		*/0x1E, LK_LOWERCASE_A,     LK_UPPERCASE_A,  LK_CTRL_KEYA, 
/* 0x1F - 's'		*/0x1F, LK_LOWERCASE_S,     LK_UPPERCASE_S,  LK_CTRL_KEYS, 
/* 0x20 - 'd'		*/0x20, LK_LOWERCASE_D,     LK_UPPERCASE_D,  LK_CTRL_KEYD, 				
/* 0x21 - 'f'		*/0x21, LK_LOWERCASE_F,     LK_UPPERCASE_F,  LK_CTRL_KEYF, 	
/* 0x22 - 'g'		*/0x22, LK_LOWERCASE_G,     LK_UPPERCASE_G,  LK_CTRL_KEYG, 		
/* 0x23 - 'h'		*/0x23, LK_LOWERCASE_H,     LK_UPPERCASE_H,  LK_CTRL_KEYH, 				
/* 0x24 - 'j'		*/0x24, LK_LOWERCASE_J,     LK_UPPERCASE_J,  LK_CTRL_KEYJ, 		
/* 0x25 - 'k'		*/0x25, LK_LOWERCASE_K,     LK_UPPERCASE_K,  LK_CTRL_KEYK, 		
/* 0x26 - 'l'		*/0x26, LK_LOWERCASE_L,     LK_UPPERCASE_L,  LK_CTRL_KEYL, 	
/* 0x27 - ';'		*/0x27, LK_SEMICOLON,       LK_COLON,        LK_NULL_KEY,
/* 0x28 - '\''		*/0x28, LK_SQUOTATION,      LK_DQUOTATION,   LK_NULL_KEY,
/* 0x29 - '`'		*/0x29, LK_HEXSIXTY,        LK_REVERSAL,     LK_NULL_KEY,
/* 0x2A - l. SHIFT	*/0x2A, SYS_LSHIFT,         SYS_LSHIFT,      SYS_LSHIFT,
/* 0x2B - '\'		*/0x2B, LK_BACKSLASH,       LK_OR,           LK_NULL_KEY,    
/* 0x2C - 'z'		*/0x2C, LK_LOWERCASE_Z,     LK_UPPERCASE_Z,  LK_CTRL_KEYZ, 
/* 0x2D - 'x'		*/0x2D, LK_LOWERCASE_X,     LK_UPPERCASE_X,  LK_CTRL_KEYX, 
/* 0x2E - 'c'		*/0x2E, LK_LOWERCASE_C,     LK_UPPERCASE_C,  LK_CTRL_KEYC, 
/* 0x2F - 'v'		*/0x2F, LK_LOWERCASE_V,     LK_UPPERCASE_V,  LK_CTRL_KEYV, 
/* 0x30 - 'b'		*/0x30, LK_LOWERCASE_B,     LK_UPPERCASE_B,  LK_CTRL_KEYB, 
/* 0x31 - 'n'		*/0x31, LK_LOWERCASE_N,     LK_UPPERCASE_N,  LK_CTRL_KEYN, 
/* 0x32 - 'm'		*/0x32, LK_LOWERCASE_M,     LK_UPPERCASE_M,  LK_CTRL_KEYM, 
/* 0x33 - ','		*/0x33, LK_COMMA,           LK_LESSSIGN,     LK_NULL_KEY, 
/* 0x34 - '.'		*/0x34, LK_DOT,             LK_GREATSIGN,    LK_NULL_KEY, 
/* 0x35 - '/'		*/0x35, LK_DIVISION,        LK_QUESTION,     LK_NULL_KEY,   
/* 0x36 - r. SHIFT	*/0x36, SYS_RSHIFT,         SYS_RSHIFT,      SYS_RSHIFT,     /*控制键很特别*/
/* 0x37 - '*'		*/0x37, LK_ASTERISK,        LK_ASTERISK,     LK_ASTERISK,    /* PAD '*' */
/* 0x38 - ALT		*/0x38, SYS_LMENU,          SYS_LMENU,       SYS_LMENU,      /*控制键很特别*/
/* 0x39 - ' '		*/0x39, LK_SPACE,           LK_NULL_KEY,     LK_NULL_KEY,
/* 0x3A - CapsLock	*/0x3A, SYS_CAPITAL,        SYS_CAPITAL,     SYS_CAPITAL,
/* 0x3B - F1		*/0x3B, LK_F1,              LK_F1,           LK_F1,	  	
/* 0x3C - F2		*/0x3C, LK_F2,              LK_F2,           LK_F2,	
/* 0x3D - F3		*/0x3D, LK_F3,	 	    LK_F3,           LK_F3,
/* 0x3E - F4		*/0x3E, LK_F4,              LK_F4,           LK_F4,		
/* 0x3F - F5		*/0x3F, LK_F5,              LK_F5,           LK_F5,		
/* 0x40 - F6		*/0x40, LK_F6,              LK_F6,           LK_F6,		
/* 0x41 - F7		*/0x41, LK_F7,	            LK_F7,           LK_F7, 	
/* 0x42 - F8		*/0x42, LK_F8,	            LK_F8,           LK_F8,	
/* 0x43 - F9		*/0x43, LK_F9,	            LK_F9,           LK_F9,	
/* 0x44 - F10		*/0x44, LK_F10,             LK_F10,          LK_F10,
/* 0x45 - NumLock	*/0x45, SYS_NUMLOCK,        SYS_NUMLOCK,     SYS_NUMLOCK,
/* 0x46 - ScrLock	*/0x46, SYS_SCROLL,         SYS_SCROLL,      SYS_SCROLL,	
/*小键盘特殊处理,不理睬shift, ctrl*/
/* 0x47 - Home		*/0x47, LK_HOME,            LK_SEVEN,        LK_HOME,
/* 0x48 - CurUp		*/0x48, LK_UP,              LK_EIGHT,        LK_UP,
/* 0x49 - PgUp		*/0x49, LK_PAGEUP,          LK_NINE,         LK_PAGEUP,
/* 0x4A - '-'		*/0x4A, LK_SUBTRACTION,     LK_SUBTRACTION,  LK_SUBTRACTION,
/* 0x4B - Left		*/0x4B, LK_LEFT,            LK_FOUR,         LK_LEFT,
/* 0x4C - MID		*/0x4C, LK_NULL_KEY,        LK_FIVE,         LK_NULL_KEY, 
/* 0x4D - Right		*/0x4D, LK_RIGHT,           LK_SIX,          LK_RIGHT,
/* 0x4E - '+'		*/0x4E, LK_PLUSSIGN,        LK_PLUSSIGN,     LK_PLUSSIGN,
/* 0x4F - End		*/0x4F, LK_END,             LK_ONE,          LK_END,
/* 0x50 - Down		*/0x50, LK_DOWN,            LK_TWO,          LK_DOWN,
/* 0x51 - PgDown	*/0x51, LK_PAGEDOWN,        LK_THREE,        LK_PAGEDOWN,
/* 0x52 - Insert	*/0x52, LK_INSERT,          LK_ZERO,         LK_INSERT,
/* 0x53 - Delete	*/0x53, LK_DELETE,          LK_DOT,          LK_DELETE,
/* 0x54 - Enter		*/0x54, LK_NULL_KEY,        LK_NULL_KEY,     LK_NULL_KEY,
/* 0x55 - ???		*/0x55, LK_NULL_KEY,        LK_NULL_KEY,     LK_NULL_KEY,
/* 0x56 - ???		*/0x56, LK_NULL_KEY,        LK_NULL_KEY,     LK_NULL_KEY,
/* 0x57 - F11		*/0x57, LK_F11,             LK_F11,          LK_F11,		
/* 0x58 - F12		*/0x58, LK_F12,             LK_F12,          LK_F12	
},
/* scan code with e0 , that is two code 
 * 该数组中元素没有排序，使用循环遍历方法查找
 */
stKM_Double[] = 
{
/* L GUI    */0x5B,   SYS_LWINDOW,  LK_NULL_KEY,  LK_NULL_KEY, 				
/* R CTRL   */0x1D,   SYS_RCONTROL, SYS_RCONTROL, SYS_RCONTROL,		
/* R GUI    */0x5C,   SYS_RWINDOW,  LK_NULL_KEY,  LK_NULL_KEY, 	
/* R ALT    */0x38,	  SYS_RMENU,    SYS_RMENU,    SYS_RMENU,		
/* APPS     */0x5D,   SYS_APPS,     LK_NULL_KEY,  LK_NULL_KEY,	 							
/* PRTSCRN  */0x2A,   LK_NULL_KEY, LK_NULL_KEY,  LK_NULL_KEY,
              0x37,   SYS_SNAPSHOT,  LK_NULL_KEY,  LK_NULL_KEY,

/* INSERT   */0x52, LK_INSERT,    LK_INSERT,   LK_INSERT, 		
/* HOME     */0x47, LK_HOME,      LK_HOME,     LK_HOME,
/* PG UP    */0x49, LK_PAGEUP,    LK_PAGEUP,   LK_PAGEUP,
/* DELETE   */0x53, LK_DELETE,    LK_DELETE,   LK_DELETE,
/* END      */0x4F, LK_END,       LK_END,      LK_END,
/* PG DN    */0x51, LK_PAGEDOWN,  LK_PAGEDOWN, LK_PAGEDOWN,
/* U ARROW  */0x48, LK_UP,        LK_UP,       LK_UP, 
/* L ARROW  */0x4B, LK_LEFT,      LK_LEFT,     LK_LEFT,
/* D ARROW  */0x50, LK_DOWN,      LK_DOWN,     LK_DOWN,
/* R ARROW  */0x4D, LK_RIGHT,     LK_RIGHT,    LK_RIGHT,
/* KP /     */0x35, LK_DIVISION,  LK_DIVISION, LK_DIVISION,
/* KP EN    */0x1C, LK_RETURN,    LK_RETURN,   LK_RETURN
},

/* begin with e1 */
stKM_Treble[] = 
{
/* PAUSE, 1D,45	,9D,C5 */
   0x1d,  LK_PAUSE, LK_PAUSE,  LK_PAUSE,
   0x45,  LK_NULL_KEY, LK_NULL_KEY, LK_NULL_KEY,
   0x9d,  LK_NULL_KEY, LK_NULL_KEY, LK_NULL_KEY,
   0xc5,  LK_NULL_KEY, LK_NULL_KEY, LK_NULL_KEY
};

/*==========================Appendix====================================
-----------------
ACPI Scan Codes:
-------------------------------------------
Key		Make Code	Break Code
-------------------------------------------
Power		E0, 5E		E0, DE
Sleep		E0, 5F		E0, DF
Wake		E0, 63		E0, E3

-------------------------------
Windows Multimedia Scan Codes:
-------------------------------------------
Key		Make Code	Break Code
-------------------------------------------
Next Track	E0, 19		E0, 99
Previous Track	E0, 10		E0, 90
Stop		E0, 24		E0, A4
Play/Pause	E0, 22		E0, A2
Mute		E0, 20		E0, A0
Volume Up	E0, 30		E0, B0
Volume Down	E0, 2E		E0, AE
Media Select	E0, 6D		E0, ED
E-Mail		E0, 6C		E0, EC
Calculator	E0, 21		E0, A1
My Computer	E0, 6B		E0, EB
WWW Search	E0, 65		E0, E5
WWW Home	E0, 32		E0, B2
WWW Back	E0, 6A		E0, EA
WWW Forward	E0, 69		E0, E9
WWW Stop	E0, 68		E0, E8
WWW Refresh	E0, 67		E0, E7
WWW Favorites	E0, 66		E0, E6
==========================Appendix over====================================*/
#endif /* __HOS_KEYMAP_H*/








