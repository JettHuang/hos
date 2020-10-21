/*--------------------------------------------------------------*
 *说明:  我不知道其它TTY是怎样定义的。我们这里的TTY是这样描述的，
 *    TTY的组成部分:  
 *    struct TTY{
 *			struct InPutKB;
 *			struct OutPutVD;
 *			struct PCB *owner;
 *	  };
 *					huangheshui  2007-06-20   15:15
 *--------------------------------------------------------------*/
#ifndef __TTY_H
#define __TTY_H 

#include "..\include\process.h"


//TTY键盘输入缓冲区结构定义
//
struct InPutKB{
    char *iqueue;     //输入缓冲队列
    int front, rear;           //队列头、尾指针
	int imode;                 //同步模式
};

//TTY文本显示结构定义
//
struct OutPutVD{
    char *TextBuf;             //文本缓冲区
	unsigned char color;       //前、背颜色
	int textfront, textrear;   //整个文本范围
	int vsline;          //从该行开始显示到屏幕上(base 0)它是个相对与front
    int chws;            //已经追加的字符个数
    int oldrear;
	int csrchs;          //屏幕上字的符个数
	int bkflg;           // backspace键按下标志
};

//TTY控制台结构定义
//
struct TTY{
	struct InPutKB kb;
	struct OutPutVD vd;
	struct PCB *wait;          //等待tty输入的进程(也只有进程它自己)
};


//有关键盘的常量
#define KB_NOTSET	       0xffffffff    //没有设置有意义的模式
#define KB_SYNC	           0x00000001    //同步获得按键,不设置则默认异步
#define KB_WAKEUP_CHAR     0x00000002    //遇到指定字符唤醒等待
#define KB_WAKEUP_NUMS     0x00000004    //获得一定数量字符唤醒等待
#define KB_QUEUELEN		   1024
//有关显示屏幕的有关常量
#define SCREEN_ROWS		    25
#define SCREEN_COLUMNS	    80
#define PHYSIC_VIDEO_BUFFER (0xC0000000 + 0x0B8000)
#define TEXTBUF_SIZE        4081         //文本缓冲区共51 rows
#define TEXTBUF_ROWS        51
#endif /* __TTY_H */