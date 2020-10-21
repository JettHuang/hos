/*----------------------------------------------------------------------------*
 *�������̲���1.44 MB, �ҵĵ���ֻ��һ����������֧��һ��
 *����.����ĺ������ù�ϵ�Ƚϸ���,�����ܵĵ���ԭ����:
 *		���������FDC------>�ص��û�ģʽ(FDC��CPU����)------->FDC�жϱ�����
 *-------->�жϴ�������Ӧ(����ָʾ) ------->   .....
 *													huangheshui	05/25/07  21:45
 *----------------------------------------------------------------------------*/
#include  "..\include\asm.h"
#include  "..\include\color.h"
#include  "..\include\floppy.h"
#include  "..\include\process.h"
#include  "..\include\blk.h"

extern struct stBlk_dev_IO  glbBlk_devs[NR_BLK_DEV];
extern void floppy_interrupt();
extern void fnVmkCpy(char *dest, char *sour, unsigned int size);
extern void fnSleepOn(struct PCB * *pwait);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);
extern void fnEnd_request(int major, int update);
extern int fnAddTimer(int jiffies, void (*pfn)());
extern void set_trap_gate(int index, void (*fnoffset)());

/*-----------------------------��ģ���к���������ʼ-------------------------------*/
void fnDo_transfer();
int ticks_to_floppy_on();
void floppy_on();
void floppy_off();
void do_floppy_timer();
int floppy_change();
void setup_DMA();
void output_byte(unsigned char byte);
int read_result();
void recalibrate_floppy();
void fnRecal_interrupt();
void fnReset_floppy();
void fnReset_interrupt();
void fnSeek_interrupt();
void bad_flp_intr();
void fnSetup_rw_floppy();
void fnRw_interrupt();
void fnDo_fd_request();
void fnDo_transfer();
/*-----------------------------��ģ���к���������ʼ-------------------------------*/

#define	COMPLIER_BASEADDR	0xC0000000
/*��ǰ������������*/
#define CURRENT	  (glbBlk_devs[MAJOR_NR_FLY].current_request)
//���̽ṹ����
struct floppy_struct{
	unsigned int size,
				 sect,
				 head,
				 track,
				 stretch;
	unsigned char gap,
				  rate,
				  specl;
}floppy_type = {2880, 18, 2, 80, 0, 0x1b, 0x00, 0xcf}; /* 1.44Mb���̲��� */

struct PCB * glbpWait_motor = 0; /* �ȴ�floppy motor�Ľ���ָ�� */
int glbMon_timer;                /* ����������������ʱ��ticks */
int glbMoff_timer;               /* �����ر�ǰ���ӳ�ʱ��ticks */
static int flgNeed_Recalibrate = 0; /* ��־����Ҫ����У�� */
static int flgNeed_Reset = 0;       /* ��Ҫ��FDC��λ */
static int flgNeed_Seek = 0;		/* ��ҪѰ������ */
void (* pfnDo_floppy)();            /* ��ǰ�����̵Ĳ����жϴ��� */

unsigned char glbCurrent_DOR = 0x0c;     /* ��������Ĵ���(����DMA���ж���������FDC) */
unsigned char glbReply_Buf[MAX_REPLIES]; /* ִ��FDC����ؽ�������� */

unsigned char glbCurrent_cylinder = 255; /* ��ǰ����� */
unsigned char glbHead = 0;				 /* ��ͷ�� */
unsigned char glbSector = 0;             /* ������ */
unsigned char glbCylinder = 0;           /* ����� */
unsigned char glbCur_specl = 0xff;		 /* ������+��ͷж��ʱ�� */
unsigned char glbCur_rate = 0xff;        /* ���ݴ����� */
unsigned char glbCommand = 0;            /* FDC��д���� */
unsigned char glbSeek_cylinder = 0;      /* ҪѰ�ҵ������(ϰ�߽�Ѱ��) */  

/* DMA��Ҫ�õ�����ʱ������,�Ҳ�֪��DMAʹ�õ��������ַ?�������Ե�ַ(������ҳ����) */
char glbFly_DMAbuf[1024];


/*=======================================================*
 *F: ��������������ת��<����ʱ>ticks
 *I: void
 *O: void
 *C: �����ʱ��û���������̣���ô������������ʼ��ʱ.
 *=======================================================*/
