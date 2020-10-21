/*----------------------------------------------------------*
 *˵��: ATӲ����������
 *				huanghehsui 05/30/07 14:33
 *----------------------------------------------------------*/
#include  "..\include\asm.h"
#include  "..\include\color.h"
#include  "..\include\hd.h"
#include  "..\include\blk.h"

extern void hd_interrupt();
extern struct stBlk_dev_IO  glbBlk_devs[NR_BLK_DEV];
extern void printk(unsigned long color, const char *lpstr);
extern void fnEnd_request(int major, int update);
extern void set_trap_gate(int index, void (*fnoffset)());

/*-----------------------------��ģ���к���������ʼ-------------------------------*/
void fnGet_hds_Info();
int fnIsController_Ready();
int fnWin_Result();
int fnIsDrive_Busy();
void fnHd_out(unsigned int drive, unsigned int nsect, unsigned int sect,
			  unsigned int head, unsigned int cyl, unsigned int cmd, 
			  void (* intr_addr)());
void fnReset_Controller();
void fnReset_hd(unsigned int nr);
void fnRecal_intr();
void bad_rw_intr();
void fnRead_intr();
void fnWrite_intr();
void fnDo_hd_request();
/*-----------------------------��ģ���к���������ʼ-------------------------------*/

#define	COMPLIER_BASEADDR  0xC0000000
#define CURRENT	  (glbBlk_devs[MAJOR_NR_HD].current_request)
/* Ӳ�̲��������ͽṹ */
struct stHDtype{
	int head;      /* ��ͷ���� */
	int	sector;    /* ÿ�ŵ��������� */    
	int	cylinder;  /* ������ */
	int	wpcom;     /* дǰԤ��������� */
	int	lzone;     /* ��ͷ��½������� */
	int	ctrl;      /* �����ֽ� */
}glbHDs_info[MAX_HD];

/* Ӳ�̵ķ����ṹ,ÿ��Ӳ�̷�4����, glbHDparts[0]��glbHDparts[5]��������Ӳ�� */
struct stHDpart{
	unsigned int start_sector;  /* ��ʼ������, base 0 */
	unsigned int nr_sectors;    /* �÷���ռ���������� */
}glbHDparts[5 * MAX_HD] = {{0, 0}, };

int HD_BIOS = (0x90080 + COMPLIER_BASEADDR);   /* setup�б���Ĳ�����0�ĵ�ַ */
unsigned int glbHds_nr;  /* Ӳ�̸��� */
void (* pfnDo_hd)();      /* Ӳ���жϴ������ָ�� */
static int flgNeed_Reset = 1;
static int flgNeed_Recalibrate = 1;


/*=====================================================*
 *F: ��ȡϵͳ��Ӳ�̲�����,������֮(��hd_init()�е�����)
 *=====================================================*/
void fnGet_hds_Info()
{
	unsigned int i, drive;
	unsigned char cmos_hd;

	for(drive = 0; drive < MAX_HD; drive++)
	{
		glbHDs_info[drive].cylinder = *(unsigned short *)HD_BIOS;
		glbHDs_info[drive].head = *(unsigned char *)(2 + HD_BIOS);
		glbHDs_info[drive].wpcom = *(unsigned short *)(5 + HD_BIOS);
		glbHDs_info[drive].ctrl = *(unsigned char *)(8 + HD_BIOS);
		glbHDs_info[drive].lzone = *(unsigned short *)(12 + HD_BIOS);
		glbHDs_info[drive].sector = *(unsigned char *)(14 + HD_BIOS);
		HD_BIOS += 16;  /* ��һ�ű� */
	}
	if(glbHDs_info[1].cylinder)
		glbHds_nr = 2;  /* ������Ӳ�� */
	else
		glbHds_nr = 1;
	/* �����ÿ��Ӳ�̵���ʼ�����š��������� */
	for(i = 0; i < glbHds_nr; i++)
	{
		glbHDparts[i * 5].start_sector = 0;  /* ��0��ʼ����(�߼��ϵ���) */
		glbHDparts[i * 5].nr_sectors = glbHDs_info[i].cylinder * 
			glbHDs_info[i].head * glbHDs_info[i].sector;  /* �������� */
	}
	/* �ٴ�ȷ��ϵͳ�е�Ӳ���Ƿ�AT���ݵ�(�ο�p141, line124) */
	CMOS_READ(0x12, cmos_hd);
	if(cmos_hd & 0xf0)     /* ����hd0 */
	{
		if(cmos_hd & 0x0f) /* ����hd1 */
			glbHds_nr = 2;
		else
			glbHds_nr = 1;
	}
	else
	{
		glbHds_nr = 0;  /* û�м��ݵ�Ӳ�� */
		printk(RED, "not find compatible hard disk.");
	}
	/* �������ݵ�Ӳ�̷����� */
	for(i = glbHds_nr; i < 2; i++)
	{
		glbHDparts[i * 5].start_sector = 0;
		glbHDparts[i * 5].nr_sectors = 0;
	}	    
}

