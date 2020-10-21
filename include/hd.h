/*----------------------------------------------------------*
 * Ӳ����������ͷ�ļ�
 *
 *----------------------------------------------------------*/
#ifndef	__HARDDISK_H
#define __HARDDISK_H

/* Ӳ�̿�����(HDC)�Ĵ����˿� */
#define  HDC_BASE0_PORT		 0x1f0   /* base register of controller 0 */
#define  HDC_DATA_PORT		 0x1f0   /* ���ݶ˿� */
#define  HDC_PRECOMP_PORT	 0x1f1   /* Ӳ��дǰԤ����, д */
#define  HDC_ERROR_PORT		 0x1f1   /* error code, �� */
#define  HDC_NR_SECTOR_PORT  0x1f2   /* Ҫ��/д���������� */
#define  HDC_START_SEC_PORT  0x1f3   /* ��ʼ������ */
#define  HDC_CYL_LOW_PORT	 0x1f4   /* low byte of start cylinder number */
#define  HDC_CYL_HIGH_PORT	 0x1f5   /* high byte of start cylinder number */
#define  HDC_CURRENT_PORT	 0x1f6   /* 101dhhhh, d=drive, hhhh=head */
#define  HDC_STATUS_PORT	 0x1f7   /* HDC��״̬, �� */
#define  HDC_COMMAND_PORT	 0x1f7   /* ��HDCд����˿�, д */

#define  HDC_CTRL_PORT		 0x3f6   /* HDC���ƼĴ����˿� */

/* HDC״̬�Ĵ����ĸ���bit���� */
#define  STATUS_BUSY		0x80	/* controller busy */
#define	 STATUS_READY		0x40	/* drive ready */
#define	 STATUS_WFAULT		0x20	/* write fault */
#define	 STATUS_SEEK_OVER	0x10	/* seek complete (obsolete) �ö˿ڻķ� */
#define	 STATUS_DATA_REQ	0x08	/* data transfer request */
#define	 STATUS_CR_DATA	    0x04	/* corrected data, ECCУ����� */
#define	 STATUS_INDEX		0x02	/* index pulse, �յ����� */
#define	 STATUS_ERROR		0x01	/* error, ����ִ�д��� */

/* HDC�ĸ��������� */
#define  CMD_IDLE			0x00	/* for w_command: drive idle */
#define  CMD_RECALIBRATE	0x10	/* recalibrate drive(����������У������λ) */
#define  CMD_READ			0x20	/* read data */
#define  CMD_WRITE			0x30	/* write data */
#define  CMD_VERIFY	        0x40	/* read verify, ������� */
#define  CMD_FORMAT		    0x50	/* format track, ��ʽ���ŵ� */
#define  CMD_SEEK			0x70	/* seek cylinder, Ѱ�� */
#define  CMD_DIAG			0x90	/* execute device diagnostics, ��������� */
#define  CMD_SPECIFY		0x91	/* specify parameters, �������������� */

/* ����Ĵ�������bitλ������ */
#define	 ERROR_BAD_BLOCK	0x80	/* bad block */
#define	 ERROR_BAD_ECC		0x40	/* bad ecc bytes */
#define	 ERROR_NO_ID		0x10	/* id not found */
#define	 ERROR_ACMD		    0x04	/* aborted command */
#define	 ERROR_TRACK_ZERO	0x02	/* track zero error */
#define	 ERROR_NO_DMARK	    0x01	/* no data address mark */

/* Ӳ�̷�����ṹ */
struct stPartition{
	unsigned char boot_ind;  /* 0x80--active */
	unsigned char sector;
	unsigned char cylinder;
	unsigned char sys_ind;
	unsigned char end_head;
	unsigned char end_sector;
	unsigned char end_cylinder;
	unsigned int start_sector;  /* starting sector ��0��ʼ���� */
	unsigned int nr_sectors;    /* �÷������������� */
};

/* others */
#define  MAX_ERRORS		0x07  /* ��/дһ������ʱ�������������� */
#define  MAX_HD			0x02  /* ϵͳ���֧�ֵ�Ӳ�̸��� */
#define  MAJOR_NR_HD	0x03  /* Ӳ�̵����豸�� */

/*
 * ����ĺ궨���˴�(��)HDC���ݶ˿ڶ�(д)����
 * port--�˿ں�, pbuf--��������ַ, nr--�ֵĸ���
 */
/* �� port --> es:edi */
#define  Read_port(port, pbuf, nr) \
__asm  mov ecx, pbuf \
__asm  mov edi, ecx \
__asm  mov ecx, nr \
__asm  mov dx, port \
__asm  cld \
__asm  rep insw

/* д ds:esi --> port */
#define  Write_port(port, pbuf, nr) \
__asm  mov ecx, pbuf \
__asm  mov esi, ecx \
__asm  mov ecx, nr \
__asm  mov dx, port \
__asm  cld \
__asm  rep outsw

/* ��CMOS�еĲ���byte,disp--����ƫ�ơ�retv--����ֵ */
#define  CMOS_READ(disp, retv) \
__asm  mov al, disp \
__asm  out 70h, al \
__asm  in al, 71h \
__asm  mov retv, al


#endif /* __HARDDISK_H */

/*-----------------------����֪ʶ��ʼ-----------------------*
Ӳ�̲���������(��ȫ)

 Offset Size	    Description

	  00   word  maximum number of cylinders
	  02   byte  maximum number of heads
	  03   word  starting reduced write current cylinder
	  05   word  starting write pre-comp cylinder
	  07   byte  maximum ECC data burst length
	  08   byte  control byte:
 *-----------------------����֪ʶ����-----------------------*/



















