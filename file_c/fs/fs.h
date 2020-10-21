/*
 *�ļ�ϵͳ����, ʹ�õ�һЩ���ݽṹ. ϣ�������ܵؼ��ٸ�ģ��������ģ��Ĺ�ϵ.
 *							huangheshui  06/04/07   17:52
 */
#ifndef __FS_H
#define __FS_H

/* �豸�������豸��ӳ���ϵ:
 * 0 - unused (nodev)
 * 1 - /dev/mem		(��֧��)
 * 2 - /dev/fd
 * 3 - /dev/hd
 * ������Ŀǰ��֧��
 */

#define MINODE_NR	64
#define SUPER_NR    8
#define ROOT_INO    1
#define FILE_NR     64

#define IS_BLKDEV(x)	((x) >= 2 && (x) <= 3)  /* �ǲ��ǿ��豸 */
#define FNAME_LEN	14   /* �ļ����ֳ��� */
#define ROOT_INODE	1    /* �ļ�ϵͳ��inode�� */

#define I_MAP_SLOTS		8   /* inode λͼռ��'��'�ĸ���, ����inode���ʹ�� */
#define Z_MAP_SLOTS		8   /* data bloksλͼռ�ÿ�'�ĸ���, ����data����ʹ�� */
#define SUPER_MAGIC		0x137F   /* �ļ�ϵͳħ�� */

//�ļ��ڵ�����Ա�־
#define S_IFMT  00170000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)

/* �����ϵ������ڵ�(inode)�Ľṹ���� */
struct stDinode{
	unsigned short i_mode;  /* �ļ����ͺ�����(rwx) */
	unsigned short i_uid;   /* �û�id */
	unsigned int  i_size;   /* �ļ����� */
	unsigned int  i_time;   /* �ļ��޸�ʱ��(��1970.1.1:0����,��) */
	unsigned char i_gid;    /* ��id(�ļ�ӵ�������ڵ����) */
	unsigned char i_nlinks; /* ������ (�ýڵ㱻���ٸ��ļ�Ŀ¼��ָ�򡢹���) */
	unsigned short i_zone[9];   /* �����ļ����ݵ�blocks's nr */
};

/* �ڴ��е������ڵ�(inode)�Ľṹ���� */
struct stMinode{
	unsigned short i_mode;  /* �ļ����ͺ�����(rwx) */
	unsigned short i_uid;   /* �û�id */
	unsigned int i_size;    /* �ļ����� */
	unsigned int i_time;    /* �ļ��޸�ʱ��(��1970.1.1:0����,��) */
	unsigned char i_gid;    /* ��id(�ļ�ӵ�������ڵ����) */
	unsigned char i_nlinks; /* ������ (�ýڵ㱻���ٸ��ļ�Ŀ¼��ָ�򡢹���) */
	unsigned short i_zone[9];   /* �����ļ����ݵ�blocks's nr */
	/* �������ڴ������е����ݽṹ�� */
	struct PCB *i_wait;     /* �ȴ���inode�Ľ��� */
	unsigned int i_atime;   /* ������ʱ�� */
	unsigned int i_ctime;   /* minode�����޸�ʱ�� */
	unsigned short i_dev;   /* inode���ڵ��豸�� */
	unsigned short i_num;   /* inode��fs�ϵĺ� */
	unsigned short i_count; /* minode��ʹ�õĴ���, 0��ʾ��minode���� */
	unsigned char i_lock;   /* minode��������־ */
	unsigned char i_dirt;   /* ���޸ı�־ */
	unsigned char i_pipe;   /* �ܵ���־ */
	unsigned char i_mount;  /* ��װ���־ */
	unsigned char i_seek;   /* ��Ѱ��־(lseekʱ) */
	unsigned char i_update; /* ���±�־(��ӳ�ýڵ������Ƿ�Ϸ�) */
};

/* �ļ��ṹ(�������ļ������minode֮�佨����ϵ) */
struct stFile{
	unsigned short f_mode;  /* �ļ�����ģʽ(rw bit) */
	unsigned short f_flags; /* �ļ��򿪺Ϳ��Ƶı�־ */
	unsigned short f_count; /* ��Ӧ�ļ����(�ļ�������)���� */
	struct stMinode *f_inode;  /* ָ���Ӧ��minode */
	unsigned int f_offset;  /* ��дָ��λ��ֵ */
};

/* �����ϳ�����ṹ�Ķ��� */
struct stDSuperBlock{
	unsigned short s_ninodes;       /* ��ϵͳ�е�dinode���� */
	unsigned short s_nzones;        /* ��ϵͳ�е��߼���(������data zones)���� */
	unsigned short s_imap_blocks;   /* dinodeλͼռ�õ����ݿ���� */
	unsigned short s_zmap_blocks;   /* data��λͼռ�õ����ݿ���� */
	unsigned short s_firstdatazone; /* ��һ�����ݿ���߼���base 0 */
	unsigned short s_log_zone_size; /* log(���ݿ����/�豸���ܿ���) */
	unsigned int s_max_size;        /* �ļ���󳤶� */
	unsigned short s_magic;         /* �ļ�ϵͳħ�� */
};

/* �ڴ��г�����ṹ�Ķ��� */
struct stMSuperBlock{
	unsigned short s_ninodes;       /* ��ϵͳ�е�dinode���� */
	unsigned short s_nzones;        /* ��ϵͳ�е��߼���(������data zones)���� */
	unsigned short s_imap_blocks;   /* dinodeλͼռ�õ����ݿ���� */
	unsigned short s_zmap_blocks;   /* data��λͼռ�õ����ݿ���� */
	unsigned short s_firstdatazone; /* ��һ�����ݿ���߼���base 0 */
	unsigned short s_log_zone_size; /* log(���ݿ����/�豸���ܿ���) */
	unsigned int s_max_size;        /* �ļ���󳤶� */
	unsigned short s_magic;         /* �ļ�ϵͳħ�� */
	/* �������ڴ������е������� */
	struct stBuf_Head * s_imap[8];  /* dinodeλͼ��buffer�е�bhָ������ */
	struct stBuf_Head * s_zmap[8];  /* data��λͼ��buffer�е�bhָ������ */
	unsigned short s_dev;           /* �ó��������ڵ��豸�� */
	struct stMinode *s_isup;        /* ���ļ�ϵͳ�ĸ�Ŀ¼��mindoe */
	struct stMinode *s_imount;      /* ���ļ�ϵͳ���ҵ���Ŀ¼minode */
	unsigned int s_time;            /* �޸�ʱ�� */
	struct PCB *s_wait;             /* �ȴ��ó�����Ľ��� */
	unsigned char s_lock;           /* ��������־ */
	unsigned char s_rd_only;        /* ���ļ�ϵͳֻ����־ */
	unsigned char s_dirt;           /* ���޸�(��)��־ */
};

/* �ļ�Ŀ¼��ṹ����(Ŀ¼���ļ������ļ�) */
struct stDirEntry{
	unsigned short inode;   /* dinode����� */
	char name[FNAME_LEN];   /* (����)�ļ����� */
};

#define INODES_PER_BLOCK	(1024 / sizeof(struct stDinode))
#define DIR_ENTRIES_PER_BLOCK   (1024 / sizeof(struct stDirEntry))
#define MAJOR(a) (((unsigned short)(a)) >> 8)  /*���ֽ����豸��*/

#endif /* __FS_H */


















