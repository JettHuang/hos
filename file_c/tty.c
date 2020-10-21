/*----------------------------------------------------------------*
 *说明：   该模块中实现了操作TYY控制台的函数, 可参考bmp文件夹中图
 *     tty.bmp中描述的构造方法。
 *     只有正在被用户选中的tty才能够拥有物理键盘和显示屏幕的使用权
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

//全局变量，被选中的tty指针
struct TTY *glbTTYSelected;
 
/*======================================================*
 *F: 为某进程创建一个tty对象.
 *I: p-进程的PCB指针
 *O: 0-失败, 1-成功.
 *C: 如果该进程已经有TTY了则不再给它创建.
 *======================================================*/
int CreateTTY(struct PCB *p)
{
	struct TTY *t;

	if(!p || p->ptty) 
		return 0;
    if(!(t = (struct TTY *)fnVmk_alloc(sizeof(struct TTY))))
		return 0;
	//初始化该tty对象
	t->kb.front = t->kb.rear = 0;
	t->kb.imode = KB_NOTSET;
	//申请键盘队列缓冲区
	if(!(t->kb.iqueue = (char *)fnVmk_alloc(KB_QUEUELEN)))
	{
		fnVmk_free(t, sizeof(struct TTY));
		return 0;
	}
	//初始化屏幕显示结构
	t->vd.color = (unsigned char)(BLACK * 16 + LIGHTGREEN);
	t->vd.textfront = t->vd.textrear = 0;
	t->vd.vsline = 0;
	t->vd.chws = 0;   //已经追加的字符个数
	t->vd.oldrear = 0;
	t->vd.bkflg = 0;
	t->vd.csrchs = 0; //屏幕上字的符个数
	//申请文本缓冲区
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
 *F: 将某进程的tty对象销毁.
 *I: p-进程的PCB指针
 *O: void
 *C: 如果该进程没有TTY了则不销毁,也不提示.
 *======================================================*/
void DestroyTTY(struct PCB *p)
{
	struct TTY *t;

	if(!p || !(p->ptty)) 
		return;
    t = p->ptty;
	//释放键盘缓冲队列
	if(t->kb.iqueue)
		fnVmk_free(t->kb.iqueue, KB_QUEUELEN);
	//释放文本缓冲区
	if(t->vd.TextBuf)
		fnFree_VmkPage((void *)(t->vd.TextBuf));
	t->wait = 0;
	fnVmk_free(t, sizeof(struct TTY));
}


/*======================================================*
 *F: 向tty的键盘缓冲队列尾部添加一个按键.
 *I: t-TTY对象指针, lkey-按键的逻辑码
 *O: 0-失败, !0-成功
 *C: 这里还检测同步模式位，根据需要可能唤醒等待按键的进程.
 *======================================================*/
int InputKeytoTTY(struct TTY *t, char lkey)
{
	char szbuf;

	if(!t) 
		return 0;
	if(t->kb.front == ((t->kb.rear + 1) % KB_QUEUELEN))
		return 0;  //缓冲区满,可以设置个响铃
	t->kb.iqueue[t->kb.rear++] = lkey;
	//检测同步标志
	if((t->kb.imode & KB_SYNC))
		fnWakeUp(&(t->wait));  //唤醒等待按键的进程
	//回写到屏幕
	szbuf = lkey;
	WriteTTY(t, &szbuf, 1);
	return 1;
}


/*======================================================*
 *F: 从tty对象获得一些输入按键.
 *I: t-TTY对象指针, lkey-按键的逻辑码
 *   kybuf-接收按键的缓冲区
 *   kynum-当mode中KB_WAKEUP_NUMS有效时指按键个数; 
 *         KB_WAKEUP_CHAR有效时只kybuf大小。
 *   mode-读取模式,可同时为KB_WAKEUP_NUMS | KB_WAKEUP_CHAR
 *        配套取值KB_SYNC、KB_ASYNC
 *   kywake- 当KB_WAKEUP_CHAR有效时，唤醒标志按键.
 *O: 获得的字符个数.
 *C: 函数根据mode,可能需要睡眠。警惕sizeof(char)
 *======================================================*/
int ReadTTY(struct TTY *t, char *kybuf, int kynum, int mode, char kywake)
{
	int len;

	if(!t || !kynum) 
		return 0;
	if(kynum >= KB_QUEUELEN)
		kynum = KB_QUEUELEN - 1;  //最多只能读(KB_QUEUELEN - 1)个按键
_repeat:
	len = (t->kb.rear - t->kb.front + KB_QUEUELEN) % KB_QUEUELEN;
	//测试键盘队列按键是否够数
	if(len < kynum)
	{
		if(mode & KB_SYNC)  //需要同步读
		{
			if(len && (mode & KB_WAKEUP_CHAR))  //遇到kywake醒
			{
				if(kywake == t->kb.iqueue[t->kb.rear - 1])
				{
					kynum = len;  //让读取字符个数变小->len
					goto _getkeys;
				}
			}
			//这儿不讨论KB_WAKEUP_NUMS了，让它成为默认情况吧
			t->kb.imode = KB_SYNC;
			fnSleepOn(&(t->wait));
			goto _repeat;
		}
		//异步读则代码直接向下执行
	}
_getkeys:
	//将键盘队列中的按键取出-->kybuf
	len = kynum;
	for(;kynum; kynum--)
	{
		*kybuf++ = t->kb.iqueue[t->kb.front++];
	}
	return len;
}


/*======================================================*
 *F: 对物理显示屏幕进行刷屏.
 *I: color-刷子颜色
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
 *F: 将整个屏幕上卷几行
 *I: rows-上卷行数, color-刷子颜色(刷掉旧痕).
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
	//处理 0 < rows < SCREEN_ROWS情况
	vsour = (unsigned short *)PHYSIC_VIDEO_BUFFER + rows * SCREEN_COLUMNS;
    for(vdest = (unsigned short *)PHYSIC_VIDEO_BUFFER; 
	   vsour < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS; )
	{
		*vdest++ = *vsour++;
	}
	//刷掉旧痕迹
	 for(; vdest < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS; )
	{
		*vdest++ = show.sh;
	}
}


/*======================================================*
 *F: 将tty对象的文本一个视图显示在屏幕上。
 *I: t-TTY对象指针 
 *O: void
 *C: 显示从viewoffset开始到textrear之间的字符 && 少于25行.
 *   并且TEXT中都是已经处理过的字符(可显示的)
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
		if(++wchs >= SCREEN_ROWS * SCREEN_COLUMNS)  //已经写满一个屏幕
			break;
	}
	//刷掉脏的屏幕区域
	show.dchar.ch = ' ';
	for(; vdest < (unsigned short *)(PHYSIC_VIDEO_BUFFER) + SCREEN_ROWS * SCREEN_COLUMNS; )
	{
		*vdest++ = show.sh;
	}
}


/*======================================================*
 *F: 向tty对象写入一段文本(追加)。
 *I: t-TTY对象指针, pszstr-字符串, len-字符个数
 *O: void
 *C: 当len==-1表明字符串以'\0'结尾.
 *======================================================*/
void WriteTTY(struct TTY *t, char *pszstr, int len)
{
    int num, textchs, oldtmp, bknum, oncewchs; //此次函数调用写入的字符个数
	int refresh;  //刷屏标志
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
	bknum = 0; //后退字符个数
	oncewchs = 0;
	refresh = 0;
	for(; len && *pszstr != '\0'; len--, pszstr++)
	{
		//是否TEXT满了,满则丢弃最前端一行
		if((t->vd.textrear + 1) % TEXTBUF_SIZE == t->vd.textfront)
		{
			t->vd.textfront = (t->vd.textfront + SCREEN_COLUMNS) % TEXTBUF_SIZE;
			refresh = 1;
		}
		switch((unsigned char)(*pszstr))
		{
		case LK_TABLE:  //制表键
			 for(num = 0; num < 8; num++)
			 {
				//是否TEXT满了,满则丢弃最前端一行
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
		case LK_CTRL_KEYJ: //换行键
			 //fall throught
		case LK_RETURN: //回车键
			 while(t->vd.chws == 0 || (t->vd.chws % SCREEN_COLUMNS) != 0)
			 {
				t->vd.TextBuf[t->vd.textrear] = LK_SPACE;
				t->vd.textrear = (t->vd.textrear + 1) % TEXTBUF_SIZE;
				t->vd.chws++;
				oncewchs++;
			 }
			 t->vd.chws = 0;  //该输入已成定局			 
			 break;
		case LK_BACKSPACE: //后退键
			 if(t->vd.chws)
			 {
				t->vd.textrear = (t->vd.textrear - 1 + TEXTBUF_SIZE) % TEXTBUF_SIZE;
				t->vd.TextBuf[t->vd.textrear] = LK_SPACE;
				t->vd.chws--;
				bknum++;
				oncewchs--; //屏幕上的字符少了一个
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
	
	//开始操作TEXT的显示
    textchs = (t->vd.textrear - t->vd.textfront + TEXTBUF_SIZE) % TEXTBUF_SIZE;
	//重置vsline,临时借用len
	len = textchs / SCREEN_COLUMNS - SCREEN_ROWS + 1;
	if((len >= t->vd.vsline || t->vd.bkflg == 0) 
		&& len > 0)
	{
		t->vd.vsline = len;
		t->vd.bkflg = 0;
	}
	oncewchs += t->vd.csrchs;  //此时屏幕上的字符总个数(可能比实际屏幕大)
	t->vd.csrchs = textchs - t->vd.vsline * SCREEN_COLUMNS;  //此时写TTY后屏幕上的字符个数
	//下面是不刷屏情况
	if(t->vd.csrchs < SCREEN_ROWS * SCREEN_COLUMNS 
		&& oncewchs < SCREEN_ROWS * SCREEN_COLUMNS)
	{
		//t->vd.oldrear开始显示
		vdest = (unsigned short *)PHYSIC_VIDEO_BUFFER + 
			(t->vd.oldrear - t->vd.textfront + TEXTBUF_SIZE) % TEXTBUF_SIZE 
			- t->vd.vsline * SCREEN_COLUMNS;
		show.dchar.clr = t->vd.color;
		oldtmp = t->vd.oldrear;
		while((oldtmp != t->vd.textrear) || bknum)
		{
			if(oldtmp == t->vd.textrear )
			{//抹掉backspace
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
		//从oldtmp处开始显示,名字起得不好
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






