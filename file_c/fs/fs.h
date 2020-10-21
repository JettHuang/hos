/*
 *文件系统管理, 使用的一些数据结构. 希望尽可能地减少该模块与其它模块的关系.
 *							huangheshui  06/04/07   17:52
 */
#ifndef __FS_H
#define __FS_H

/* 设备名称与设备号映射关系:
 * 0 - unused (nodev)
 * 1 - /dev/mem		(不支持)
 * 2 - /dev/fd
 * 3 - /dev/hd
 * 其它的目前不支持
 */

#define MINODE_NR	64
#define SUPER_NR    8
#define ROOT_INO    1
#define FILE_NR     64

#define IS_BLKDEV(x)	((x) >= 2 && (x) <= 3)  /* 是不是块设备 */
#define FNAME_LEN	14   /* 文件名字长度 */
#define ROOT_INODE	1    /* 文件系统根inode号 */

#define I_MAP_SLOTS		8   /* inode 位图占用'块'的个数, 描述inode表的使用 */
#define Z_MAP_SLOTS		8   /* data bloks位图占用块'的个数, 描述data区的使用 */
#define SUPER_MAGIC		0x137F   /* 文件系统魔数 */

//文件节点的属性标志
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

/* 磁盘上的索引节点(inode)的结构定义 */
struct stDinode{
	unsigned short i_mode;  /* 文件类型和属性(rwx) */
	unsigned short i_uid;   /* 用户id */
	unsigned int  i_size;   /* 文件长度 */
	unsigned int  i_time;   /* 文件修改时间(自1970.1.1:0算起,秒) */
	unsigned char i_gid;    /* 组id(文件拥有者所在的组号) */
	unsigned char i_nlinks; /* 链接数 (该节点被多少个文件目录项指向、关联) */
	unsigned short i_zone[9];   /* 承载文件数据的blocks's nr */
};

/* 内存中的索引节点(inode)的结构定义 */
struct stMinode{
	unsigned short i_mode;  /* 文件类型和属性(rwx) */
	unsigned short i_uid;   /* 用户id */
	unsigned int i_size;    /* 文件长度 */
	unsigned int i_time;    /* 文件修改时间(自1970.1.1:0算起,秒) */
	unsigned char i_gid;    /* 组id(文件拥有者所在的组号) */
	unsigned char i_nlinks; /* 链接数 (该节点被多少个文件目录项指向、关联) */
	unsigned short i_zone[9];   /* 承载文件数据的blocks's nr */
	/* 下面是内存中特有的数据结构项 */
	struct PCB *i_wait;     /* 等待该inode的进行 */
	unsigned int i_atime;   /* 最后访问时间 */
	unsigned int i_ctime;   /* minode自身修改时间 */
	unsigned short i_dev;   /* inode所在的设备号 */
	unsigned short i_num;   /* inode在fs上的号 */
	unsigned short i_count; /* minode被使用的次数, 0表示该minode空闲 */
	unsigned char i_lock;   /* minode被锁定标志 */
	unsigned char i_dirt;   /* 已修改标志 */
	unsigned char i_pipe;   /* 管道标志 */
	unsigned char i_mount;  /* 安装点标志 */
	unsigned char i_seek;   /* 搜寻标志(lseek时) */
	unsigned char i_update; /* 更新标志(反映该节点数据是否合法) */
};

/* 文件结构(用于在文件句柄与minode之间建立关系) */
struct stFile{
	unsigned short f_mode;  /* 文件操作模式(rw bit) */
	unsigned short f_flags; /* 文件打开和控制的标志 */
	unsigned short f_count; /* 对应文件句柄(文件描述符)个数 */
	struct stMinode *f_inode;  /* 指向对应的minode */
	unsigned int f_offset;  /* 读写指针位移值 */
};

/* 磁盘上超级块结构的定义 */
struct stDSuperBlock{
	unsigned short s_ninodes;       /* 该系统中的dinode总数 */
	unsigned short s_nzones;        /* 该系统中的逻辑块(不仅仅data zones)总数 */
	unsigned short s_imap_blocks;   /* dinode位图占用的数据块个数 */
	unsigned short s_zmap_blocks;   /* data块位图占用的数据块个数 */
	unsigned short s_firstdatazone; /* 第一个数据块的逻辑号base 0 */
	unsigned short s_log_zone_size; /* log(数据块个数/设备上总块数) */
	unsigned int s_max_size;        /* 文件最大长度 */
	unsigned short s_magic;         /* 文件系统魔数 */
};

/* 内存中超级块结构的定义 */
struct stMSuperBlock{
	unsigned short s_ninodes;       /* 该系统中的dinode总数 */
	unsigned short s_nzones;        /* 该系统中的逻辑块(不仅仅data zones)总数 */
	unsigned short s_imap_blocks;   /* dinode位图占用的数据块个数 */
	unsigned short s_zmap_blocks;   /* data块位图占用的数据块个数 */
	unsigned short s_firstdatazone; /* 第一个数据块的逻辑号base 0 */
	unsigned short s_log_zone_size; /* log(数据块个数/设备上总块数) */
	unsigned int s_max_size;        /* 文件最大长度 */
	unsigned short s_magic;         /* 文件系统魔数 */
	/* 以下是内存中特有的数据项 */
	struct stBuf_Head * s_imap[8];  /* dinode位图在buffer中的bh指针数组 */
	struct stBuf_Head * s_zmap[8];  /* data块位图在buffer中的bh指针数组 */
	unsigned short s_dev;           /* 该超级块所在的设备号 */
	struct stMinode *s_isup;        /* 该文件系统的根目录的mindoe */
	struct stMinode *s_imount;      /* 该文件系统被挂到的目录minode */
	unsigned int s_time;            /* 修改时间 */
	struct PCB *s_wait;             /* 等待该超级块的进程 */
	unsigned char s_lock;           /* 被锁定标志 */
	unsigned char s_rd_only;        /* 该文件系统只读标志 */
	unsigned char s_dirt;           /* 已修改(脏)标志 */
};

/* 文件目录项结构定义(目录和文件都是文件) */
struct stDirEntry{
	unsigned short inode;   /* dinode的序号 */
	char name[FNAME_LEN];   /* (该项)文件名字 */
};

#define INODES_PER_BLOCK	(1024 / sizeof(struct stDinode))
#define DIR_ENTRIES_PER_BLOCK   (1024 / sizeof(struct stDirEntry))
#define MAJOR(a) (((unsigned short)(a)) >> 8)  /*高字节主设备号*/

#endif /* __FS_H */


