int ticks_to_floppy_on()
{
	glbMoff_timer = 2000;                 /* 20sÿ�ζ�Ҫ���õ�! */
	if(!(glbCurrent_DOR & MOTOR_A_RUNING)) /* ����綯����û�п��� */
	{
		glbCurrent_DOR |= MOTOR_A_RUNING;
		Out_Byte(FDC_DOR_PORT, glbCurrent_DOR)
		glbMon_timer = 50;                 /* 0.5s������ʱ�� */
	}
	return glbMon_timer;
}


/*=======================================================*
 *F: ���������綯����ֱ������ת�ٲ��㿪�����
 *I: void
 *O: void
 *=======================================================*/
void floppy_on()
{
	while(ticks_to_floppy_on())
		fnSleepOn(&glbpWait_motor);  /* �ȴ�������ת�� */
}


/*=======================================================*
 *F: �ر������綯��
 *I: void
 *O: void
 *=======================================================*/
void floppy_off()
{
	glbMoff_timer = 3 * 100;  /* 3s��رյ綯�� */
}


/*=======================================================*
 *F: ������ʱ���������clock intr�е���
 *I: void
 *O: void
 *=======================================================*/
void do_floppy_timer()
{
	if(!(glbCurrent_DOR & MOTOR_A_RUNING))  /* �綯��û�п���,���� */
		return;
	if(glbMon_timer)
	{
		if(!(--glbMon_timer))          /* �Ѿ��ﵽ����ת�� */
		{
			fnWakeUp(&glbpWait_motor); /* ���ѵȴ����� */
		}
	}
	else if(!glbMoff_timer)  /* ͣתʱ�䵽 */
	{
		glbCurrent_DOR &= ~MOTOR_A_RUNING; /* �رյ綯�� */
		Out_Byte(FDC_DOR_PORT, glbCurrent_DOR)
	}
	else
		glbMoff_timer--;
}


/*=======================================================*
 *F: ��������A�е������Ƿ��Ѹ���
 *I: �������--1, ����--0
 *O: void
 *=======================================================*/
int floppy_change()
{
	unsigned char valdir;

	floppy_on();
	In_Byte(FDC_DIR_PORT, valdir)
	if(valdir & 0x80)
	{
		printk(BROWN, "floppy changed");
		floppy_off();
		return 1;
	}
	floppy_off();
	return 0;
}


/*=======================================================*
 *F: ��ʼ��(����)���̵�DMAͨ��,����(DMA_READ, DMA_WRITE)
 *I: void
 *O: void
 *=======================================================*/
void setup_DMA()
{
	union {
		unsigned int pyhsic; /* �������ĵ�ַ */
		struct {
			unsigned char first; /* ��8λ */
			unsigned char second;
			unsigned char third;
			unsigned char forth; /* ��8λ */
		}bytes;
	}addr;
	unsigned char command;

	/* ���ǵ�buffer��1M��(�����ַ),�����ʱDMA�����������ַ */
	addr.pyhsic = (unsigned int)glbFly_DMAbuf - COMPLIER_BASEADDR; 
	if(glbCommand == FDC_WRITE)
		fnVmkCpy(glbFly_DMAbuf, (char *)CURRENT->pdata_buf, 1024);
	if(glbCommand == FDC_READ)
		command = DMA_READ;
	else
		command = DMA_WRITE;
	/* ����DMAͨ��2 */
	/* ��ͨ�����μĴ����˿�=0x0A, bit0~1ָ��DMAͨ��(0~3), bit2:1--���Σ�0--�������� */
	Out_Byte(DMA_INIT, 4|2)
	/* д��ʽ��(read=0x46, write=0x4A) */
	Out_Byte(DMA_FLIPFLOP, command)
	Out_Byte(DMA_MODE, command)
	/* д��addr */
	Out_Byte(DMA_ADDR, addr.bytes.first)
	Out_Byte(DMA_ADDR, addr.bytes.second)
	Out_Byte(DMA_TOP, addr.bytes.third) /* bit 16~19 */
	/* д�����count-1 */
	Out_Byte(DMA_COUNT, 0xff)
	Out_Byte(DMA_COUNT, 0x03) /* count = 0x3ff + 1 = 1024�ֽ� */
	/* ����DMAͨ��2������ */
	Out_Byte(DMA_INIT, 0|2)
}


/*------------------------------------------------------------------------*
 *����: ����ĺ����ṩ����д��������ơ������ж�
 *
 *------------------------------------------------------------------------*/							
