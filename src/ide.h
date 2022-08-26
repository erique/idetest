#pragma once

#include <stdint.h>

// COMMAND
#define ATA_CMD_RECALIBRATE             (0x10)
#define ATA_CMD_READ_SECTORS            (0x20)
#define ATA_CMD_READ_SECTORS_EXT        (0x24)
#define ATA_CMD_WRITE_SECTORS           (0x30)
#define ATA_CMD_WRITE_SECTORS_EXT       (0x34)
#define ATA_CMD_READ_VERIFY_SECTORS     (0x40)
#define ATA_CMD_READ_VERIFY_SECTORS_EXT (0x42)
#define ATA_CMD_FORMAT                  (0x50)
#define ATA_CMD_SEEK                    (0x70)
#define ATA_CMD_DIAGNOSTICS             (0x90)
#define ATA_CMD_INIT_PARAMETERS         (0x91)
#define ATA_CMD_READ_MULTIPLE           (0xc4)
#define ATA_CMD_READ_MULTIPLE_EXT       (0x29)
#define ATA_CMD_WRITE_MULTIPLE          (0xc5)
#define ATA_CMD_WRITE_MULTIPLE_EXT      (0x39)
#define ATA_CMD_SET_MULTIPLE_MODE       (0xc6)
#define ATA_CMD_FLUSH_CACHE             (0xe7)
#define ATA_CMD_IDENTIFY_DEVICE         (0xec)
#define ATA_CMD_SET_FEATURES            (0xef)

// STATUS
#define ATA_SRB_ERR                     (0)     // Error
#define ATA_SRB_IDX                     (1)     // Index
#define ATA_SRB_CORR                    (2)     // Corrected data
#define ATA_SRB_DRQ                     (3)     // Data request ready
#define ATA_SRB_DSC                     (4)     // Drive seek complete
#define ATA_SRB_DF                      (5)     // Drive fault
#define ATA_SRB_DRDY                    (6)     // Drive ready
#define ATA_SRB_BSY                     (7)     // Busy

#define ATA_SRF_ERR                     (1 << ATA_SRB_ERR)
#define ATA_SRF_IDX                     (1 << ATA_SRB_IDX)
#define ATA_SRF_CORR                    (1 << ATA_SRB_CORR)
#define ATA_SRF_DRQ                     (1 << ATA_SRB_DRQ)
#define ATA_SRF_DSC                     (1 << ATA_SRB_DSC)
#define ATA_SRF_DF                      (1 << ATA_SRB_DF)
#define ATA_SRF_DRDY                    (1 << ATA_SRB_DRDY)
#define ATA_SRF_BSY                     (1 << ATA_SRB_BSY)

// ERROR
#define ATA_ERB_AMNF                    (0)     // No address mark
#define ATA_ERB_TK0NF                   (1)     // Track 0 not found
#define ATA_ERB_ABRT                    (2)     // Command aborted
#define ATA_ERB_MCR                     (3)     // Media change request
#define ATA_ERB_IDNF                    (4)     // ID mark not found
#define ATA_ERB_MC                      (5)     // Media changed
#define ATA_ERB_UNC                     (6)     // Uncorrectable data
#define ATA_ERB_BBK                     (7)     // Bad block

#define ATA_ERF_AMNF                    (1 << ATA_ERB_AMNF)
#define ATA_ERF_TK0NF                   (1 << ATA_ERB_TK0NF)
#define ATA_ERF_ABRT                    (1 << ATA_ERB_ABRT)
#define ATA_ERF_MCR                     (1 << ATA_ERB_MCR)
#define ATA_ERF_IDNF                    (1 << ATA_ERB_IDNF)
#define ATA_ERF_MC                      (1 << ATA_ERB_MC)
#define ATA_ERF_UNC                     (1 << ATA_ERB_UNC)
#define ATA_ERF_BBK                     (1 << ATA_ERB_BBK)

struct ATA_IDE
{
    volatile uint16_t   data;
    volatile uint8_t    pad0[2];

    union
    {
        struct
        {
            volatile uint8_t    error;
        };
        struct
        {
            volatile uint8_t    features;
        };
    };

    volatile uint8_t    pad1[3];
    volatile uint8_t    sectorCount;
    volatile uint8_t    pad2[3];
    volatile uint8_t    sectorNum;          // LBA28[0] / LBA48[3]
    volatile uint8_t    pad3[3];
    volatile uint8_t    cylLow;             // LBA28[1] / LBA48[4]
    volatile uint8_t    pad4[3];
    volatile uint8_t    cylHigh;            // LBA28[2] / LBA48[5]
    volatile uint8_t    pad5[3];
    volatile uint8_t    driveHead;          // LBA28[3] / DEV / LBA
    volatile uint8_t    pad6[3];
    union
    {
        struct
        {
            volatile uint8_t    status;
        };
        struct
        {
            volatile uint8_t    command;
        };
    };
    volatile uint8_t    pad7[0x1000 - 0x1d];
    volatile uint8_t    intStatus;
    volatile uint8_t    padb[0x18 - 1];
    union
    {
        struct
        {
            volatile uint8_t    altStatus;
        };
        struct
        {
            volatile uint8_t    deviceCtrl;
        };
    };
    volatile uint8_t    pad9[3];
    volatile uint8_t    driveAddr;
};
