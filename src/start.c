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
        UART_Get();
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
             u8 input;
            UART_Get_FAST(input);
            
            #if 0
            UART_Printf("CS: %u, RX: %u\n", rxState, input);
            #endif
            
            switch (rxState)
            {
                case (RX_STATE_IDLE):
                {
                    rxStateData.command = input;
                    
                    switch (rxStateData.command)
                    {
                        case BOOTLOADER_COMMAND_PRINT_INFO:
                        {
                            UART_Puts("Hello! This is PiBoot, a basic bitch bootloader for my team's embedded junior project written by Garrison Peacock!");
                            
                            rxState = RX_STATE_IDLE;
                        } break;
                        
                        case BOOTLOADER_COMMAND_ECHO:
                        {
                            rxState = RX_STATE_ECHO;
                        } break;
                        
                        case BOOTLOADER_COMMAND_UPLOAD:
                        {
                            rxStateData.uploadSize      = 0;
                            rxStateData.uploadSizeIndex = 0;
                            
                            rxState = RX_STATE_UPLOAD_GET_SIZE;
                        } break;
                        
                        case BOOTLOADER_COMMAND_UNKNOWN:
                        default:
                        {
                            UART_Printf("Unknown command 0x%x", input);
                            
                            rxState = RX_STATE_IDLE;
                        } break;
                    }
                } break;
                
                case (RX_STATE_ECHO):
                {
                        UART_Put_FAST(input);
                    
                    if (input == '\0')
                    {
                        rxState = RX_STATE_IDLE;
                    }
                } break;
                
                case (RX_STATE_UPLOAD_GET_SIZE):
                {
                    rxStateData.uploadSize |= ((u32) input << (rxStateData.uploadSizeIndex * 8));
                    rxStateData.uploadSizeIndex += 1;
                    
                    if (rxStateData.uploadSizeIndex >= 4)
                    {
                        UART_Printf("Kernel size: %u", rxStateData.uploadSize);
                        
                        rxStateData.uploadIndex    = BOOTLOADER_MEMORY_TARGET;
                        rxStateData.uploadRXSize   = 0;
                        
                        rxState = RX_STATE_UPLOAD_DATA;
                    }
                } break;
                
                case (RX_STATE_UPLOAD_DATA):
                {
                    *rxStateData.uploadIndex++ = input;
                    rxStateData.uploadRXSize += 1;
                    
                    if (rxStateData.uploadRXSize >= rxStateData.uploadSize)
                    {
                        UART_Printf("Upload done...");
                        
                        rxState = RX_STATE_IDLE;
                    }
                    
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
    
    UART_Puts("MMU configured...");
    
    rxState = RX_STATE_IDLE;
    rxStateData.command = 0;
    
    UART_Puts("Bootloader state configured...");
    
    *IRQ_ENABLE_IRQ_1 = _IRQ_ENABLE_IRQ_1_AUX_MASK;
    
    UART_Puts("PiBoot ready and waiting...");
    
    enable_irq();
    
    while (1)
    {
        UART_Printf("Pi Ping: %x", *BOOTLOADER_MEMORY_TARGET);
        DelayS(2);
        
        UART_Printf("Pi Pong: %x", *(BOOTLOADER_MEMORY_TARGET + 1024));
        DelayS(2);
    }

    return EXIT_SUCCESS;
}
