/*----------------------------------------------------------------*
 *˵����   ��ģ����ʵ���˲���TYY����̨�ĺ���, �ɲο�bmp�ļ�����ͼ
 *     tty.bmp�������Ĺ��췽����
 *     ֻ�����ڱ��û�ѡ�е�tty���ܹ�ӵ��������̺���ʾ��Ļ��ʹ��Ȩ
 *
 *
 *						       huangheshui   2007-06-20 15:20
 *----------------------------------------------------------------*/
#include "..\include\logkey.h"
#include "..\include\tty.h"
#include "..\include\color.h"

extern void fnWakeUp(struct PCB * *pwait);
extern void fnSleepOn(struct PCB * *pwait);
extern void * fnGet_VmkPage();
extern void * fnVmk_alloc(unsigned int length);
extern void fnVmk_free(void *lpblock, unsigned int length);
extern void fnFree_VmkPage(void * addrpage);

void WriteTTY(struct TTY *t, char *pszstr, int len);

//ȫ�ֱ�������ѡ�е�ttyָ��
struct TTY *glbTTYSelected;
 
/*======================================================*
 *F: Ϊĳ���̴���һ��tty����.
 *I: p-���̵�PCBָ��
 *O: 0-ʧ��, 1-�ɹ�.
 *C: ����ý����Ѿ���TTY�����ٸ�������.
 *======================================================*/
int CreateTTY(struct PCB *p)
{
	struct TTY *t;

	if(!p || p->ptty) 
		return 0;
    if(!(t = (struct TTY *)fnVmk_alloc(sizeof(struct TTY))))
		return 0;
	//��ʼ����tty����
	t->kb.front = t->kb.rear = 0;
	t->kb.imode = KB_NOTSET;
	//������̶��л�����
	if(!(t->kb.iqueue = (char *)fnVmk_alloc(KB_QUEUELEN)))
	{
		fnVmk_free(t, sizeof(struct TTY));
		return 0;
	}
	//��ʼ����Ļ��ʾ�ṹ
	t->vd.color = (unsigned char)(BLACK * 16 + LIGHTGREEN);
	t->vd.textfront = t->vd.textrear = 0;
	t->vd.vsline = 0;
	t->vd.chws = 0;   //�Ѿ�׷�ӵ��ַ�����
	t->vd.oldrear = 0;
	t->vd.bkflg = 0;
	t->vd.csrchs = 0; //��Ļ���ֵķ�����
	//�����ı�������
	if(!(t->vd.TextBuf = (char *)fnGet_VmkPage()))
	{
		fnVmk_free(t->kb.iqueue, KB_QUEUELEN);
		fnVmk_free(t, sizeof(struct TTY));
		return 0;
	}
	t->wait = 0;
	p->ptty = t;
	glbTTYSelected = t;
	return 1;
}


/*======================================================*
 *F: ��ĳ���̵�tty��������.
 *I: p-���̵�PCBָ��
 *O: void
 *C: ����ý���û��TTY��������,Ҳ����ʾ.
 *======================================================*/
void DestroyTTY(struct PCB *p)
{
	struct TTY *t;

	if(!p || !(p->ptty)) 
		return;
    t = p->ptty;
	//�ͷż��̻������
	if(t->kb.iqueue)
		fnVmk_free(t->kb.iqueue, KB_QUEUELEN);
	//�ͷ��ı�������
	if(t->vd.TextBuf)
		fnFree_VmkPage((void *)(t->vd.TextBuf));
	t->wait = 0;
	fnVmk_free(t, sizeof(struct TTY));
}


/*======================================================*
 *F: ��tty�ļ��̻������β�����һ������.
 *I: t-TTY����ָ��, lkey-�������߼���
 *O: 0-ʧ��, !0-�ɹ�
 *C: ���ﻹ���ͬ��ģʽλ��������Ҫ���ܻ��ѵȴ������Ľ���.
 *======================================================*/
