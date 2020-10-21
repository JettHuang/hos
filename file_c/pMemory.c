/*-------------------------------------------------------------*
 * ˵����  pMemory.c �ļ�ʵ�ֵ������ڴ�����һЩ������������
 *	    1������һ�������ڴ�ҳ��
 *	    2���ͷ�һ�������ڴ�ҳ��
 *	    ���ڲ���һ��ҳ����ڴ�Ĳ����У�
 *	    3������ָ����С���ڴ�� fnPm_alloc(...)
 *	    4���ͷ�ָ��λ�õ��ڴ�� fnPm_free(...)
 *      ����������һЩ�����Ժ�����
 * ��ע��  ����ĺ����������ں˿ռ��з����ڴ�ģ����õ����ڴ��ַ
 *      �����Ե�ַ����ת��Ϊʵ�ʵ������ַ��Ҫ retValue - COMPLIER_BASEADDR
 *	    Pm ---physical memory 
 *	    Vmk ---virtual Kernel memory / line Kernel memory	 
 * Ӧ�ü�ס��
 *	   �����ڴ�0 -- 256M  =======>  �ں˵�ַ 0xC0000000 -- 0xD0000000
 *
 * �޸�ʱ�䣺04/10/07  20:30 
 *           2007-06-16 21:03
 *-------------------------------------------------------------*/
#include "..\include\pMemory.h"
#include "..\include\color.h"
#include "..\include\process.h"

extern void vsprintf(char *buf, char *fmt, ...);
extern void printk(unsigned long color, const char *lpstr);
extern void fnWakeUp(struct PCB * *pwait);
extern struct PCB *glbWaitMMPage;

#define	PAGE_SIZE	        4096	
#define	KERNEL_PAGES	    64  //����Ŀǰ�ں�ռ��64pages
#define	COMPLIER_BASEADDR	0xC0000000
#define BITS_CHAR	(sizeof(unsigned char) * 8) 
//˵���� �����ڴ�ҳ��ʹ�÷���λͼ��
//	  ÿ��bit��ʾһ������ҳ�棬bit == 1 ��ʾ��ռ�ã�bit == 0 ��ʾûռ�á�
//    �ɱ�ʾ�������ڴ��С�� 8 * 1024 * 8 * 4k = 256MB
//
unsigned char glbPm_Bitmap[8*1024];	
unsigned int uglbTotal_PmPages;   //�����ڴ�ҳ������


/*=============================================*
 *F: ����һ�������ڴ�ҳ��
 *I: void 
 *O: 0-����ʧ�ܣ�!0-�ں��ж�Ӧ�����Ե�ַ
 *=============================================*/
void * fnGet_VmkPage()
{
	unsigned int i, *addrpage;

	for(i = 0; i < uglbTotal_PmPages; i++)
	{
		if(0 == (glbPm_Bitmap[i / BITS_CHAR] & (1 << (i & (BITS_CHAR - 1)))))
		{
			//����bitmap
			glbPm_Bitmap[i / BITS_CHAR] |= (1 << (i & (BITS_CHAR - 1)));	 
			//��������ҳ���ַӳ����ں˵�ַ
			addrpage = (unsigned int *)(i * 1024 * 4 + COMPLIER_BASEADDR);		
			//���ں˿ռ��ʼ�������ڴ� zero!
			for(i =0; i < PAGE_SIZE/sizeof(unsigned int); i++)
				addrpage[i] = 0; 
			return (void *)(addrpage);
		}
	}
	return  0;
}


/*=============================================*
 *F: �ͷ�һ�������ڴ�ҳ��
 *I: ����ҳ�����ں���ӳ���ַ
 *O: void
 *C: ����ʧ�ܵ����û�����������Ĵ��� 
 *=============================================*/
void fnFree_VmkPage(void * addrpage)
{
	unsigned int  uPageId;

	uPageId = ((unsigned int)addrpage - COMPLIER_BASEADDR) >> 12;
	//�жϵ�ַ�Ƿ�Ϸ�
	if((uPageId < KERNEL_PAGES) || (uPageId >= uglbTotal_PmPages))
		return;
	//����bitmap
	glbPm_Bitmap[uPageId / BITS_CHAR] &= (1 << (uPageId & (BITS_CHAR - 1))) ^ 0xFF;
	fnWakeUp(&glbWaitMMPage);
}