/*=====================================================*
 *���Ӳ�̷�����,�ú������ȡӲ�̣����԰����ڽ���1��
 *��ʱ��ʵ����.
 *=====================================================*/



/*============================================================*
 *F: ѭ���ȴ�Ӳ�̿�����(HDC)����!��������æ��,�Ҳ�֪��Ϊʲô
 *   �����STATUS_READY(drive)?
 *O: 0--æ(��ʱ), !0--����
 *============================================================*/
int fnIsController_Ready()
{
	int retries = 10000;
	unsigned char status;

	do{
		In_Byte(HDC_STATUS_PORT, status)
	}while(--retries && (status & (STATUS_BUSY | STATUS_READY)) != STATUS_READY);
	return retries;  /* ����ʣ��ֵ */
}


/*=====================================================*
 *F: ���HDCִ��������״̬,ͨ����ȡ״̬�Ĵ����еĽ��״̬��
 *I: void
 *O: 0--ִ������, 1--ִ�г���
 *=====================================================*/
int fnWin_Result()
{
	unsigned char status;

	In_Byte(HDC_STATUS_PORT, status)
	if((status & (STATUS_BUSY | STATUS_READY | STATUS_WFAULT | STATUS_ERROR))
		== STATUS_READY)
		return 0;
	if(status & STATUS_ERROR)  /* ����ִ�д��� */
	{
		In_Byte(HDC_ERROR_PORT, status)  /* ������ֵ�����! */
	}
	return 1;
}


/*==============================================================*
 *F: �ȴ�Ӳ�̾���(hdc && drive),������fnIsController_Ready()��ͬ
 *I: void
 *O: 0-ready, 1-�ȴ���ʱ
 *==============================================================*/
int fnIsDrive_Busy()
{
	int i;
	unsigned char status;

	for(i = 0; i < 10000; i++)
	{
		In_Byte(HDC_STATUS_PORT, status)
		if(STATUS_READY == (status & (STATUS_BUSY | STATUS_READY)))
			break;
	}
	/* �ٴζ��ղŵ�״ֵ̬ */
	In_Byte(HDC_STATUS_PORT, status)
	status &= STATUS_BUSY | STATUS_READY;
	if(status == STATUS_READY)
		return 0;
	printk(RED, "fnIsDrive_Busy() times out!");
	return 1;
}


/*=======================================================*
 *F: ��Ӳ�̿�����(HDC)���������,ȫ����7B,6������+1������
 *I: ��
 *O: void
 *=======================================================*/
void fnHd_out(unsigned int drive, unsigned int nsect, unsigned int sect,
			  unsigned int head, unsigned int cyl, unsigned int cmd, 
			  void (* intr_addr)())
{
	unsigned char param;

	if(drive > 1 || head > 15)
		printk(RED, "fnHd_out(): error!");
	if(!fnIsController_Ready())
		printk(RED, "fnHd_out(): HD controller error!");
	pfnDo_hd = intr_addr;
	/* ����:�����ֽ� */
	param = (unsigned char)glbHDs_info[drive].ctrl;  
	Out_Byte(HDC_CTRL_PORT, param)
	/* ����:дԤ���������( >> 2) */
	param = (unsigned char)(glbHDs_info[drive].wpcom >> 2); 
	Out_Byte(HDC_PRECOMP_PORT, param)
	/* ����:��|д�������� */
	param = (unsigned char)nsect;  
	Out_Byte(HDC_NR_SECTOR_PORT, param)
	/* ����:��ʼ���� */
	param = (unsigned char)sect;  
	Out_Byte(HDC_START_SEC_PORT, param)
	/* ����:����ŵ�8λ */
	param = (unsigned char)cyl;         
	Out_Byte(HDC_CYL_LOW_PORT, param)
	/* ����:����Ÿ�8λ */
	param = (unsigned char)(cyl >> 8);  
	Out_Byte(HDC_CYL_HIGH_PORT, param)
	/* ����:�������� + ��ͷ�� */
	param = (unsigned char)(0xa0 | (drive << 4) | head); 
	Out_Byte(HDC_CURRENT_PORT, param)
	/* ���������� */
	param = (unsigned char)cmd;
	Out_Byte(HDC_COMMAND_PORT, param)
}


