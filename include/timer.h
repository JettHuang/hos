#ifndef __TIMER_H
#define __TIMER_H

//定时器节点结构定义
//
struct Timer_Node{
	int jiffies;
	void (*pfn)();
	struct Timer_Node *pnext;
};

#endif //__TIMER_H