/*---------------------------------------------------*
 *说明:  处理时钟中断和定时器方面的工作
 *                    huangheshui 2007-06-17  21:05
 *
 *---------------------------------------------------*/
#include "..\include\asm.h"
#include  "..\include\color.h"
#include  "..\include\process.h"
#include  "..\include\timer.h"


extern void Scheduler();
extern void printk(unsigned int color, const char *lpstr);
extern void do_floppy_timer();
extern struct PCB *pCurrent;
extern struct Timer_Node *pglbfirtimer;


/*==========================================*
 *F: 处理时钟中断
 *I: 被中断历程的CPL
 *O: void
 *==========================================*/
void fndo_clock( unsigned int old_cpl )
{
	void (*pfn)();

	if(old_cpl)
		pCurrent->uticks++;
	else
		pCurrent->kticks++;
	//软驱定时处理
	do_floppy_timer();
	//处理定时器,去掉到时的节点
	if(pglbfirtimer)
	{
		pglbfirtimer->jiffies--;
		while(pglbfirtimer && pglbfirtimer->jiffies <= 0)
		{
			pfn = pglbfirtimer->pfn;
			pglbfirtimer->pfn = 0;
			pglbfirtimer = pglbfirtimer->pnext;
			(pfn)();
		}
	}
	if((--pCurrent->counter) > 0)
		return;
	pCurrent->counter = 0;
	if(!old_cpl)
		return;	 //在内核状态下不进行调度
	Scheduler();
}