/*============================================================*
 *F: ��ϸ�λ(����У��)Ӳ�̿�����(HDC),û���жϴ�������Ӧ֮
 *I: void
 *O: void
 *============================================================*/
void fnReset_Controller()
{
	int i;
	unsigned char tmp;

	/* ����ƶ˿ڷ���(4-��λ) */
	Out_Byte(HDC_CTRL_PORT, 0x04)
	for(i = 0; i < 100; i++)  /* ��ʱ�ȴ� */
		NOP()
	tmp = (unsigned char)(glbHDs_info[0].ctrl & 0x0f);  /* ����ֹ���ԡ��ض� */
	Out_Byte(HDC_CTRL_PORT, tmp)
	if(fnIsDrive_Busy())
		printk(RED, "fnReset_Controller(): drive busy!");
	/* ������Ĵ��� */
	In_Byte(HDC_ERROR_PORT, tmp)
	if(tmp != 0x01)
		printk(RED, "fnReset_Controller(): reset failed!");
}


/*=====================================================*
 *F: ��λָ����Ӳ��nr, ��У��HDC,�ٷ���Ӳ�̿���������-->
 * ������������������
 *I: nr-��������
 *O: void
 *=====================================================*/
void fnReset_hd(unsigned int nr)
{
	fnReset_Controller();
	fnHd_out(nr, glbHDs_info[nr].sector, glbHDs_info[nr].sector,
		glbHDs_info[nr].head - 1, glbHDs_info[nr].cylinder, 
		CMD_SPECIFY, &fnRecal_intr);  /* ����У���жϴ����� */
}


/*===========================================================*
 *F: У���жϴ�����.
 *C: ���Ӳ�̿��������ش�����Ϣ,�����ȵ���bad_rw_intr(),Ȼ��
 *   ����fnDo_hd_request()�е�"��λ"���ܻ����������һ������.
 *===========================================================*/
void fnRecal_intr()
{
	if(fnWin_Result())  /* ����ִ��ʧ�� */
		bad_rw_intr();
	/* ��������´���һ������*/
	fnDo_hd_request();
}


/*=====================================================*
 *F: ��дӲ��ʧ�ܴ�����.
 *I: void
 *O: void
 *=====================================================*/
void bad_rw_intr()
{
	CURRENT->errors++;
	if(CURRENT->errors >= MAX_ERRORS)
		fnEnd_request(MAJOR_NR_HD, 0);  /* û�и������� */
	if(CURRENT->errors >= MAX_ERRORS / 2)
		flgNeed_Reset = 1;              /* ��Ҫ����fnReset_Controller() */
}


/*=====================================================*
 *F: HDC������, �жϴ������
 *=====================================================*/
void fnRead_intr()
{
	unsigned char *tmp;

	/* �������ִ�еĽ�� */
	if(fnWin_Result())
	{
		bad_rw_intr();
		/* ��������´���һ������*/
		fnDo_hd_request();
		return;
	}
	tmp = CURRENT->pdata_buf;
	/* �����ݶ�������ṹ.������ */
	Read_port(HDC_DATA_PORT, tmp, 256)
	CURRENT->errors = 0;
	if(--CURRENT->nr_sectors)       /* ��û�ж��� */
	{
		CURRENT->pdata_buf += 512;  /* ��һ������ */
		CURRENT->begin_sector++;
		pfnDo_hd = &fnRead_intr;
		return;
	}
	fnEnd_request(MAJOR_NR_HD, 1);  /* �������� */
	fnDo_hd_request();              /* ������һ�������� */
}


/*=====================================================*
 *F: HDCд����, �жϴ������
 *=====================================================*/
void fnWrite_intr()
{
	unsigned char *tmp;

	/* �������ִ�еĽ�� */
	if(fnWin_Result())
	{
		bad_rw_intr();
		/* ��������´���һ������ */
		fnDo_hd_request();
		return;
	}
	CURRENT->errors = 0;
	if(--CURRENT->nr_sectors)  /* ��û��д�� */
	{
		CURRENT->pdata_buf += 512;  /* ��һ������ */
		CURRENT->begin_sector++;
		pfnDo_hd = &fnWrite_intr;
		/* д��һ������ */
		tmp = CURRENT->pdata_buf;
		Write_port(HDC_DATA_PORT, tmp, 256)
		return;
	}
	fnEnd_request(MAJOR_NR_HD, 1);  /* ���ݺϷ� */
	fnDo_hd_request();  /* ������һ�������� */
}


