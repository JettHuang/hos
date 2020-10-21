/*---------------------------------------------------------*
 *˵����ʵ�ָ�ʽ������vsprintf(...)
 *�޸�ʱ�䣺 04/11/07  15:20
 *
 *           2007-06-17  9:27
 *---------------------------------------------------------*/
void InttoHex(char *szbuf, unsigned int uvalue);
unsigned int strcpy(char *dest, char *sour);


/*=====================================================*
 *F: ��ʽ���ַ���
 *I: buf-������ָ��, fmt-��ʽ���ַ���, ��ֵ1, ...
 *O: void
 *C: û�жԻ������������м�⣬ϣ��ʹ��ʱע��buffer��С��
 *=====================================================*/
void vsprintf(char *buf, char *fmt, ...)
{
	char *p;
	char szBuffer[32];
	void *valuelist;
	
	//��õ�һ��value�ڶ�ջ�ϵ�ָ��
	valuelist = (void *)((unsigned int)(&fmt) + 4);
	for(p = buf; *fmt; fmt++)
	{
		if(*fmt != '%')
		{
			*p++ = *fmt;
			continue;
		}
		fmt++;
		switch(*fmt)
		{
		case 'x':
			//����
		case 'X':
			InttoHex(szBuffer, *((unsigned int *)valuelist));
			p += strcpy(p, szBuffer);
			break;
		case 's':
			//����
		case 'S':
			p += strcpy(p, *((char* *)valuelist));
			break;
		case 'd':

		default:
			break;
		}
		//�����һ��value�ڶ�ջ�ϵ�ָ��
		valuelist = (void *) ((unsigned int)(valuelist) + 4);
	}
	*p = '\0';
}


/*=====================================================*
 *F: int ת���� hex
 *I: szbuf, uvalue
 *O: void
 *=====================================================*/
void InttoHex(char *szbuf, unsigned int uvalue)
{
	int i;
	char ch;

	for(i = 0; i < 8; i++)
	{
		ch = (char)((uvalue >> (i << 2)) & 0x0F);
		if(ch < 0x0A)
			szbuf[7-i] = (char)('0' + ch);
		else
			szbuf[7-i] = (char)('A' + (ch - 0x0A));
	}
	szbuf[8] = '\0';
}


/*=====================================================*
 *F: ��sourָ����ַ������Ƶ�dest�� 
 *I: dest, sour
 *O: ���Ƶ��ַ�����bytes
 *=====================================================*/
unsigned int strcpy(char *dest, char *sour)
{
	unsigned int count;
	
	count = 0;
	while(*sour)
	{
		*dest++ = *sour++;
		count++;
	}
	*dest = '\0';
	return count;
}
