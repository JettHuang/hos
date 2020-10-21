/*-----------------------------------------
*AT keyboard driver
*huangheshui	07/05/14    12:46
*8048 ---> 8042 ---> 8259A
*-----------------------------------------*/
#include  "..\include\asm.h"
#include  "..\include\color.h"
#include  "..\include\logkey.h"
#include  "..\include\keyboard.h"
#include  "..\include\tty.h"

extern void printk(unsigned long color, const char *lpstr);
extern int InputKeytoTTY(struct TTY *t, char lkey);
extern struct TTY *glbTTYSelected;

#define	KB_DATA_PORT	0x60
#define	KB_CMD_PORT		0x64
#define	LED_CODE	    0xED
#define	KB_ACK		    0xFA

enum KeyStatus{UP = 0, DOWN}
     ks_lShfit,			/* 一些系统控制键的状态*/
	 ks_rShfit,
	 ks_lCtrl,
	 ks_rCtrl,
	 ks_lAlt,
	 ks_rAlt,
	 ks_NumLock,
	 ks_ScrollLock,
	 ks_CapsLock;
enum LedStatus{OFF = 0, ON}
	 ls_NumLock,        /* 键盘上3个led的状态*/
	 ls_CapsLock,
	 ls_ScrollLock;

/*----------------以下三个函数使用了Tinix代码方法--------------------------*/
/*=======================================================*
 * 等待 8042 的输入缓冲区空
 *=======================================================*/
void kb_wait()	
{
	unsigned char kb_stat;

	do{
		In_Byte(KB_CMD_PORT, kb_stat)
	}while(kb_stat & 0x02); /*测试8042输入(cpu-->8042)缓冲区*/
}


/*=======================================================*
 * 读取键盘标志号(read 2 bytes), AT键盘返回响应码0xfa
 *=======================================================*/
void kb_ack()
{
	unsigned char kb_read;

	do{
		In_Byte(KB_DATA_PORT, kb_read)
	}while(kb_read != KB_ACK); /*响应码测试*/
}


/*=======================================================*
 * 设置键盘上的leds, 令我吃惊的是kb_ack()之后,还有KB_ACK!
 *=======================================================*/
void set_leds()
{
	unsigned char leds = (unsigned char)((ls_CapsLock << 2) | (ls_NumLock << 1) | ls_ScrollLock);

	kb_wait();
	Out_Byte(KB_DATA_PORT, LED_CODE)
	kb_ack();	/*read acknowledge code*/
	kb_wait();
	Out_Byte(KB_DATA_PORT, leds)
	kb_ack();  /*read acknowledge code*/
}
/*----------------以上三个函数使用了Tinix代码方法--------------------------*/


/*=======================================================*
 *F: 初始化键盘
 *I: void
 *O: void
 *=======================================================*/
void fnInitKeyboard()
{	
	/* 一些系统控制键的状态*/
	 ks_lShfit = UP;
	 ks_rShfit = UP;
	 ks_lCtrl = UP;
	 ks_rCtrl = UP;
	 ks_lAlt = UP;
	 ks_rAlt = UP;
	 ks_NumLock = UP;
	 ks_ScrollLock = UP;
	 ks_CapsLock = UP;
	/* 键盘上3个led的状态*/
	 ls_NumLock = ON;      
	 ls_CapsLock = OFF;
	 ls_ScrollLock = OFF;
     set_leds();
}


/*=======================================================*
 *F: keyboard interrupt handler
 *I: void
 *O: void
 *=======================================================*/
