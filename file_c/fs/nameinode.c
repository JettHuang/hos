/*-------------------------------------------------------------*
 * 该模块的功能是：对目录和目录项的操作；
 * 1，根据路径名找到目标接点；
 * 2，添加/删除目录和目录项。
 *						huangheshui  06/16/07 13:40
 *-------------------------------------------------------------*/
#include "fs.h"
#include "buffer.h"
#include "..\..\include\process.h"

extern struct stMSuperBlock * GetSuperblock(int dev);
extern void MiPut(struct stMinode *mi);
extern int Bmap(struct stMinode *mi, int fileblk);
extern struct stBuf_Head * fnread_bufblk(unsigned short dev, unsigned int block);
extern void fnRelse_bufblk(struct stBuf_Head *bh);

extern struct PCB *pCurrent;


/*=======================================================*
 *F: 检验文件名与目录项的名字是否相同。
 *I: int len, name, *entry 
 *O: 1-same, 0-not same
 *=======================================================*/
int MatchName(int len, const char *name, struct stDirEntry *de)
{
	int i;

	if(!de || !de->inode || len > FNAME_LEN)
		return 0;
	if(len < FNAME_LEN && de->name[len])
		return 0;
	//开始比较两个字符串
	for(i = 0; i < len; i++)
		if(*name++ != de->name[i])
			return 0;
	return 1;
}


/*=======================================================*
 *F: 在指定的目录节点下查找与输入名字匹配的目录项。
 *I: 指定目录的minode指针, name-文件名, namelen文件名长度
     ppentry-装有目录项指针的缓冲区指针,
 *O: bh-成功,目录项所在的块; 否则bh = NULL
 *C: 传递的参数必须保证是目录属性
 *=======================================================*/
struct stBuf_Head * FindEntry(struct stMinode * *dir, const char *name, int namelen,
							  struct stDirEntry * *ppentry)
{
	int entries;
	int block, i;
	struct stBuf_Head *bh;
	struct stDirEntry *de;
	struct stMSuperBlock *sb;

	if(!namelen || namelen > FNAME_LEN || !S_ISDIR((*dir)->i_mode))
		return 0;
	// 计算该节点目录下有多少个目录项
	entries = (*dir)->i_size / (sizeof(struct stDirEntry));
	*ppentry = 0;
	//开始逐步检查,对于'..'要考虑到挂载问题
	if(namelen = 2 && name[0] == '.' && name[1] == '.')
	{
		if((*dir) == pCurrent->root)
			namelen = 1;
		if((*dir)->i_num == ROOT_INO)
		{
			sb = GetSuperblock((*dir)->i_dev);
			if(sb->s_imount)
			{
				MiPut(*dir);
				(*dir) = sb->s_imount;  //我们找的是被安装点的'..'
				(*dir)->i_count++;
			}
		}
	}
	// 下面一定能够满足，至少存在'.', '..'
	if(!(block = (*dir)->i_zone[0]))
		return 0;
	// 读取目录这个特殊文件数据
	if(!(bh = fnread_bufblk((*dir)->i_dev, block)))
		return 0;
	i = 0;
	de = (struct stDirEntry *)bh->pb_data;
	while(i < entries)
	{
		if((unsigned char *)de >= bh->pb_data + BLOCK_SIZE)
		{
			fnRelse_bufblk(bh);
			bh = 0;
			//在读取下一个文件数据块
			if(!(block = Bmap(*dir, i / DIR_ENTRIES_PER_BLOCK) || 
				!(bh = fnread_bufblk((*dir)->i_dev, block))))
			{
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			de = (struct stDirEntry *)bh->pb_data;
		}
		if(MatchName(namelen, name, de))
		{
			*ppentry = de;
			return bh;
		}
		de++;
		i++;
	}
	//最终没有找到
	fnRelse_bufblk(bh);
	return 0;
}


/*=======================================================*
 *F: 将文件路径名最终映射成Minode,不区分目录与一般文件
 *I: 路径名
 *O: 内存inode
 *C: 路径名的格式可为 /huanghe/love 或 /huanghe/love/
 *=======================================================*/
struct stMinode * PathnametoMi(const char *pathname)
{

}