/*=====================================================*
 *F: ����Ӳ�̲�������
 *fnDo_hd_request()��floppy.c�е�fnDo_fd_request()�Ľṹ
 *����,��������Ҳ����.
 *=====================================================*/
void fnDo_hd_request()
{
	int i;
	char *tmp;
	unsigned char status;
	unsigned int block, dev;
	unsigned int sect, head, cyl;  /* ʵ�ʵ������š���ͷ�š������ */
	unsigned int nr_sect;

	/* ���CURRENT������ĺϷ��� */
_repeat:
	if(!CURRENT)
		return;
	if(MAJOR(CURRENT->dev) != MAJOR_NR_HD)
		printk(RED, "fnDo_hd_request():dev is wrong!");
	dev = MINOR(CURRENT->dev);      /*���豸��(������)*/
	block = CURRENT->begin_sector;  /* �����е����������, base 0 */
	if(dev >= 5 * glbHds_nr || (block + 2) > glbHDparts[dev].nr_sectors)
	{
		fnEnd_request(MAJOR_NR_HD, 0);  /* ���ݷǷ� */
		goto _repeat;
	}
	block += glbHDparts[dev].start_sector;  /* �������ĳӲ�̵ľ��������� base 0 */
	dev /= 5;  /* dev ��Ӳ��0��1 */
	/* �������߼�������(base 0), ת����ʵ�ʵ����桢��ͷ���ŵ�����(base 1) */
	sect = block % glbHDs_info[dev].sector;
	sect++;
	block /= glbHDs_info[dev].sector;      /* �߼��ŵ��� */
	head = block % glbHDs_info[dev].head;  /* ��ͷ��base 0 */
	cyl = block / glbHDs_info[dev].head;   /* �����base 0 */
	nr_sect = CURRENT->nr_sectors;         /* ��д�������� */
	if(flgNeed_Reset)
	{
		flgNeed_Reset = 0;
		flgNeed_Recalibrate = 1;
		fnReset_hd(MINOR(CURRENT->dev) / 5);  /* Ҳ����dev */
		return;
	}
	if(flgNeed_Recalibrate)
	{
		flgNeed_Recalibrate = 0;
		fnHd_out(dev, glbHDs_info[dev].sector, 0, 0, 0, CMD_RECALIBRATE, &fnRecal_intr);
		return;
	}
	if(CURRENT->cmd == WRITE)
	{
		fnHd_out(dev, nr_sect, sect, head, cyl, CMD_WRITE, &fnWrite_intr);
		In_Byte(HDC_STATUS_PORT, status)
		for(i = 0; i < 3000 && !(status & STATUS_DATA_REQ); i++)
			In_Byte(HDC_STATUS_PORT, status)
		/* �������û����λ, ��дӲ�̲���ʧ�� */
		if(!(status & STATUS_DATA_REQ))
		{
			bad_rw_intr();
			goto _repeat;  /* �ٴ���һ�� */
		}
		Write_port(HDC_DATA_PORT, tmp, 256)
	}
	else if(CURRENT->cmd == WRITE)
	{
		fnHd_out(dev, nr_sect, sect, head, cyl, CMD_READ, &fnRead_intr);
	}
	else
	{
		printk(RED, "fnDo_hd_request(): CURRENT->cmd error!");
	}
}


/*=====================================================*
 *F: Ӳ�̳�ʼ�� 
 *C: ��StartMain()�е���
 *=====================================================*/
void fnHd_init()
{
	unsigned char val;

	fnGet_hds_Info();  /* ע��: һ����BIOS���ݱ��ƻ�ǰִ�� */
	glbBlk_devs[MAJOR_NR_HD].pfn_doRequest = fnDo_hd_request;
	set_trap_gate(0x2e ,&hd_interrupt);
	/* ��λ8259a��Ƭ��int2����λ */
	In_Byte(0x21, val)
	val &= 0xfb;
	Mask_8259AA(val)
	/* ��λ8259a��Ƭ��int14����λ */
	In_Byte(0xa1, val)
	val &= 0xbf;
	Mask_8259AB(val)
}