/*----------------------------------------------------------------------* 
 *����:    Ϊ��ʹ�ں��ܹ��������������ڴ棬���ǲ�ȡ���洢Ͱ������ʵ������
 *     �ڴ�ķ�����ͷš�
 *	   mallock(unsigned int size);  
 *	   mfreek(void *lpblock);
 *----------------------------------------------------------------------*/

/*=======================================================================*
 *F: ��������ʼ������������������.
 *I: void
 *O: 0-ʧ�ܣ�!0-�ɹ�
 *C: ��lpfree_bucket_desc == NULLʱ��˵����ʱû�п��õġ�Ͱ��������������
 *   һͰ(һҳ)�ڴ棬����������ܷ����ڵ�һ�ε���mallock(..),��������ǰ��
 *   �����ù��ˣ�����һ���ڴ�ҳ�������조Ͱ������������������<Ͱ>��
 *=======================================================================*/
short  fnInit_Bucket_Desc()
{
	struct bucket_desc *lptemp, *lpfirst;
	unsigned int i;

	//������һ������ҳ��PAGE_SIZE
	lpfirst = lptemp = (struct bucket_desc *)fnGet_VmkPage();
	if(!lpfirst)
		return  0 ;
	//��ʼ����ҳ�棬����Ͱ����������
	for(i = PAGE_SIZE / sizeof(struct bucket_desc); i > 1; i--)
	{
		lptemp->lpnext = lptemp + 1; //������
		lptemp++;
	}
	lptemp->lpnext = lpfree_bucket_desc; //���һ��Ͱ��������lpnext
	lpfree_bucket_desc = lpfirst;
	return  1;
}


/*=====================================================*
 *F: ����<=4KB���ڴ档
 *I: �ڴ��С(��λ:�ֽ�)
 *O: 0-ʧ�ܣ�!0-�ɹ�����ΪС���ڴ����ں˵�ַ�ռ���׵�ַ��
 *=====================================================*/
void * fnVmk_alloc(unsigned int length)
{
	struct bucket_dir_entry *bdir;
	struct bucket_desc *bdesc;
	void *retval;
	unsigned int i, temp;

	//�����������洢ͰĿ¼��Ѱ�Һ��ʵ�Ͱ
	for(bdir = Bucket_Dir; bdir->size; bdir++)
	{
		if(bdir->size >= length)
			break; //�ҵ���
	}
	if(!bdir->size)
		return 0;
	//���ڿ�ʼ����á�Ͱ���������������е�̺ʽ������
	for(bdesc = bdir->chain; bdesc; bdesc = bdesc->lpnext)
	{
		if(bdesc->lpfree)
			break ; //�ҵ�һͰ�����ṩС���ڴ�!
	}	
	if(!bdesc)
	{
		//�������ϵ�����Ͱ�����ù���,���롰Ͱ���������͡�Ͱ��
		if(!lpfree_bucket_desc) //Ͱ������Ҳ����
		{
			if(!fnInit_Bucket_Desc())
				return 0;
		}
		//�ӿ���Ͱ����������ǰ��ȡ��һ��������
		bdesc = lpfree_bucket_desc;
		lpfree_bucket_desc = bdesc->lpnext;
		//��ʼ�����������
		bdesc->refcount = 0;
		bdesc->bucket_size = bdir->size;
		bdesc->lppage = bdesc->lpfree = (void *)fnGet_VmkPage();
		temp = (unsigned int)(bdesc->lppage);
		if(!bdesc->lppage)
			return 0;

		//�Ը�Ͱ���зָ�롰�������������ĳ�ʼ�����ơ�
		for(i = PAGE_SIZE / bdir->size; i >1; i--)
		{
			*((unsigned int *)temp) = temp + bdir->size;
			temp += bdir->size;
		}
		*((unsigned int *)temp) = 0;  //���һ��ָ��NULL
		//���潨���ˡ����������롰Ͱ����ӳ�䣬��������������������ͰĿ¼�������ͷ����
		bdesc->lpnext = bdir->chain;
		bdir->chain = bdesc;		
	}
	//����������,��ʼ�����ڴ�object
	retval = (void *)(bdesc->lpfree);
	bdesc->lpfree = *(void * *)retval;  //�ڴ�С���ǰ4bytes point to next free block.
	bdesc->refcount++;
	return retval;
}


