/*---------------------------------------------*
 *����һЩ���������������ж���������
 *                   2007-06-17 19:37
 *---------------------------------------------*/

extern  label_des_idt;

//�����ж���, dpl = 0
//
void set_trap_gate(int index, void (*fnoffset)()) 
{
	unsigned int addr_gate;
	addr_gate = (int)(&label_des_idt) + index * 8;
	__asm{
		mov ecx, addr_gate
		mov edx, fnoffset			//�ж����̵�ַƫ��
		mov eax, 0x080000			//Selector_code32
		mov ax, dx				//ax���offset's low 16 bits
		mov dx, 0<<13 + 1<<15 + 0x0e<<8		//dpl + p + type
		mov dword ptr [ecx], eax 
		mov dword ptr [ecx + 4], edx 
	}
}


//����ϵͳ�ж���, dpl = 3
//
void set_system_gate(int index , void (*fnoffset)() ) 
{
	unsigned int addr_gate;
	addr_gate = (int)(&label_des_idt) + index * 8;
	__asm{
		mov ecx, addr_gate
		mov edx, fnoffset			//�ж����̵�ַƫ��
		mov eax, 0x080000			//Selector_code32
		mov ax, dx				//ax���offset's low 16 bits
		mov dx, 3<<13 + 1<<15 + 0x0e<<8		//dpl + p + type
		mov dword ptr [ecx], eax 
		mov dword ptr [ecx + 4], edx 
	}
}

