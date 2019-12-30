#if !defined(PI_BOOTLOADER_H)
#define PI_BOOTLOADER_H

typedef u32 BootloaderCommand;
enum _BootloaderCommand
{
    BOOTLOADER_COMMAND_PRINT_INFO,
    BOOTLOADER_COMMAND_ECHO,
    
    BOOTLOADER_COMMAND_ECHO_SIZE,
};

#endif

// The ARM side of things
#if defined(PI_BOOT)

typedef enum
{
    RX_STATE_IDLE,
    RX_STATE_COMMAND,
    
    RX_STATE_ECHO,
    
    RX_STATE_ECHO_SIZE_GET_SIZE,
    RX_STATE_ECHO_SIZE,
} RXState;

typedef struct
{
    union
    {
        u32 command;
        u32 sizeToEcho;
    };
    
    union
    {
        u8 commandByteCount;
        u8 sizeToEchoByteCount;
    };
    
} RXStateData;

GLOBAL RXState rxState;
GLOBAL RXStateData rxStateData;

#endif

// The PC side of things
#if defined(PI_TERM)



#endif
