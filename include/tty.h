/*--------------------------------------------------------------*
 *˵��:  �Ҳ�֪������TTY����������ġ����������TTY�����������ģ�
 *    TTY����ɲ���:  
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


//TTY�������뻺�����ṹ����
//
struct InPutKB{
    char *iqueue;     //���뻺�����
    int front, rear;           //����ͷ��βָ��
	int imode;                 //ͬ��ģʽ
};

//TTY�ı���ʾ�ṹ����
//
struct OutPutVD{
    char *TextBuf;             //�ı�������
	unsigned char color;       //ǰ������ɫ
	int textfront, textrear;   //�����ı���Χ
	int vsline;          //�Ӹ��п�ʼ��ʾ����Ļ��(base 0)���Ǹ������front
    int chws;            //�Ѿ�׷�ӵ��ַ�����
    int oldrear;
	int csrchs;          //��Ļ���ֵķ�����
	int bkflg;           // backspace�����±�־
};

//TTY����̨�ṹ����
//
struct TTY{
	struct InPutKB kb;
	struct OutPutVD vd;
	struct PCB *wait;          //�ȴ�tty����Ľ���(Ҳֻ�н������Լ�)
};


//�йؼ��̵ĳ���
#define KB_NOTSET	       0xffffffff    //û�������������ģʽ
#define KB_SYNC	           0x00000001    //ͬ����ð���,��������Ĭ���첽
#define KB_WAKEUP_CHAR     0x00000002    //����ָ���ַ����ѵȴ�
#define KB_WAKEUP_NUMS     0x00000004    //���һ�������ַ����ѵȴ�
#define KB_QUEUELEN		   1024
//�й���ʾ��Ļ���йس���
#define SCREEN_ROWS		    25
#define SCREEN_COLUMNS	    80
#define PHYSIC_VIDEO_BUFFER (0xC0000000 + 0x0B8000)
#define TEXTBUF_SIZE        4081         //�ı���������51 rows
#define TEXTBUF_ROWS        51
#endif /* __TTY_H */