int InputKeytoTTY(struct TTY *t, char lkey)
{
	char szbuf;

	if(!t) 
		return 0;
	if(t->kb.front == ((t->kb.rear + 1) % KB_QUEUELEN))
		return 0;  //��������,�������ø�����
	t->kb.iqueue[t->kb.rear++] = lkey;
	//���ͬ����־
	if((t->kb.imode & KB_SYNC))
		fnWakeUp(&(t->wait));  //���ѵȴ������Ľ���
	//��д����Ļ
	szbuf = lkey;
	WriteTTY(t, &szbuf, 1);
	return 1;
}


/*======================================================*
 *F: ��tty������һЩ���밴��.
 *I: t-TTY����ָ��, lkey-�������߼���
 *   kybuf-���հ����Ļ�����
 *   kynum-��mode��KB_WAKEUP_NUMS��Чʱָ��������; 
 *         KB_WAKEUP_CHAR��Чʱֻkybuf��С��
 *   mode-��ȡģʽ,��ͬʱΪKB_WAKEUP_NUMS | KB_WAKEUP_CHAR
 *        ����ȡֵKB_SYNC��KB_ASYNC
 *   kywake- ��KB_WAKEUP_CHAR��Чʱ�����ѱ�־����.
 *O: ��õ��ַ�����.
 *C: ��������mode,������Ҫ˯�ߡ�����sizeof(char)
 *======================================================*/
int ReadTTY(struct TTY *t, char *kybuf, int kynum, int mode, char kywake)
{
	int len;

	if(!t || !kynum) 
		return 0;
	if(kynum >= KB_QUEUELEN)
		kynum = KB_QUEUELEN - 1;  //���ֻ�ܶ�(KB_QUEUELEN - 1)������
_repeat:
	len = (t->kb.rear - t->kb.front + KB_QUEUELEN) % KB_QUEUELEN;
	//���Լ��̶��а����Ƿ���
	if(len < kynum)
	{
		if(mode & KB_SYNC)  //��Ҫͬ����
		{
			if(len && (mode & KB_WAKEUP_CHAR))  //����kywake��
			{
				if(kywake == t->kb.iqueue[t->kb.rear - 1])
				{
					kynum = len;  //�ö�ȡ�ַ�������С->len
					goto _getkeys;
				}
			}
			//���������KB_WAKEUP_NUMS�ˣ�������ΪĬ�������
			t->kb.imode = KB_SYNC;
			fnSleepOn(&(t->wait));
			goto _repeat;
		}
		//�첽�������ֱ������ִ��
	}
_getkeys:
	//�����̶����еİ���ȡ��-->kybuf
	len = kynum;
	for(;kynum; kynum--)
	{
		*kybuf++ = t->kb.iqueue[t->kb.front++];
	}
	return len;
}


/*======================================================*
 *F: ��������ʾ��Ļ����ˢ��.
 *I: color-ˢ����ɫ
 *O: void
 *======================================================*/
void PaintScreen(int color)
{
	union{
		unsigned short sh;
		struct{
			char ch;
			unsigned char clr;
		}dchar;
	}show;
	unsigned short *vaddr;

	show.dchar.clr = (unsigned char)((color << 4) + color);
	show.dchar.ch = ' ';
    for(vaddr = (unsigned short *)PHYSIC_VIDEO_BUFFER; 
	    vaddr < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS;
		vaddr++)
	{
		*vaddr = show.sh;
	}
}


/*==============================================================*
 *F: ��������Ļ�Ͼ���
 *I: rows-�Ͼ�����, color-ˢ����ɫ(ˢ���ɺ�).
 *O: void
 *C: 
 *==============================================================*/
void ScrollUpScreen(int rows, int color)
{
	union{
		unsigned short sh;
		struct{
			char ch;
			unsigned char clr;
		}dchar;
	}show;
	unsigned short *vdest, *vsour;

	if(rows <= 0)
		return;
	if(rows >= SCREEN_ROWS)
	{
		PaintScreen(color);
		return;
	}
	show.dchar.clr = (unsigned char)((color << 4) + color);
	show.dchar.ch = ' ';
	//���� 0 < rows < SCREEN_ROWS���
	vsour = (unsigned short *)PHYSIC_VIDEO_BUFFER + rows * SCREEN_COLUMNS;
    for(vdest = (unsigned short *)PHYSIC_VIDEO_BUFFER; 
	   vsour < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS; )
	{
		*vdest++ = *vsour++;
	}
	//ˢ���ɺۼ�
	 for(; vdest < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS; )
	{
		*vdest++ = show.sh;
	}
}