void fndo_keyboard()
{
	unsigned char scan_code, lk_code;
    static unsigned char scan_prefix, count = 0; /*当前扫描码的前缀(0, 0xE0, 0xE1), 已收到一个按键扫描码个数*/
	int i, flagbreak = 0;
    
	In_Byte(KB_DATA_PORT, scan_code)
/* 解析扫描码,除了caps lock, scroll lock, Num Lock, L(R)_shift,
 * L(R)_Ctrl, L(R)_Alt 其它按键的break忽略 
 */
	if(scan_code == 0xE0 || scan_code == 0xE1)
	{
		scan_prefix = scan_code;
		return;
	}
	switch(scan_prefix)
	{
/* 一个按键产生的make | break 是 e0,xx ..*/
	case 0xE0:	
		/* 判断是否为break */
		if(scan_code > 0x5D)
		{
		    flagbreak = 1;  
			scan_code -= 0x80;
		}
		for(i = 0; i < NUM_DOUBLE; i++)
			if(stKM_Double[i].make_code == scan_code)
				break;
		scan_prefix = 0; /* clear e0 flag */
		if(i == NUM_DOUBLE)
		{
			printk(RED, "warning: scan_code is not find in stKM_Double[]\n");
			return;
		}
		/*shfit's priority level is bigger than ctrl*/
		if(flagbreak)
			lk_code = stKM_Double[i].nature_lkey;   /* 对于break就写nature_lkey */
		else if(ks_lShfit == DOWN || ks_rShfit == DOWN)
			lk_code = stKM_Double[i].shift_lkey;
		else if(ks_lCtrl == DOWN || ks_rCtrl == DOWN)
			lk_code = stKM_Double[i].ctrl_lkey;
		else
            lk_code = stKM_Double[i].nature_lkey;
		break;
/* 一个按键产生的make | break 是 e1,xx, ..我们这里仅有一个pause,没有break */
	case 0xE1: 
        for(i = 0; i < NUM_TREBLE; i++)
			if(stKM_Treble[i].make_code == scan_code)
				break;
		count++;
		if(count == 2)
			scan_prefix = count = 0; /* clear e1 flag */
		if(i == NUM_TREBLE)
		{
			printk(RED, "warning: scan_code is not find in stKM_Treble[]\n");
			return;
		}
		lk_code = stKM_Treble[i].nature_lkey;
		break;
/* 一个按键产生的make | break 是 xx */
    default:    
		if(scan_code > 0xe0) 
			return;
		if(scan_code >= NUM_SINGLE)
		{
		    flagbreak = 1; /* is break */
			scan_code -= 0x80;
		}
		/*以scan_code作为索引
		 *shfit's priority level is bigger than ctrl
		 */
		if(flagbreak)
			lk_code = stKM_Single[scan_code].nature_lkey;   /* 对于break就写nature_lkey */
		/*小键盘 PA0 - PA9*/
		else if((scan_code >= 0x4f && scan_code <= 0x53) || 
			    (scan_code >= 0x4b && scan_code <= 0x4d) || 
				(scan_code >= 0x47 && scan_code <= 0x49))
		{
			if(ls_NumLock == ON)
				lk_code = stKM_Single[scan_code].shift_lkey;
			else
				lk_code = stKM_Single[scan_code].nature_lkey;
		}
		else if(ks_lShfit == DOWN || ks_rShfit == DOWN)
			lk_code = stKM_Single[scan_code].shift_lkey;
		else if(ks_lCtrl == DOWN || ks_rCtrl == DOWN)
			lk_code = stKM_Single[scan_code].ctrl_lkey;
		else
            lk_code = stKM_Single[scan_code].nature_lkey;
	}
/* 判断lk_code是否为系统键 */
	if(lk_code == LK_NULL_KEY)
		return;
	switch(lk_code)
	{
	case SYS_SNAPSHOT:
		if(flagbreak) return;
		printk(BLUE, "SYS_SNAPSHOT\n");
		break;
    case SYS_NUMLOCK:
		if(!flagbreak) /* make */
		{
			if(ks_NumLock == UP) /* 上一次NumLock没有按下 */
			{
				ls_NumLock = (ls_NumLock == ON) ? OFF : ON;
				ks_NumLock = DOWN;
				set_leds();
			}			
		}
		else /* break code */
		{
			ks_NumLock = UP;
		}
		break;
    case SYS_SCROLL:                    /*Scroll Lock*/
		if(!flagbreak) /* make */
		{
			if(ks_ScrollLock == UP)
			{
				ls_ScrollLock = (ls_ScrollLock == ON) ? OFF : ON;
				ks_ScrollLock = DOWN;
				set_leds();
			}			
		}
		else /* break code */
		{
			ks_ScrollLock = UP;
		}
		break;
    case SYS_CAPITAL:                    /*Caps Lock*/
		if(!flagbreak) /* make */
		{
			if(ks_CapsLock == UP)
			{
				ls_CapsLock = (ls_CapsLock == ON) ? OFF : ON;
				ks_CapsLock = DOWN;
				set_leds();
			}			
		}
		else /* break code */
		{
			ks_CapsLock = UP;
		}
		break;
    case SYS_LSHIFT:                     /*left shift*/
		if(flagbreak)
		{
			ks_lShfit = UP;
		}
		else
			ks_lShfit = DOWN;
		break;
    case SYS_RSHIFT:  
		if(flagbreak)
		{
			ks_rShfit = UP;
		}
		else
			ks_rShfit = DOWN;
		break;
    case SYS_LCONTROL:                   /*left control*/
		if(flagbreak)
			ks_lCtrl = UP;
		else
			ks_lCtrl = DOWN;
		break;
    case SYS_RCONTROL: 
		if(flagbreak)
			ks_rCtrl = UP;
		else
			ks_rCtrl = DOWN;
		break;
    case SYS_LMENU:                      /*left alt*/
    case SYS_RMENU:  
		DisplayTTY(glbTTYSelected);
    case SYS_APPS:                       /*application, at left of r-ctrl*/
		break;	/* 抛弃*/
    case SYS_LWINDOW:                    /*left window*/
		if(flagbreak) return;
        break;
    case SYS_RWINDOW:  
		if(flagbreak) return;
        break;
	case LK_PAGEDOWN:
		if(flagbreak) return;
		break;
	case LK_PAGEUP:
		if(flagbreak) return;
		break;
	default:                             /* 一般按键 */
		if(flagbreak) return;
		/* 如果是字母键还需要看CapsLock */
		if(ls_CapsLock == ON) 
		{
		   if(lk_code >= LK_UPPERCASE_A && lk_code <= LK_UPPERCASE_Z)
				lk_code |= 0x20;
		   else if(lk_code >= LK_LOWERCASE_A && lk_code <= LK_LOWERCASE_Z)
				lk_code &= 0xdf;	
		}
		InputKeytoTTY(glbTTYSelected, lk_code);
	}	
}


















