#ifndef __TIMER_H
#define __TIMER_H

//��ʱ���ڵ�ṹ����
//
struct Timer_Node{
	int jiffies;
	void (*pfn)();
	struct Timer_Node *pnext;
};

#endif //__TIMER_H