/*-------------------------------------------------------------*
 * ��ģ��Ĺ����ǣ���Ŀ¼��Ŀ¼��Ĳ�����
 * 1������·�����ҵ�Ŀ��ӵ㣻
 * 2�����/ɾ��Ŀ¼��Ŀ¼�
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
 *F: �����ļ�����Ŀ¼��������Ƿ���ͬ��
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
	//��ʼ�Ƚ������ַ���
	for(i = 0; i < len; i++)
		if(*name++ != de->name[i])
			return 0;
	return 1;
}


/*=======================================================*
 *F: ��ָ����Ŀ¼�ڵ��²�������������ƥ���Ŀ¼�
 *I: ָ��Ŀ¼��minodeָ��, name-�ļ���, namelen�ļ�������
     ppentry-װ��Ŀ¼��ָ��Ļ�����ָ��,
 *O: bh-�ɹ�,Ŀ¼�����ڵĿ�; ����bh = NULL
 *C: ���ݵĲ������뱣֤��Ŀ¼����
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
	// ����ýڵ�Ŀ¼���ж��ٸ�Ŀ¼��
	entries = (*dir)->i_size / (sizeof(struct stDirEntry));
	*ppentry = 0;
	//��ʼ�𲽼��,����'..'Ҫ���ǵ���������
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
				(*dir) = sb->s_imount;  //�����ҵ��Ǳ���װ���'..'
				(*dir)->i_count++;
			}
		}
	}
	// ����һ���ܹ����㣬���ٴ���'.', '..'
	if(!(block = (*dir)->i_zone[0]))
		return 0;
	// ��ȡĿ¼��������ļ�����
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
			//�ڶ�ȡ��һ���ļ����ݿ�
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
	//����û���ҵ�
	fnRelse_bufblk(bh);
	return 0;
}


/*=======================================================*
 *F: ���ļ�·��������ӳ���Minode,������Ŀ¼��һ���ļ�
 *I: ·����
 *O: �ڴ�inode
 *C: ·�����ĸ�ʽ��Ϊ /huanghe/love �� /huanghe/love/
 *=======================================================*/
struct stMinode * PathnametoMi(const char *pathname)
{

}