/*======================================================*
 *F: ��tty������ı�һ����ͼ��ʾ����Ļ�ϡ�
 *I: t-TTY����ָ�� 
 *O: void
 *C: ��ʾ��viewoffset��ʼ��textrear֮����ַ� && ����25��.
 *   ����TEXT�ж����Ѿ���������ַ�(����ʾ��)
 *======================================================*/
void DisplayTTY(struct TTY *t)
{
	int front, wchs;
	union{
		unsigned short sh;
		struct{
			char ch;
			unsigned char clr;
		}dchar;
	}show;
	unsigned short *vdest;

	if(!t)
		return;
	if(t->vd.textfront == t->vd.textrear)
		return;
	front = (t->vd.vsline * SCREEN_COLUMNS + t->vd.textfront) % TEXTBUF_SIZE;
	vdest = (unsigned short *)PHYSIC_VIDEO_BUFFER;
	show.dchar.clr = t->vd.color;
	wchs = 0;
	while(front != t->vd.textrear)
	{
		show.dchar.ch = t->vd.TextBuf[front];
		*vdest++ = show.sh;
		front = (front + 1) % TEXTBUF_SIZE;
		if(++wchs >= SCREEN_ROWS * SCREEN_COLUMNS)  //�Ѿ�д��һ����Ļ
			break;
	}
	//ˢ�������Ļ����
	show.dchar.ch = ' ';
	for(; vdest < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS; )
	{
		*vdest++ = show.sh;
	}
}


/*======================================================*
 *F: ��tty����д��һ���ı�(׷��)��
 *I: t-TTY����ָ��, pszstr-�ַ���, len-�ַ�����
 *O: void
 *C: ��len==-1�����ַ�����'\0'��β.
 *======================================================*/
