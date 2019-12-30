#include <stdio.h>
#include <stdlib.h>

#include "Platform.h"
#include "Bootloader.h"

extern void
PUT32(u32, u32);

extern u32
GET32(u32);

#define MMUTABLEBASE 0x00004000

extern void start_mmu ( unsigned int, unsigned int );
extern void stop_mmu ( void );
extern void invalidate_tlbs ( void );
extern void invalidate_caches ( void );
extern void enable_irq(void);

u64
GetSystemTimer(void)
{
    u32 low;
    u32 high;

    dmb();

timerRead:
    low  = *SYSTIMER_CLO;
    high = *SYSTIMER_CHI;

    if (high != *SYSTIMER_CHI)
    {
        goto timerRead;
    }

    u64 result = (high << 31) | low;

    return result;
}

u32
GetSystemTimer32(void)
{
    dmb();

    return *SYSTIMER_CLO;
}

void
DelayS(u32 sec)
{
    u64 target = (sec * SYSTIMER_FREQ) + GetSystemTimer();

    while (target > GetSystemTimer())
    {}
}

void 
SetupUART(void)
{
    dmb();

    *AUX_ENABLES = _AUX_ENABLES_UART_MASK;
    *AUX_MU_IER  = 0;
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR  = 3;
    *AUX_MU_MCR  = 0;
    *AUX_MU_IER  = 0x5;
    *AUX_MU_IIR  = 0xC6;
    *AUX_MU_BAUD = BAUD_DIV_115200;
    
    dmb();

    SetGPIOMode(GPIO_14, GPIO_ALT5);
    SetGPIOMode(GPIO_15, GPIO_ALT5);
}

void
StartUART(void)
{   
    dmb();
    
    *AUX_MU_CNTL = 3;
    
    while (UART_HasInput())
    {
        UART_GetC();
    }
}

unsigned int 
mmu_section(unsigned int vadd, unsigned int padd, unsigned int flags)
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    ra = vadd >> 20;
    rb = MMUTABLEBASE | (ra << 2);
    rc = (padd & 0xFFF00000) | 0xC00 | flags | 2;
    
    PUT32(rb,rc);
    
    return(0);
}

unsigned int 
mmu_small(unsigned int vadd, unsigned int padd, unsigned int flags, unsigned int mmubase)
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    ra = vadd >> 20;
    rb = MMUTABLEBASE | (ra << 2);
    rc = (mmubase & 0xFFFFFC00)/*|(domain<<5)*/ | 1;
    
    PUT32(rb, rc); //first level descriptor
    
    ra = (vadd >> 12) & 0xFF;
    rb = (mmubase & 0xFFFFFC00) | (ra << 2);
    rc = (padd & 0xFFFFF000) | (0xFF0) | flags | 2;
    
    PUT32(rb, rc); //second level descriptor
    
    return(0);
}

void
c_irq_handler(void)
{
    while(1) 
    {
        if ((*AUX_MU_IIR & _AUX_MU_IO_REG_INT_PENDING_MASK))
        {
            break;
        }

        if (*AUX_MU_IIR & _AUX_MU_IO_REG_RX_MASK)
        {
            // We've received a byte of data
            
            // Read the byte to remove it from the fifo
            char c;
            UART_GetC_FAST(c);
            
            UART_Printf("Got byte 0x%x\n", c);
            
            switch (rxState)
            {
                case (RX_STATE_IDLE):
                {
                    rxStateData.command = 0;
                    rxStateData.commandByteCount = 1;
                    
                    rxState = RX_STATE_COMMAND;
                } break;
                
                case (RX_STATE_COMMAND):
                {
                    rxStateData.command = rxStateData.command | (u32) ((u32) c << rxStateData.commandByteCount++);
                    UART_Printf("Got command 0x%04x\n", rxStateData.command);
                    
                    if (rxStateData.commandByteCount >= 4)
                    {
                        if (rxStateData.command == BOOTLOADER_COMMAND_PRINT_INFO)
                        {
                            UART_Puts("Hello! This is PiBoot, a basic bitch bootloader for my team's embedded junior project written by Garrison Peacock!");
                            
                            rxState = RX_STATE_IDLE;
                        }
                        else if (rxStateData.command == BOOTLOADER_COMMAND_ECHO)
                        {
                            rxState = RX_STATE_ECHO;
                        }
                        else if (rxStateData.command == BOOTLOADER_COMMAND_ECHO_SIZE)
                        {
                            rxStateData.sizeToEcho = 0;
                            rxStateData.sizeToEchoByteCount = 0;
                            
                            rxState = RX_STATE_ECHO_SIZE_GET_SIZE;
                        }
                        else
                        {
                            UART_Printf("Unknown command 0x%x\n", c);
                            
                            rxState = RX_STATE_IDLE;
                        }
                    }
                } break;
                
                case (RX_STATE_ECHO_SIZE_GET_SIZE):
                {
                    rxStateData.sizeToEcho |= (u32) (c << rxStateData.sizeToEchoByteCount++);
                    
                    if (rxStateData.sizeToEchoByteCount >= 4)
                    {
                        rxState = RX_STATE_ECHO;
                        UART_Printf("Will echo %u chars\n", rxStateData.sizeToEcho);
                    }
                } break;
                
                case (RX_STATE_ECHO_SIZE):
                {
                    UART_PutC_FAST(c);
                    
                    rxStateData.sizeToEcho -= 1;
                    if (!rxStateData.sizeToEcho)
                    {
                        rxState = RX_STATE_IDLE;
                    }
                } break;
                
                case (RX_STATE_ECHO):
                {
                    UART_PutC_FAST(c);
                    
                    rxState = RX_STATE_IDLE;
                } break;
            }
        }
    }
}

int 
start()
{
    // Disable the mini-uart IRQ during startup
    *IRQ_DISABLE_IRQ_1 = _IRQ_DISABLE_IRQ_1_AUX_MASK;
    
    SetGPIOMode(GPIO_LED, GPIO_OUTPUT);
    ClearGPIO(GPIO_LED);
    
    SetupUART();
    StartUART();

    u32 ra;
    for (ra=0;; ra += 0x00100000)
    {
        mmu_section(ra, ra, 0x0000);
        if (ra == 0xFFF00000) 
        {
            break;
        }
    }

    mmu_section(0x20000000, 0x20000000, 0x0000); // NOT CACHED!
    mmu_section(0x20200000, 0x20200000, 0x0000); // NOT CACHED!

    start_mmu(MMUTABLEBASE, 0x00000001 | 0x1000 | 0x0004); // [23]=0 subpages enabled = legacy ARMv4,v5 and v6
    
    rxState = RX_STATE_IDLE;
    
    rxStateData.command = 0;
    
    *IRQ_ENABLE_IRQ_1 = _IRQ_ENABLE_IRQ_1_AUX_MASK;
    
    UART_Puts("PiBoot ready and waiting...");
    
    enable_irq();

    char inputBuffer[512];
    while (1)
    {
        UART_Printf("Pi Ping\n");
        DelayS(10);
        
        UART_Printf("Pi Pong\n");
        DelayS(10);
    }

    return EXIT_SUCCESS;
}
