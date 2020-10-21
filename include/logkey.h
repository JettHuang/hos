/*-----------------------------------------------------------
说明我们的主要目的是将物理键盘的scan code 转化成逻辑键盘的Lk code
，然后我们的程序将看到的是Lk code.为什么要这样做呢?因为scan code 的
字节大小等(1,2,4)，而我们的Lk code 均是1 byte.
转换的规则：
1,将可显示的按键转换成Ascii表中的编码后发给逻辑键盘,如：
  shift + 字符按键 --> 大小写字母, 符号...
  ctrl  + a(z) --> 0x01(0x1A)
2,不传递shift, ctrl ,alt, windows key这些控制按键给逻辑键盘.
3,主键盘或小键盘上的数字键均转换成Ascii中的数字编码,发给逻辑键盘.
4,Caps Lock, Num Lock, Scroll Lock, PrintScreen由系统处理.
5,PageUp, PageDown, Home, End, 方向键(上、下、左、右), Insert,
  Delete, F1--F12, Esc, Pause编码后发给逻辑键盘.
6,Power, Sleep, WakeUp系统处理.
7,其它的按键以后再扩充
								huangheshui 	05/17/07 22:28
-----------------------------------------------------------*/
/*Logical Key code set*/
#ifndef __LOG_KEYBOARD_H
#define __LOG_KEYBOARD_H

#define LK_NULL_KEY               0x00      /*null key*/
/*ASCII中的编码01~26, ctrl + a~z*/
#define LK_CTRL_KEYA              0x01
#define LK_CTRL_KEYB              0x02
#define LK_CTRL_KEYC              0x03
#define LK_CTRL_KEYD              0x04
#define LK_CTRL_KEYE              0x05
#define LK_CTRL_KEYF              0x06
#define LK_CTRL_KEYG              0x07
#define LK_CTRL_KEYH              0x08
#define LK_CTRL_KEYI              0x09
#define LK_CTRL_KEYJ              0x0a
#define LK_CTRL_KEYK              0x0b
#define LK_CTRL_KEYL              0x0c
#define LK_CTRL_KEYM              0x0d
#define LK_CTRL_KEYN              0x0e
#define LK_CTRL_KEYO              0x0f
#define LK_CTRL_KEYP              0x10
#define LK_CTRL_KEYQ              0x11
#define LK_CTRL_KEYR              0x12
#define LK_CTRL_KEYS              0x13
#define LK_CTRL_KEYT              0x14
#define LK_CTRL_KEYU              0x15
#define LK_CTRL_KEYV              0x16
#define LK_CTRL_KEYW              0x17
#define LK_CTRL_KEYX              0x18
#define LK_CTRL_KEYY              0x19
#define LK_CTRL_KEYZ              0x1a
/*0x1c~0x0x1f未定义*/

#define LK_BACKSPACE              0x1b    /*back space*/
/*ASCII中的编码0x20~0x7E*/
#define LK_SPACE		          0x20    /* space*/
#define LK_EXCALMATORY            0x21    /*!*/
#define LK_DQUOTATION             0x22    /*"*/
#define LK_WELL                   0x23    /*#*/
#define LK_DOLLAR                 0x24    /*$*/
#define LK_MODULE                 0x25    /*%*/
#define LK_AND                    0x26    /*&*/
#define LK_SQUOTATION             0x27    /*'*/
#define LK_LROUND_BRAKET          0x28    /*( left round braket*/
#define LK_RROUND_BRAKET          0x29    /*) right round braket*/
#define LK_ASTERISK               0x2a    /** asterisk*/
#define LK_PLUSSIGN               0x2b    /*+ plus sign*/
#define LK_COMMA                  0x2c    /*, comma*/
#define LK_SUBTRACTION            0x2d    /*- subtraction sign*/  
#define LK_DOT                    0x2e    /*. dot*/
#define LK_DIVISION               0x2f    /*/ division sign*/
#define LK_ZERO                   0x30    /*0 zero*/
#define LK_ONE                    0x31    
#define LK_TWO                    0x32
#define LK_THREE                  0x33
#define LK_FOUR                   0x34
#define LK_FIVE                   0x35
#define LK_SIX                    0x36
#define LK_SEVEN                  0x37
#define LK_EIGHT                  0x38
#define LK_NINE                   0x39
#define LK_COLON                  0x3a    /*: colon*/
#define LK_SEMICOLON              0x3b    /*; semicolon*/
#define LK_LESSSIGN               0x3c    /*< less sign*/            
#define LK_EQUALSIGN              0x3d    /*= equal sign*/
#define LK_GREATSIGN              0x3e    /*> great sign*/
#define LK_QUESTION               0x3f    /*? question mark*/               
#define LK_AT                     0x40    /*@ at mark*/
/*ASCII中大写A~Z的按键编码*/
#define LK_UPPERCASE_A            0x41
#define LK_UPPERCASE_B            0x42
#define LK_UPPERCASE_C            0x43
#define LK_UPPERCASE_D            0x44
#define LK_UPPERCASE_E            0x45
#define LK_UPPERCASE_F            0x46
#define LK_UPPERCASE_G            0x47
#define LK_UPPERCASE_H            0x48
#define LK_UPPERCASE_I            0x49
#define LK_UPPERCASE_J            0x4a
#define LK_UPPERCASE_K            0x4b
#define LK_UPPERCASE_L            0x4c
#define LK_UPPERCASE_M            0x4d
#define LK_UPPERCASE_N            0x4e
#define LK_UPPERCASE_O            0x4f
#define LK_UPPERCASE_P            0x50
#define LK_UPPERCASE_Q            0x51
#define LK_UPPERCASE_R            0x52
#define LK_UPPERCASE_S            0x53
#define LK_UPPERCASE_T            0x54
#define LK_UPPERCASE_U            0x55
#define LK_UPPERCASE_V            0x56
#define LK_UPPERCASE_W            0x57
#define LK_UPPERCASE_X            0x58
#define LK_UPPERCASE_Y            0x59
#define LK_UPPERCASE_Z            0x5a

