#if !defined(JP_UART_H)
#define JP_UART_H

#include "BCM2835.h"

#define BAUD_DIV_9600 (3254)
#define BAUD_DIV_115200 (270)

#define UART_PutNewline() UART_PutC('\r'); UART_PutC('\n')

#define UART_PutC_FAST(c) dmb(); *AUX_MU_IO = (c)
extern void
UART_PutC(char c);

extern void
UART_PutError(u32 data);

extern void
UART_Puts(char *str);

extern void
UART_PutStr(char *str);

extern void
UART_PutB(char *str, u32 size);

extern void
UART_Printf(const char *fmt, ...);

extern char
UART_GetC(void);

extern u32
UART_GetS(char *str);

extern void
UART_Flush(void);

extern u8
UART_HasInput(void);

#endif