void WriteTTY(struct TTY *t, char *pszstr, int len)
{
    int num, textchs, oldtmp, bknum, oncewchs; //�˴κ�������д����ַ�����
	int refresh;  //ˢ����־
	union{
		unsigned short sh;
		struct{
			char ch;
			unsigned char clr;
		}dchar;
	}show;
	unsigned short *vdest;
	
	if(!pszstr)
		return;
	if(t->vd.chws == 0)
		t->vd.oldrear = t->vd.textrear;
	bknum = 0; //�����ַ�����
	oncewchs = 0;
	refresh = 0;
	for(; len && *pszstr != '\0'; len--, pszstr++)
	{
		//�Ƿ�TEXT����,��������ǰ��һ��
		if((t->vd.textrear + 1) % TEXTBUF_SIZE == t->vd.textfront)
		{
			t->vd.textfront = (t->vd.textfront + SCREEN_COLUMNS) % TEXTBUF_SIZE;
			refresh = 1;
		}
		switch((unsigned char)(*pszstr))
		{
		case LK_TABLE:  //�Ʊ��
			 for(num = 0; num < 8; num++)
			 {
				//�Ƿ�TEXT����,��������ǰ��һ��
				if((t->vd.textrear + 1) % TEXTBUF_SIZE == t->vd.textfront)
				{
					t->vd.textfront = (t->vd.textfront + SCREEN_COLUMNS) % TEXTBUF_SIZE;
					refresh = 1;
				}
				t->vd.TextBuf[t->vd.textrear] = LK_SPACE;
				t->vd.textrear = (t->vd.textrear + 1) % TEXTBUF_SIZE;
				t->vd.chws++;
				oncewchs++;
			 }
			 break;
		case LK_CTRL_KEYJ: //���м�
			 //fall throught
		case LK_RETURN: //�س���
			 while(t->vd.chws == 0 || (t->vd.chws % SCREEN_COLUMNS) != 0)
			 {
				t->vd.TextBuf[t->vd.textrear] = LK_SPACE;
				t->vd.textrear = (t->vd.textrear + 1) % TEXTBUF_SIZE;
				t->vd.chws++;
				oncewchs++;
			 }
			 t->vd.chws = 0;  //�������ѳɶ���			 
			 break;
		case LK_BACKSPACE: //���˼�
			 if(t->vd.chws)
			 {
				t->vd.textrear = (t->vd.textrear - 1 + TEXTBUF_SIZE) % TEXTBUF_SIZE;
				t->vd.TextBuf[t->vd.textrear] = LK_SPACE;
				t->vd.chws--;
				bknum++;
				oncewchs--; //��Ļ�ϵ��ַ�����һ��
			 }
			 t->vd.bkflg = 1;
			 break;
        default:
			t->vd.TextBuf[t->vd.textrear] = *pszstr;
			t->vd.textrear = (t->vd.textrear + 1) % TEXTBUF_SIZE;
			t->vd.chws++;
			oncewchs++;
		}
	}
	
	//��ʼ����TEXT����ʾ
    textchs = (t->vd.textrear - t->vd.textfront + TEXTBUF_SIZE) % TEXTBUF_SIZE;
	//����vsline,��ʱ����len
	len = textchs / SCREEN_COLUMNS - SCREEN_ROWS + 1;
	if((len >= t->vd.vsline || t->vd.bkflg == 0) 
		&& len > 0)
	{
		t->vd.vsline = len;
		t->vd.bkflg = 0;
	}
	oncewchs += t->vd.csrchs;  //��ʱ��Ļ�ϵ��ַ��ܸ���(���ܱ�ʵ����Ļ��)
	t->vd.csrchs = textchs - t->vd.vsline * SCREEN_COLUMNS;  //��ʱдTTY����Ļ�ϵ��ַ�����
	//�����ǲ�ˢ�����
	if(t->vd.csrchs < SCREEN_ROWS * SCREEN_COLUMNS 
		&& oncewchs < SCREEN_ROWS * SCREEN_COLUMNS)
	{
		//t->vd.oldrear��ʼ��ʾ
		vdest = (unsigned short *)PHYSIC_VIDEO_BUFFER + 
			(t->vd.oldrear - t->vd.textfront + TEXTBUF_SIZE) % TEXTBUF_SIZE 
			- t->vd.vsline * SCREEN_COLUMNS;
		show.dchar.clr = t->vd.color;
		oldtmp = t->vd.oldrear;
		while((oldtmp != t->vd.textrear) || bknum)
		{
			if(oldtmp == t->vd.textrear )
			{//Ĩ��backspace
				while(bknum)
				{
					show.dchar.ch = t->vd.TextBuf[oldtmp];
					*vdest++ = show.sh;
					oldtmp = (oldtmp + 1) % TEXTBUF_SIZE;
					bknum--;
				}
				oldtmp = t->vd.textrear;
			}
			else
			{
				show.dchar.ch = t->vd.TextBuf[oldtmp];
				*vdest++ = show.sh;
				oldtmp = (oldtmp + 1) % TEXTBUF_SIZE;
			}
		}
	}
    else
	{
		show.dchar.clr = t->vd.color;
		vdest = (unsigned short *)PHYSIC_VIDEO_BUFFER;
		oldtmp = (t->vd.textfront + t->vd.vsline * SCREEN_COLUMNS) % TEXTBUF_SIZE; 
		//��oldtmp����ʼ��ʾ,������ò���
		while(oldtmp != t->vd.textrear)
		{
			show.dchar.ch = t->vd.TextBuf[oldtmp];
			*vdest++ = show.sh;
			oldtmp = (oldtmp + 1) % TEXTBUF_SIZE;
		}
		show.dchar.clr = t->vd.color;
		show.dchar.ch = ' ';
		while(vdest < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS)
		{
			*vdest++ = show.sh;
		}
	}
}






