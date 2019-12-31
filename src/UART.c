#include "UART.h"

#include <stdarg.h>
#include <stdio.h>

#include "Bootloader.h"

void
UART_Put(u8 c)
{
	dmb();
	
	while (1)
    {
    	if (*AUX_MU_LSR & _AUX_MU_LSR_TRANS_EMPTY_MASK)
    	{
    		break;
    	}
    }

    *AUX_MU_IO = c;
}

void
UART_Puts(char *str)
{
	while (*str)
	{
		UART_Put(*str++);
	}

	UART_PutNewlineStop();
}

void
UART_PutB(u8 *str, u32 size)
{
	while (size--)
	{
		UART_Put(*str++);
	}
}

void
UART_Printf(const char *fmt, ...)
{
	char buffer[4096];

	va_list args;
	va_start(args, fmt);

	vsprintf(buffer, fmt, args);

	UART_Puts(buffer);

	va_end(args);
}

u8
UART_Get(void)
{
    dmb();
    
	while (!(*AUX_MU_LSR & _AUX_ME_LSR_DATA_READY_MASK))
	{
		nop();
	}
    
    u8 result = *AUX_MU_IO;
    
	return result;
}

u32
UART_Gets(char *str)
{
	u32 count = 1;
	char c;

	bool done = false;

	while (!done)
	{
		c = (char) UART_Get();

		if (c != '\0')
		{
			*str++ = c;
			++count;
		}
		else
		{
			done = true;
		}
	}

	*str = '\0';

	return count;
}

void
UART_Flush(void)
{
    dmb();
    
	while(!(*AUX_MU_LSR & _AUX_ME_LSR_TRANS_IDLE_MASK))
	{
		nop();
	}
}

u8
UART_HasInput(void)
{
	u8 result = 0;
    
    dmb();
    
	result = (*AUX_MU_LSR & _AUX_ME_LSR_DATA_READY_MASK);

	return result;
}
