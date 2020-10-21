/*---------------------------------------------------*
 *˵��:  ����ʱ���жϺͶ�ʱ������Ĺ���
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
 *F: ����ʱ���ж�
 *I: ���ж����̵�CPL
 *O: void
 *==========================================*/
void fndo_clock( unsigned int old_cpl )
{
	void (*pfn)();

	if(old_cpl)
		pCurrent->uticks++;
	else
		pCurrent->kticks++;
	//������ʱ����
	do_floppy_timer();
	//����ʱ��,ȥ����ʱ�Ľڵ�
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
		return;	 //���ں�״̬�²����е���
	Scheduler();
}
