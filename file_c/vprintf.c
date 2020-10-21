/*---------------------------------------------------------*
 *说明；实现格式化函数vsprintf(...)
 *修改时间： 04/11/07  15:20
 *
 *           2007-06-17  9:27
 *---------------------------------------------------------*/
void InttoHex(char *szbuf, unsigned int uvalue);
unsigned int strcpy(char *dest, char *sour);


/*=====================================================*
 *F: 格式化字符串
 *I: buf-缓冲区指针, fmt-格式化字符串, 数值1, ...
 *O: void
 *C: 没有对缓冲区容量进行检测，希望使用时注意buffer大小。
 *=====================================================*/
void vsprintf(char *buf, char *fmt, ...)
{
	char *p;
	char szBuffer[32];
	void *valuelist;
	
	//获得第一个value在堆栈上的指针
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
			//继续
		case 'X':
			InttoHex(szBuffer, *((unsigned int *)valuelist));
			p += strcpy(p, szBuffer);
			break;
		case 's':
			//继续
		case 'S':
			p += strcpy(p, *((char* *)valuelist));
			break;
		case 'd':

		default:
			break;
		}
		//获得下一个value在堆栈上的指针
		valuelist = (void *) ((unsigned int)(valuelist) + 4);
	}
	*p = '\0';
}


/*=====================================================*
 *F: int 转换成 hex
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
 *F: 将sour指向的字符串复制到dest处 
 *I: dest, sour
 *O: 复制的字符个数bytes
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