#define LK_LSQUARE_BRACKET        0x5b    /*[ left square bracket*/
#define LK_BACKSLASH              0x5c    /*\ backslash sign*/
#define LK_RSQUARE_BRACKET        0x5d    /*] right square bracket*/
#define LK_XOR                    0x5e    /*^ xor sign*/
#define LK_BASELINE               0x5f    /*_ base line*/
#define LK_HEXSIXTY               0x60    /*` 我不知道怎样称呼这个符号,tab的上面*/
/*ASCII中小写a~z的按键编码*/
#define LK_LOWERCASE_A            0x61
#define LK_LOWERCASE_B            0x62
#define LK_LOWERCASE_C            0x63
#define LK_LOWERCASE_D            0x64
#define LK_LOWERCASE_E            0x65
#define LK_LOWERCASE_F            0x66
#define LK_LOWERCASE_G            0x67
#define LK_LOWERCASE_H            0x68
#define LK_LOWERCASE_I            0x69
#define LK_LOWERCASE_J            0x6a
#define LK_LOWERCASE_K            0x6b
#define LK_LOWERCASE_L            0x6c
#define LK_LOWERCASE_M            0x6d
#define LK_LOWERCASE_N            0x6e
#define LK_LOWERCASE_O            0x6f
#define LK_LOWERCASE_P            0x70
#define LK_LOWERCASE_Q            0x71
#define LK_LOWERCASE_R            0x72
#define LK_LOWERCASE_S            0x73
#define LK_LOWERCASE_T            0x74
#define LK_LOWERCASE_U            0x75
#define LK_LOWERCASE_V            0x76
#define LK_LOWERCASE_W            0x77
#define LK_LOWERCASE_X            0x78
#define LK_LOWERCASE_Y            0x79
#define LK_LOWERCASE_Z            0x7a

#define LK_LBRACE                 0x7b    /*{ left brace*/
#define LK_OR                     0x7c    /*| or sign*/
#define LK_RBRACE                 0x7d    /*} right brace*/
#define LK_REVERSAL               0x7e    /*~ reversal */
/*ASCII中的0x7f未定义*/

/* expanded Ascii as our key for some control using:
 * PageUp, PageDown, Home, End, 方向键(上、下、左、右), Insert,
 * Delete, F1--F12, Esc.
 */
#define LK_TABLE                   0x80    /*table*/
#define LK_PAUSE                   0x81    /*Pause*/
#define LK_ESCAPE                  0x82    /*Esc*/
#define LK_PAGEUP                  0x83    /*Page Up*/
#define LK_PAGEDOWN                0x84    /*Page Down*/
#define LK_END                     0x85    /*End*/
#define LK_HOME                    0x86    /*Home*/
#define LK_INSERT                  0x87    /*Insert*/
#define LK_DELETE                  0x88    /*Delete*/  
#define LK_LEFT                    0x89    /*左箭头*/
#define LK_UP                      0x8a    /*上箭头*/
#define LK_RIGHT                   0x8b    /*右箭头*/
#define LK_DOWN                    0x8c    /*下箭头*/
#define LK_F1                      0x8d    /*功能键F1到F12*/
#define LK_F2                      0x8e
#define LK_F3                      0x8f
#define LK_F4                      0x90
#define LK_F5                      0x91
#define LK_F6                      0x92
#define LK_F7                      0x93
#define LK_F8                      0x94
#define LK_F9                      0x95
#define LK_F10                     0x96
#define LK_F11                     0x97
#define LK_F12                     0x98
#define LK_RETURN                  0x99    /*Enter*/                  
/*系统监视的物理按键编码*/
#define SYS_SNAPSHOT               0xa0    /*Print Screen*/
#define SYS_NUMLOCK                0xa1	   /*Num Lock*/
#define SYS_SCROLL                 0xa2    /*Scroll Lock*/
#define SYS_CAPITAL                0xa3    /*Caps Lock*/
#define SYS_LSHIFT                 0xa4    /*left shift*/
#define SYS_RSHIFT                 0xa5
#define SYS_LCONTROL               0xa6    /*left control*/
#define SYS_RCONTROL               0xa7
#define SYS_LMENU                  0xa8    /*left alt*/
#define SYS_RMENU                  0xa9
#define SYS_APPS                   0xaa    /*application, at left of r-ctrl*/
#define SYS_LWINDOW                0xab    /* left window */
#define SYS_RWINDOW                0xac

#define SYS_POWER                  0xb0    /*power*/
#define SYS_SLEEP                  0xb1    /*sleep*/
#define SYS_WAKEUP                 0xb2    /*wake up*/

#endif  /*end __LOG_KEYBOARD_H*/