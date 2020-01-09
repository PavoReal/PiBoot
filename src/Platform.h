#if !defined(JP_PLATFORM_H)
#define JP_PLATFORM_H

#define JP_VERSION "0.0.0"

#define UNUSED(a) (void) a

#define KILOBYTES(a) ((a) * 1024)

#include "BCM2835.h"
#include "GPIO.h"
#include "UART.h"

#endif