#if !defined(PI_BOOTLOADER_H)
#define PI_BOOTLOADER_H

typedef u8 BootloaderCommand;
enum _BootloaderCommand
{
    BOOTLOADER_COMMAND_UNKNOWN = 0,
    
    BOOTLOADER_COMMAND_PRINT_INFO, // Print some happy message
    BOOTLOADER_COMMAND_ECHO,       // Echo until a zero byte is sent
    
    BOOTLOADER_COMMAND_AWK, // Okay to proceed 
    BOOTLOADER_COMMAND_ERR,
    
    BOOTLOADER_COMMAND_UPLOAD // Send 4 bytes for the size of the kernel then the data
};

#define BOOTLOADER_CHUNK_COUNT_SIZE (4)
#define BOOTLOADER_CHUNK_SIZE (KILOBYTES(10))
#define BOOTLOADER_RETRY_MAX (10)

// The ARM side of things
#if defined(PI_BOOT)

#include "libsha1.h"

typedef enum
{
    RX_STATE_IDLE,
    
    RX_STATE_ECHO,
    
    RX_STATE_UPLOAD_GET_SIZE,
    RX_STATE_UPLOAD_GET_WHOLE_CHECKSUM,
    RX_STATE_UPLOAD_GET_CHUNK_CHECKSUM,
    RX_STATE_UPLOAD_GET_CHUNK_DATA
} RXState;

typedef struct
{
    u8 command;
    
    u8 helperIndex;
    
    u8 *lastFileWorkingIndex;
    u8 *fileWorkingIndex;
    u8 *fileStopIndex;
    
    u32 totalFileSize;
    u32 rxFileSize;
    u32 chunkCount;
    
    u8 totalChecksum[SHA1_DIGEST_SIZE];
    u8 workingChecksum[SHA1_DIGEST_SIZE];
} RXStateData;

GLOBAL RXState rxState;
GLOBAL RXStateData rxStateData;

GLOBAL u8 * const BOOTLOADER_MEMORY_TARGET = (u8*) 0x06400000;

#endif

// The PC side of things
#if defined(PI_TERM)

#endif
#endif