/*=======================================================*
 *F: �����̿��������һ���ֽ�����(��������)
 *I: byte-�����ֻ��������
 *O: void
 *=======================================================*/
void output_byte(unsigned char byte)
{
	int count;
	unsigned char status;

	if(flgNeed_Reset) /* ��λ�Ͳ�Ҫ����д������ */
		return;
	for(count = 0; count < 10000; count++)
	{
		In_Byte(FDC_STATUS_PORT, status) /* ��ȡ״̬�Ĵ��� */
		status &= STATUS_DATA_READY | STATUS_DIRECTION; /* ready =1, direction = 0 */
		if(status == STATUS_DATA_READY)
		{
			status = byte; /* ��,��Ȼ����ֱ����byte */
			Out_Byte(FDC_DATA_PORT, status) 
			return; /* �ɹ� */
		}
	}
	flgNeed_Reset = 1;
	printk(RED, "output_byte() failed!");
}


/*=======================================================*
 *F: ��ȡFDCִ�еĽ����Ϣ�����7B,�����reply_buffer[]��.
 *I: void
 *O: ���ض�ȡ�Ľ���ֽ���Ŀ, -1��ʾ����
 *=======================================================*/
int read_result()
{
	int i = 0, count;
	unsigned char status, reply;

	if(flgNeed_Reset) /* FDC error,�Ͳ��ܶ������� */
		return -1;
	for(count = 0; count < 10000; count++)
	{
		In_Byte(FDC_STATUS_PORT, status) /* ��ȡ״̬�Ĵ��� */
		status &= STATUS_DATA_READY | STATUS_DIRECTION | STATUS_FDC_BUSY;
		if(status == STATUS_DATA_READY)
			return i; /* ��ʱFDC��æ��cpu->FDC˵������ִ�н����� */
		if(status == (STATUS_DATA_READY | STATUS_DIRECTION | STATUS_FDC_BUSY))
		{
			if(i >= MAX_REPLIES)
				break; /* FDC error */
			In_Byte(FDC_DATA_PORT, reply)
			glbReply_Buf[i++] = reply; /* ����reply */
		}
	}
	flgNeed_Reset = 1;
	printk(RED, "read_result() failed!");
	return -1;
}


/*=======================================================*
 *F: ������У��,�����̿�����FDC����У������Ͳ���������λУ����־
 *I: void
 *O: void
 *=======================================================*/
void recalibrate_floppy()
{
	flgNeed_Recalibrate = 0;
	glbCurrent_cylinder = 0; /*У�����ǽ���ͷ��������*/
	pfnDo_floppy = fnRecal_interrupt; /*��������FDCУ���������ж�*/
	output_byte(FDC_RECALIBRATE);
	output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); /*���Ͳ���:��ͷ��+��������*/
	if(flgNeed_Reset) /*�ܿ��ܷ���output_byte()ʧ��*/
		fnDo_fd_request();
}


/*=======================================================*
 *F: ��������У��ִ�к�������ж�
 *=======================================================*/
void fnRecal_interrupt()
{
	/* ��Ҫ��������ִ�н�� */
	output_byte(FDC_SENSEI);
	if(read_result()!=2 || (glbReply_Buf[0] & 0xe0) == 0x60) /* У�������쳣���� */
	{/* Ӧ�÷�����������ֽ� */
		flgNeed_Reset = 1;
	}
	else
		flgNeed_Recalibrate = 0;
	fnDo_fd_request();
}


/*=======================================================*
 *F: ��λFDC,ͨ������������Ĵ���DOR's bit2��λһ��ʱ��
 *=======================================================*/
void fnReset_floppy()
{
	int i;
	unsigned char dor;

	flgNeed_Reset = 0;
	glbCur_specl = 0xff;  /* ������+��ͷж��ʱ�� */
	glbCur_rate = 0xff;
	flgNeed_Recalibrate = 1;  /* ������Ӧ������У�� */
	/* ׼��reset FDC */
	pfnDo_floppy = fnReset_interrupt;
	dor = (unsigned char)(glbCurrent_DOR & ~0x04);
	Out_Byte(FDC_DOR_PORT, dor);
	for(i = 0; i < 100; i++)
		NOP()
	Out_Byte(FDC_DOR_PORT, glbCurrent_DOR);  /* ����FDC,֮ǰmotorӦ������ת�� */
}


