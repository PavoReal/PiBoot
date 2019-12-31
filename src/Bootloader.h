#if !defined(PI_BOOTLOADER_H)
#define PI_BOOTLOADER_H

typedef u8 BootloaderCommand;
enum _BootloaderCommand
{
    BOOTLOADER_COMMAND_UNKNOWN = 0,
    
    BOOTLOADER_COMMAND_PRINT_INFO, // Print some happy message
    BOOTLOADER_COMMAND_ECHO,       // Echo until a zero byte is sent
    
    BOOTLOADER_COMMAND_UPLOAD // Send 4 bytes for the size of the kernel then the data
};

// The ARM side of things
#if defined(PI_BOOT)

typedef enum
{
    RX_STATE_IDLE,
    
    RX_STATE_ECHO,
    
    RX_STATE_UPLOAD_GET_SIZE,
    RX_STATE_UPLOAD_DATA,
} RXState;

typedef struct
{
    u8 command;
    
    u32 uploadSize;
    u8 uploadSizeIndex;
    
    u8 *uploadIndex;
    u32 uploadRXSize;
    
} RXStateData;

GLOBAL RXState rxState;
GLOBAL RXStateData rxStateData;

GLOBAL u8 * const BOOTLOADER_MEMORY_TARGET = (u8*) 0x06400000;

#endif

// The PC side of things
#if defined(PI_TERM)

#endif
#endif
