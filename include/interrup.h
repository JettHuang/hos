/*---------------------------------------------*
 *定义一些函数，用于设置中断描述符表
 *                   2007-06-17 19:37
 *---------------------------------------------*/

extern  label_des_idt;

//设置中断门, dpl = 0
//
void set_trap_gate(int index, void (*fnoffset)()) 
{
	unsigned int addr_gate;
	addr_gate = (int)(&label_des_idt) + index * 8;
	__asm{
		mov ecx, addr_gate
		mov edx, fnoffset			//中断历程地址偏移
		mov eax, 0x080000			//Selector_code32
		mov ax, dx				//ax存放offset's low 16 bits
		mov dx, 0<<13 + 1<<15 + 0x0e<<8		//dpl + p + type
		mov dword ptr [ecx], eax 
		mov dword ptr [ecx + 4], edx 
	}
}


//设置系统中断门, dpl = 3
//
void set_system_gate(int index , void (*fnoffset)() ) 
{
	unsigned int addr_gate;
	addr_gate = (int)(&label_des_idt) + index * 8;
	__asm{
		mov ecx, addr_gate
		mov edx, fnoffset			//中断历程地址偏移
		mov eax, 0x080000			//Selector_code32
		mov ax, dx				//ax存放offset's low 16 bits
		mov dx, 3<<13 + 1<<15 + 0x0e<<8		//dpl + p + type
		mov dword ptr [ecx], eax 
		mov dword ptr [ecx + 4], edx 
	}
}

