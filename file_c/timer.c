/*-------------------------------------------------------------*	
 *说明: 与定时器相关,添加定时器	
 *                            07/05/12  18:45
 *							  2007-06-17 13:25
 *-------------------------------------------------------------*/
#include  "..\include\asm.h"
#include  "..\include\timer.h"
#include  "..\include\color.h"

extern void vsprintf(char *buf, char *fmt, ...);
extern void printk(unsigned long color, const char *lpstr);

#define	TIMERS_MAX	64	//最多可有64个定时器

//定时器链表(以数组形式)
//
struct Timer_Node glbtmrlist[TIMERS_MAX];
struct Timer_Node *pglbfirtimer = 0;  //第一个定时节点指针

 
/*==========================================================*
 *F: 添加定时器节点
 *I: jiffies-定时值，*fn()-处理函数
 *O: 0-失败，1-成功
 *==========================================================*/
int fnAddTimer(int jiffies, void (*pfn)())
{
	struct Timer_Node *p;
	
	Cli();	//避免其它定时器的影响,此时这也是个“竞争资源”
	if(jiffies <= 0)
	{
		(pfn)();
		Sti();
		return 0;
	}
	else
	{
		//从定时器数组中找一个空闲节点
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
		//开始对链表进行节点调整，构成定时数轴
		while(p->pnext && p->pnext->jiffies < p->jiffies)
		{
			p->jiffies -= p->pnext->jiffies;
			//交换这相临节点的jiffies ,pfn
			pfn = p->pfn;
			p->pfn = p->pnext->pfn;
			p->pnext->pfn = pfn;
			jiffies = p->jiffies;
			p->jiffies = p->pnext->jiffies;
			p->pnext->jiffies = jiffies;
			//下一个节点
			p = p->pnext;
		}
		if(p->pnext)
			p->pnext->jiffies -= p->jiffies;
	}
	Sti();
	return 1;
}

/*==========================================================*
 *F: 显示定时器链表
 *C: 调试使用
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