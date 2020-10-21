/*-------------------------------------------------------------*	
 *˵��: �붨ʱ�����,��Ӷ�ʱ��	
 *                            07/05/12  18:45
 *							  2007-06-17 13:25
 *-------------------------------------------------------------*/
#include  "..\include\asm.h"
#include  "..\include\timer.h"
#include  "..\include\color.h"

extern void vsprintf(char *buf, char *fmt, ...);
extern void printk(unsigned long color, const char *lpstr);

#define	TIMERS_MAX	64	//������64����ʱ��

//��ʱ������(��������ʽ)
//
struct Timer_Node glbtmrlist[TIMERS_MAX];
struct Timer_Node *pglbfirtimer = 0;  //��һ����ʱ�ڵ�ָ��

 
/*==========================================================*
 *F: ��Ӷ�ʱ���ڵ�
 *I: jiffies-��ʱֵ��*fn()-������
 *O: 0-ʧ�ܣ�1-�ɹ�
 *==========================================================*/
int fnAddTimer(int jiffies, void (*pfn)())
{
	struct Timer_Node *p;
	
	Cli();	//����������ʱ����Ӱ��,��ʱ��Ҳ�Ǹ���������Դ��
	if(jiffies <= 0)
	{
		(pfn)();
		Sti();
		return 0;
	}
	else
	{
		//�Ӷ�ʱ����������һ�����нڵ�
		for(p = glbtmrlist; p < glbtmrlist + TIMERS_MAX; p++)
		{
			if(!p->pfn)
				break;
		}
		if(p >= glbtmrlist + TIMERS_MAX)
		{
			Sti();
			return 0;
		}
		p->pfn = pfn;
		p->jiffies = jiffies;
		p->pnext = pglbfirtimer;
		pglbfirtimer = p;
		//��ʼ��������нڵ���������ɶ�ʱ����
		while(p->pnext && p->pnext->jiffies < p->jiffies)
		{
			p->jiffies -= p->pnext->jiffies;
			//���������ٽڵ��jiffies ,pfn
			pfn = p->pfn;
			p->pfn = p->pnext->pfn;
			p->pnext->pfn = pfn;
			jiffies = p->jiffies;
			p->jiffies = p->pnext->jiffies;
			p->pnext->jiffies = jiffies;
			//��һ���ڵ�
			p = p->pnext;
		}
		if(p->pnext)
			p->pnext->jiffies -= p->jiffies;
	}
	Sti();
	return 1;
}

/*==========================================================*
 *F: ��ʾ��ʱ������
 *C: ����ʹ��
 *==========================================================*/
void fnDisplayTimers()
{
	struct Timer_Node *p;
	char szBuffer[128];
    
	Cli();
	for(p = pglbfirtimer; p; p = p->pnext)
	{
		vsprintf(szBuffer, "jiffies = 0x%x\n", p->jiffies);
		printk(GREEN, szBuffer);
	}
	Sti();    
}