/*=====================================================*
 *F: �ͷ�<=4KB���ڴ档
 *I: �ڴ�С���addr, �ڴ��С(��λ:�ֽ�)
 *O: void 
 *C: ��mallock(..)��Ӧ������ܹ��ڲ������ṩ���length,
 *   ���ͷŵĸ��졣���������set length = 0 ,���ǽ���̺ʽ
 *   ����; ����ʧ�ܵ����û�����������Ĵ��� 
 *=====================================================*/
void fnVmk_free(void *lpblock, unsigned int length)
{
	void *page;
	struct bucket_dir_entry *bdir;
	struct bucket_desc *bdesc, *prev;

	//�����object���ڵ�ҳ��
	page = (void *)((unsigned int)lpblock & 0xfffff000);
	//ѭ��������ͰĿ¼�,Ѱ��page
	for(bdir = Bucket_Dir; bdir->size; bdir ++)
	{
		prev = 0;
		//if length==0 ,then ����ִ��continue
		if(bdir->size < length)
			continue;	
		//��ʱ,length ==0,��ô����̺ʽ����page
		for(bdesc = bdir->chain; bdesc; bdesc = bdesc->lpnext)
		{
			if(bdesc->lppage == page)  //һ����Ͱ����������Ӧһ����ҳ�桱
				goto _found;
			prev = bdesc;
		}
	}
	//û���ҵ��ͷ���
	return;
_found:
	//�����С����뵽�����ж���ͷ����
	*(void **)lpblock = bdesc->lpfree;
	bdesc->lpfree = lpblock;
	bdesc->refcount--;  //���ô�����һ
	//������ô���==0����ô�����ͷŸá�Ͱ���������Ͷ�Ӧ�ġ�Ͱ��
	if(bdesc->refcount == 0)
	{
		if(prev)
			prev->lpnext = bdesc->lpnext;
		else
			bdir->chain = bdesc->lpnext;
		//ɾ����Ͱ��,�������������뵽���С�Ͱ��������ͷ��
		fnFree_VmkPage((void *)bdesc->lppage);
		bdesc->lpnext = lpfree_bucket_desc;
		lpfree_bucket_desc = bdesc;
	}
	return;
}


/*=====================================================*
 *F: ��һ���ڴ���Ϣ���Ƶ� ��һ���顣
 *I: char * dest, char * sour, unsigned int size(bytes)
 *O: void
 *=====================================================*/
void fnVmkCpy(char *dest, char *sour, unsigned int size)
{
	unsigned int i;
	for(i=0; i< size; i++)
		*dest++ = *sour++; 
}

/*=====================================================*
 *F: ��ʼ�������ڴ������Ϣ��
 *C: ��StratMain()�е���
 *=====================================================*/
void fnInit_Mem()
{
	unsigned int i;
	char szbuf[128];

	for(i = 0; i < 8 * 1024; i++)
		 glbPm_Bitmap[i] = 0;
	//��־�ں�ռ���� 0 -- (KERNEL_PAGES - 1)
	for(i = 0; i < KERNEL_PAGES; i++)
	{
		glbPm_Bitmap[i / BITS_CHAR] |= (1 << (i & (BITS_CHAR-1)));
	}
	//��־��Ƶ��BIOS,ҳ��,FileBufferռ����(��ȥ��һЩ),��Χ0x0A0000 ~ 0x400000
	for(i = (0x0A0000 >>12); i < (0x400000 >> 12); i++)
	{
		glbPm_Bitmap[i / BITS_CHAR] |= (1 << (i & (BITS_CHAR - 1)));
	}
	//��ʾ�����ڴ��С
	vsprintf(szbuf, "size of RAM is: 0x%x * 4kb\n", uglbTotal_PmPages);
	printk(WHITE, szbuf);
}