/*=======================================================*
 *F: ������λFDC������������ж�
 *=======================================================*/
void fnReset_interrupt()
{
	/* ��Ҫ��������ִ�н�� */
	output_byte(FDC_SENSEI);
	read_result();             /* ��ȡ����ִ�н�� */
	output_byte(FDC_SPECIFY);  /* �趨���� */
	output_byte(glbCur_specl); /* step rate time | head unload time */
	output_byte(6);            /* head load time = 6ms, DMA ������ */
	fnDo_fd_request(); 
}


/*=======================================================*
 *F: ����δ֪�������ж�
 *=======================================================*/
void fnUnknown_interrupt()
{
    /* ��Ҫ��������ִ�н�� */
	output_byte(FDC_SENSEI);
	if(read_result()!=2 || (glbReply_Buf[0] & 0xe0) == 0x60)
		flgNeed_Reset = 1;
	else
		flgNeed_Recalibrate = 1; 
}


/*=======================================================*
 *F: ����FDC_SEEK����ִ�в������ж�,FDC�������ǿ��Զ�д������
 *=======================================================*/
void fnSeek_interrupt()
{
	/* ��Ҫ��������ִ�н�� */
	output_byte(FDC_SENSEI);
	if(read_result()!=2 || (glbReply_Buf[0] & 0xf8) != 0x20 ||
		glbReply_Buf[1] != glbSeek_cylinder) /* seek����,��û�е���Ҫ��cylinder */
	{
		bad_flp_intr();  /* ������ */
		fnDo_fd_request();
		return;
	}
	glbCurrent_cylinder = glbReply_Buf[1];  /* ���õ�ǰcylinder */
	fnSetup_rw_floppy();                    /* ���Է����д������*/
}


/*=======================================================*
 *F: �����ж��з��ֵĴ���,seek��������
 *=======================================================*/
void bad_flp_intr()
{
	CURRENT->errors++;
	if(CURRENT->errors > MAX_ERRORS)
	{
		fnEnd_request(MAJOR_NR_FLY, 0);  /* ������Ч��־0 */
		return;
	}
	if(CURRENT->errors > MAX_ERRORS / 2)
		flgNeed_Reset = 1;  /* reset FDC */
	else
		flgNeed_Recalibrate = 1;  /* Ҫ��У�� */
}


/*=======================================================*
 *F:   ����DMA��������̲����������(1 CMD + 0~7 PARAM),
 *  ʵ�ֶ�д������
 *=======================================================*/
void fnSetup_rw_floppy()
{
	setup_DMA();
	pfnDo_floppy = fnRw_interrupt;
	output_byte(glbCommand);                /* ���������ֽ� */
	output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); /* ��ͷ�� + �������� */
	output_byte(glbCylinder);               /* ����� */
	output_byte(glbHead);                   /* ��ͷ�� */
	output_byte(glbSector);                 /* ��ʼ���� */
	output_byte(2);                         /* N=2��ʾ512 bytes/sector */
	output_byte((unsigned char)floppy_type.sect); /* �ŵ�������������base=1 */
	output_byte(floppy_type.gap);           /* ����������� */
	output_byte(0xff);                      /* ����!��ΪN=0ʱ������ */
	if(flgNeed_Reset)
		fnDo_fd_request();
}


/*=======================================================*
 *F: ����ִ�ж�д����������ж�
 *=======================================================*/
void fnRw_interrupt()
{
	if(read_result()!=7 || (glbReply_Buf[0] & 0xf8) ||
		(glbReply_Buf[1] & 0xb7) || (glbReply_Buf[2] & 0x73))
	{
		if(glbReply_Buf[1] & 0x02)
		{
			printk(RED, "write protect in floppy!");
			fnEnd_request(MAJOR_NR_FLY, 0);  /* ������Ч��־0 */
		}
		else
			 bad_flp_intr();
		fnDo_fd_request();
		return;
	}
	/* OK,��ʼת������ */
	if(glbCommand == FDC_READ)
		fnVmkCpy((char *)CURRENT->pdata_buf, glbFly_DMAbuf, 1024);
	fnEnd_request(MAJOR_NR_FLY, 1); /* ������Ч��־1 */
	fnDo_fd_request();              /* ����������һ�������� */
}


