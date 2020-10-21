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
     ks_lShfit,			/* һЩϵͳ���Ƽ���״̬*/
	 ks_rShfit,
	 ks_lCtrl,
	 ks_rCtrl,
	 ks_lAlt,
	 ks_rAlt,
	 ks_NumLock,
	 ks_ScrollLock,
	 ks_CapsLock;
enum LedStatus{OFF = 0, ON}
	 ls_NumLock,        /* ������3��led��״̬*/
	 ls_CapsLock,
	 ls_ScrollLock;

/*----------------������������ʹ����Tinix���뷽��--------------------------*/
/*=======================================================*
 * �ȴ� 8042 �����뻺������
 *=======================================================*/
void kb_wait()	
{
	unsigned char kb_stat;

	do{
		In_Byte(KB_CMD_PORT, kb_stat)
	}while(kb_stat & 0x02); /*����8042����(cpu-->8042)������*/
}


/*=======================================================*
 * ��ȡ���̱�־��(read 2 bytes), AT���̷�����Ӧ��0xfa
 *=======================================================*/
void kb_ack()
{
	unsigned char kb_read;

	do{
		In_Byte(KB_DATA_PORT, kb_read)
	}while(kb_read != KB_ACK); /*��Ӧ�����*/
}


/*=======================================================*
 * ���ü����ϵ�leds, ���ҳԾ�����kb_ack()֮��,����KB_ACK!
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
/*----------------������������ʹ����Tinix���뷽��--------------------------*/


/*=======================================================*
 *F: ��ʼ������
 *I: void
 *O: void
 *=======================================================*/
void fnInitKeyboard()
{	
	/* һЩϵͳ���Ƽ���״̬*/
	 ks_lShfit = UP;
	 ks_rShfit = UP;
	 ks_lCtrl = UP;
	 ks_rCtrl = UP;
	 ks_lAlt = UP;
	 ks_rAlt = UP;
	 ks_NumLock = UP;
	 ks_ScrollLock = UP;
	 ks_CapsLock = UP;
	/* ������3��led��״̬*/
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
    static unsigned char scan_prefix, count = 0; /*��ǰɨ�����ǰ׺(0, 0xE0, 0xE1), ���յ�һ������ɨ�������*/
	int i, flagbreak = 0;
    
	In_Byte(KB_DATA_PORT, scan_code)
/* ����ɨ����,����caps lock, scroll lock, Num Lock, L(R)_shift,
 * L(R)_Ctrl, L(R)_Alt ����������break���� 
 */
	if(scan_code == 0xE0 || scan_code == 0xE1)
	{
		scan_prefix = scan_code;
		return;
	}
	switch(scan_prefix)
	{
/* һ������������make | break �� e0,xx ..*/
	case 0xE0:	
		/* �ж��Ƿ�Ϊbreak */
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
			lk_code = stKM_Double[i].nature_lkey;   /* ����break��дnature_lkey */
		else if(ks_lShfit == DOWN || ks_rShfit == DOWN)
			lk_code = stKM_Double[i].shift_lkey;
		else if(ks_lCtrl == DOWN || ks_rCtrl == DOWN)
			lk_code = stKM_Double[i].ctrl_lkey;
		else
            lk_code = stKM_Double[i].nature_lkey;
		break;
/* һ������������make | break �� e1,xx, ..�����������һ��pause,û��break */
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
/* һ������������make | break �� xx */
    default:    
		if(scan_code > 0xe0) 
			return;
		if(scan_code >= NUM_SINGLE)
		{
		    flagbreak = 1; /* is break */
			scan_code -= 0x80;
		}
		/*��scan_code��Ϊ����
		 *shfit's priority level is bigger than ctrl
		 */
		if(flagbreak)
			lk_code = stKM_Single[scan_code].nature_lkey;   /* ����break��дnature_lkey */
		/*С���� PA0 - PA9*/
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
/* �ж�lk_code�Ƿ�Ϊϵͳ�� */
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
			if(ks_NumLock == UP) /* ��һ��NumLockû�а��� */
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
		break;	/* ����*/
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
	default:                             /* һ�㰴�� */
		if(flagbreak) return;
		/* �������ĸ������Ҫ��CapsLock */
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


















