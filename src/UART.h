#if !defined(JP_UART_H)
#define JP_UART_H

#include "BCM2835.h"

#define BAUD_DIV_9600 (3254)
#define BAUD_DIV_115200 (270)

#define UART_PutNewline() UART_Put('\n')
#define UART_PutEchoStop() UART_Put(0);
#define UART_PutNewlineStop() UART_PutNewline(); UART_PutEchoStop()

#define UART_Put_FAST(c) *AUX_MU_IO = (c)
#define UART_Get_FAST(c) c = *AUX_MU_IO

extern void
UART_Put(u8 c);

extern void
UART_Puts(char *str);

extern void
UART_PutB(u8 *str, u32 size);

extern void
UART_Printf(const char *fmt, ...);

extern u8
UART_Get(void);

extern u32
UART_Gets(char *str);

extern void
UART_Flush(void);

extern u8
UART_HasInput(void);

#endif
