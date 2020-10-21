/*-------------------------------------------------*
 *�������̲���1.44 MB
 *			huangheshui	05/25/07  21:45
 *-------------------------------------------------*/
#ifndef __FLOPPY_H
#define __FLOPPY_H

/* ���̿������Ķ˿� */
#define FDC_DOR_PORT		0x3f2   /* ��������Ĵ��� */
#define FDC_STATUS_PORT		0x3f4   /* ��״̬�Ĵ��� */
#define FDC_DATA_PORT		0x3f5   /* ���ݡ�����˿� */
#define FDC_DIR_PORT		0x3f7	/* ��������Ĵ��� */
#define FDC_DCR_PORT		0x3f7	/* �����ʿ��ƼĴ��� */

/* DMA�˿� */
#define DMA_ADDR			0x004	/* port for low 16 bits of DMA address */
#define DMA_TOP				0x081	/* port for top 4 bits of 20-bit DMA addr */
#define DMA_COUNT			0x005	/* port for DMA count (count =  bytes - 1) */
#define DMA_FLIPFLOP		0x00C	/* DMA byte pointer flip-flop */
#define DMA_MODE			0x00B	/* DMA mode port */
#define DMA_INIT			0x00A	/* DMA init port */
#define DMA_RESET_VAL		0x006

/* ��״̬�Ĵ�����bit���� */
#define STATUS_DRIVER_BUSYMASK	0x0f /* ������æ */
#define STATUS_FDC_BUSY			0x10 /* ���̿�����æ */
#define STATUS_DMA_USED			0x20 /* 0 -- ʹ��DMA */
#define STATUS_DIRECTION		0x40 /* ���ݴ��䷽�� 0:cpu-->FDC, 1:FDC-->cpu */
#define STATUS_DATA_READY		0x80 /* ���ݼĴ������� */

/* ״̬�ֽ�0(ST0)��������λ�ĺ��� */
#define ST0_DS		0x03 /* ������ѡ���(�����ж�ʱ) */
#define ST0_HA		0x04 /* ��ͷ�� */
#define ST0_NR		0x08 /* ������δ׼���� */
#define ST0_ECE		0x10 /* �豸������(���У׼����) */
#define ST0_SE		0x20 /* seek end Ѱ��������У������ִ�н��� */ 
#define ST0_INTR	0xc0 /* �жϴ�������λ */

/* ״̬�ֽ�1(ST1)��������λ�ĺ��� */
#define ST1_MAM		0x01 /* missing address mark, û�ҵ���ַ��־ */
#define ST1_WP		0x02 /* write protect, д���� */
#define ST1_ND		0x04 /* No data--unreadable, û�ҵ�ָ�������� */
#define ST1_OR		0x10 /* over run, ���ݴ��䳬ʱ(DMA����) */
#define ST1_CRC		0x20 /* CRC������� */
#define ST1_EOC		0x80 /* ���ʳ����ŵ������������ */

/* ״̬�ֽ�2(ST2)��������λ�ĺ��� */ 
#define ST2_MAM		0x01 /* missing address mark, û�ҵ���ַ��־ */
#define ST2_BC		0x02 /* Bad cylinder, �ŵ��� */
#define ST2_SNS		0x04 /* scan not satisfied, ����(ɨ��)�������� */
#define ST2_SEH		0x08 /* scan equal hit, ������������ */
#define ST2_WC		0x10 /* wrong cylinder, �ŵ�(�����)���� */
#define ST2_CRC		0x20 /* ���ݳ�CRCУ��� */
#define ST2_CM		0x40 /* control mark = deleted, ����������ɾ����־ */

/* ״̬�ֽ�3(ST3)��������λ�ĺ��� */ 
#define ST3_HA		0x04 /* head address, ��ͷ�� */
#define ST3_TZ		0x10 /* track zero sign, 1=track 0 ��ŵ��ź� */
#define ST3_WP		0x40 /* д����*/

/*------FDC_COMMAND���������� -------*/
#define FDC_RECALIBRATE		0x07 /* move to track 0, ����У��(��ͷ�˵�0�ŵ�) */
#define FDC_SEEK			0x0f /* seek track, ��ͷѰ�� */
#define FDC_READ			0xe6 /* ������(MT��ŵ�������MFM��ʽ������ɾ������) */
#define FDC_WRITE			0xc5 /* д����(MT,MFM) */
#define FDC_SENSEI			0x08 /* ����ж�״̬ */
#define FDC_SPECIFY			0x03 /* �趨����������(�������ʡ���ͷж��ʱ���) */

/* DMA command */
#define DMA_READ			0x46 /* DMA���� */
#define DMA_WRITE			0x4A /* DMAд�� */

/* A�̵綯����� */
#define MOTOR_A_RUNING		0x10
#define	DRIVER_A_SELECT		0x00
#define DRIVER_A			0x00

#define MAX_REPLIES			0x07 /* ���7B���ؽ�� */
#define MAJOR_NR_FLY		0x02 /* floppy major nr */
#define MAX_ERRORS			0x08 /* ����������8�δ��� */

#endif /* __FLOPPY_H */
