/*===============================================================*
 *F: fnDo_fd_request()�Ǵ���������׷�����,��ϣ�����ܹ����췵��;
 *   ���м������ܷ�֧:У������λ��Ѱ����������һ��м�����������
 *   ����,�������õ�Ŀ�Ĳ�����ͬ,ֻҪ����ʱ��ͣ����ring0̬�ͺ���
 *===============================================================*/
void fnDo_fd_request()
{
	unsigned int block;

	flgNeed_Seek = 0; /* seek ����transfer()����� */
	if(flgNeed_Reset) /* reset FDC���� */
	{
		fnReset_floppy();
		return;
	}
	if(flgNeed_Recalibrate) /* У������������ */
	{
		recalibrate_floppy();
		return;
	}
	/* ����������, �����Ĵ����д������ */
__repeat:
	if(!CURRENT)
		return;
	block = CURRENT->begin_sector;      /* base 0 */
	if(block + 2 > floppy_type.size)    /* �������̵Ĵ�С */
	{
		fnEnd_request(MAJOR_NR_FLY, 0); /* ������Ч��־0 */
		goto __repeat;
	}
	/* ���Ӧ�ڴŵ��ϵ�������,��ͷ��,�ŵ���,�����ŵ��� */
	glbSector = (unsigned char)(block % floppy_type.sect);   /* �ŵ��ϵ������� base 0 */
	block /= floppy_type.sect;                               /* �߼��ŵ��� base 0 */
	glbHead = (unsigned char)(block % floppy_type.head);     /* ��ͷ�� */
	glbCylinder = (unsigned char)(block / floppy_type.head); /* ����� */
	glbSeek_cylinder = (unsigned char)(glbCylinder << floppy_type.stretch); /* Ѱ��������Ҫ���� */
	if(glbSeek_cylinder != glbCurrent_cylinder)
		flgNeed_Seek = 1;                   /* need seek */
	glbSector++;                            /* ��������,ʵ������������1��ʼ */
	if(CURRENT->cmd == READ)
		glbCommand = FDC_READ;
	else if(CURRENT->cmd == WRITE)
		glbCommand = FDC_WRITE;
	else
		printk(RED, "fnDo_fd_request(): unknown cmd!");
	/* ׼��:�����ǲ��ܹ�ʹ��floppy_on()����Ϊ�����Ѿ�˯�߹�һ���� */
	fnAddTimer(ticks_to_floppy_on(), &fnDo_transfer);	
}


/*=======================================================*
 *F: fnDo_transfer()�ɶ�ʱ�����õ�.���motor������ת���
 *   floppy���ʴ���
 *=======================================================*/
void fnDo_transfer()
{
	if(glbCur_rate != floppy_type.rate)
	{
		glbCur_rate = floppy_type.rate;
		Out_Byte(FDC_DCR_PORT, glbCur_rate)
	}
	if(flgNeed_Reset) /* Ҫ��reset FDC, Ӧ�ò���ִ�и����� */
	{
		fnDo_fd_request();
		return;
	}
	if(!flgNeed_Seek) /* ����Ҫseek,����RW */
	{
		fnSetup_rw_floppy();
		return;
	}
	/* Ѱ������,����FDCѰ�� */
	pfnDo_floppy = fnSeek_interrupt;
	if(glbSeek_cylinder)
	{
		output_byte(FDC_SEEK);
		/* ���Ͳ���:��ͷ��+�������� */
		output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); 
		output_byte(glbSeek_cylinder);	/* ���Ͳ���:�ŵ��� */
	}
	else /* glbSeek_cylinder == 0,У��������ɸ�Ҫ�� */
	{
		output_byte(FDC_RECALIBRATE);
		/* ���Ͳ���:��ͷ��+�������� */
		output_byte((unsigned char)((glbHead << 2) | DRIVER_A)); 
	}
	/* ����output_byte()���ܲ���reset,Ҫ��reset FDC */
	if(flgNeed_Reset) 
		fnDo_fd_request();
}


/*=======================================================*
 *F: ������ʼ������
 *I: void
 *O: void
 *C: ��StartMain()�б�����
 *=======================================================*/
void fnFloppy_init()
{
	unsigned char val;

	glbBlk_devs[MAJOR_NR_FLY].pfn_doRequest = fnDo_fd_request;
	set_trap_gate(0x26 ,&floppy_interrupt);
	/* ��λ���̵�����λ */
	In_Byte(0x21, val)
	val &= ~0x40;
	Mask_8259AA(val)
}





