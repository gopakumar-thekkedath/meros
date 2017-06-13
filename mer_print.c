
#include "mer_krnl.h"
/*
 *	Function : mprintk
 *	Arguments: 1) The string/decimal to print
 *		   2) Size
 *		   3) Type (string or decimal)
*/

void mprintk(unsigned char *string, int size, char type)
{
	volatile unsigned short cursor = *CUR_PTR;
	int i=0;
	/* find if we require a new line */
	if (string[0] == '*') {
		/* yes, it is a new line */
		unsigned short old_cursor = cursor;
		unsigned short offset;
		while (cursor > 160)
			cursor -=160;
		offset = 160 - cursor;
		cursor = old_cursor + offset;
		/* set the new cursor value */
		*CUR_PTR = cursor;
		string++; /* first character is new line, forget it */
	}
	*((short*)CUR_PTR)+= size*2;
	/* the display stuff is a string or a decimal ?? */
	if (type == 'c') {
	/* it is a string */	
		while(size--) {
			*(DISP_PTR + cursor +i*2) = string[i];
			*(DISP_PTR + cursor +i*2 + 1) = 0x1e;
			i++;
		}
	} else {
	/* it is a decimal */	
		unsigned int num = 0;
		unsigned int temp_num;
		unsigned char ch;
		int len=0;
		if (size == 1) {
			 num = *string;
		} else if (size == 2) {
			num = *((unsigned short*)string); 	
		} else if (size == 4) {
			num = *((unsigned int*)string);
		}
		temp_num = num;
		/* find number of bytes of video memory required */
		while (temp_num) {
			temp_num/=10;
			len ++;
		}	
		len --;
		/* display it */
	//	while (num)
		do {
			ch = (num % 10) + 48;
			*(DISP_PTR + cursor + len*2) = ch;
			*(DISP_PTR + cursor + len*2 + 1) = 0x1e;
			num/= 10;
			len -=1;
		} while(num);

	}
}

