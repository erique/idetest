#include <alloca.h>
#include <stdbool.h>

#include <proto/exec.h>
#include <devices/timer.h>

#include <inline/alib.h>

#include "ide.h"

#define TITLE		"IDE Test"
#define AUTHOR		"Terrible Fire"

#include "version.h"
#include "logging.h"

const char IDString[] = VSTRING;
const char VTAG[] = VERSTAG;

typedef enum DeviceStatus
{
    DeviceStatus_NoError,               // ready to accept commands
    DeviceStatus_DeviceNotPresent,      // device not ready nor busy
    DeviceStatus_DeviceBusy,            // device is busy
    DeviceStatus_DeviceError,           // device reports error
    DeviceStatus_DeviceFault,           // device is in fault state
    DeviceStatus_DeviceNotReady,        // device not ready
} DeviceStatus;

static DeviceStatus PollStatus(struct ATA_IDE* ide);
static void PrintStatus(struct ATA_IDE* ide, DeviceStatus status);

static bool DetectDrive(struct ATA_IDE* ide, uint8_t drive);
static void IdentifyDevice(struct ATA_IDE* ide);

static void SleepTimer(uint32_t secs, uint32_t micros);

__stdargs  // main() uses arguments passed via the stack
int main(int argc, const char* argv[])
{
    // print to serial (9600 8N1)
    kprintf("\n%s :\n", argv[0]);

    KLOG(LOG_INFO, "idetest", "%s\n", IDString);

    struct ATA_IDE* ide = (struct ATA_IDE*)0x00da4000;

    if (!DetectDrive(ide, 0))
        return -1;

    kprintf("|:._\n");

    IdentifyDevice(ide);

    return 0;
}

#define rw(x) ((buffer[(x)*2] << 8) | (buffer[(x)*2+1]))
#define rl(x) ((rw(x)) | (rw((x)+1) << 16))

static void IdentifyDevice(struct ATA_IDE* ide)
{
    const uint8_t cmd = ATA_CMD_IDENTIFY_DEVICE;

    KINFO("IDE", "IdentifyDevice(); wait device ready...\n");

    DeviceStatus status;

    while ((status = PollStatus(ide)) == DeviceStatus_DeviceBusy)
        KDEBUG("IDE", "Device busy...\n");

    PrintStatus(ide, status);

    KINFO("IDE", "Sending Identify Command 0x%02lX\n", cmd);
    ide->command = ATA_CMD_IDENTIFY_DEVICE;

    while ((status = PollStatus(ide)) == DeviceStatus_DeviceBusy)
        KDEBUG("IDE", "Device busy...\n");

    const uint16_t buffer_size = 512;
    uint8_t* buffer = alloca(buffer_size);

    for (short i = 0; i < buffer_size / 2; i++)
    {
        uint16_t v = ide->data;
        buffer[i * 2 + 0] = v;
        buffer[i * 2 + 1] = v >> 8;
    }

// See ATA/ATAPI document, IDENTIFY DEVICE information
// (offsets in words)
#define ATA_IDENT_GENERAL_CONFIG    (0)
#define ATA_IDENT_CYLINDERS         (1)
#define ATA_IDENT_HEADS             (3)
#define ATA_IDENT_SECTORS           (6)
#define ATA_IDENT_SERIAL            (10)
#define ATA_IDENT_FIRMWARE          (23)
#define ATA_IDENT_MODEL             (27)
#define ATA_IDENT_CAPABILITIES      (49)
#define ATA_IDENT_MAX_LBA           (60)
#define ATA_IDENT_COMMAND_SET       (83)

    KINFO("IDE", "General configuration bits    : $%04lx\n", rw(ATA_IDENT_GENERAL_CONFIG));
    KINFO("IDE", "Drive is removable?           : %s\n", rw(ATA_IDENT_GENERAL_CONFIG) & (1 <<  7) ? "yes" : "no");
    KINFO("IDE", "Drive is fixed?               : %s\n", rw(ATA_IDENT_GENERAL_CONFIG) & (1 <<  6) ? "yes" : "no");
    KINFO("IDE", "Number of logical cylinders   : $%lx\n", rw(ATA_IDENT_CYLINDERS));
    KINFO("IDE", "Number of logical heads       : $%lx\n", rw(ATA_IDENT_HEADS));
    KINFO("IDE", "Number of sectors per track   : $%lx\n", rw(ATA_IDENT_SECTORS));

    buffer[19 * 2 + 1] = '\0';
    KINFO("IDE", "Serial number                 : %s\n", &buffer[ATA_IDENT_SERIAL * 2]);
    buffer[26 * 2 + 1] = '\0';
    KINFO("IDE", "Firmware revision             : %s\n", &buffer[ATA_IDENT_FIRMWARE * 2]);
    buffer[46 * 2 + 1] = '\0';
    KINFO("IDE", "Model number                  : %s\n", &buffer[ATA_IDENT_MODEL * 2]);

    KINFO("IDE", "Total number of sectors (LBA) : %lx\n", rl(ATA_IDENT_MAX_LBA));

    KINFO("IDE", "Capabilities bits             : $%04lx\n", rw(ATA_IDENT_CAPABILITIES));

    KINFO("IDE", "LBA is supported?             : %s\n", rw(ATA_IDENT_CAPABILITIES) & (1 <<  9) ? "yes" : "no");
    KINFO("IDE", "DMA is supported?             : %s\n", rw(ATA_IDENT_CAPABILITIES) & (1 <<  8) ? "yes" : "no");

    KINFO("IDE", "Command sets supported        : $%04lx\n", rw(ATA_IDENT_COMMAND_SET));

    KINFO("IDE", "LBA48 is supported?           : %s\n", rw(ATA_IDENT_COMMAND_SET) & (1 <<  10) ? "yes" : "no");

    klogmem((uint8_t*)buffer, buffer_size);

    KINFO("IDE", "Done\n");
}

static DeviceStatus PollStatus(struct ATA_IDE* ide)
{
    // dummy read STATUS 15 times (min delay 400ns, extra cautious)
    for (int i = 0; i < 15; ++i)
    {
        __asm volatile("tst.b 0xbfe001": : : "cc"); // delay 1us
        uint8_t status = ide->status;
        (void) status;
        KVERBOSE("IDE", "dummy read #%2ld; status = %02lx\n", i, status);
    }

    uint8_t status = ide->status;

    KDEBUG("IDE", "Status: %02lx\n", status);

    if (!(status & (ATA_SRF_BSY | ATA_SRF_DRDY)))
    {
        KVERBOSE("IDE", "!(ATA_SRF_BSY | ATA_SRF_DRDY)\n");
        return DeviceStatus_DeviceNotReady;
    }

    if ((status & (ATA_SRF_BSY | ATA_SRF_DRDY)) == (ATA_SRF_BSY | ATA_SRF_DRDY))
    {
        KVERBOSE("IDE", "(ATA_SRF_BSY | ATA_SRF_DRDY) == (ATA_SRF_BSY | ATA_SRF_DRDY)\n");
        return DeviceStatus_DeviceNotPresent;
    }

    if (status & ATA_SRF_BSY)
    {
        KVERBOSE("IDE", "ATA_SRF_BSY\n");
        return DeviceStatus_DeviceBusy;
    }

    if (status & ATA_SRF_ERR)
    {
        KVERBOSE("IDE", "ATA_SRF_ERR\n");
        return DeviceStatus_DeviceError;
    }

    if (status & ATA_SRF_DF)
    {
        KVERBOSE("IDE", "ATA_SRF_DF\n");
        return DeviceStatus_DeviceFault;
    }

    // if (!(status & ATA_SRF_DRQ)) {
    //     KVERBOSE("IDE", "!ATA_SRF_DRQ\n");
    //     return DeviceStatus_DeviceNotReady;
    // }

    return DeviceStatus_NoError;
}

static void PrintStatus(struct ATA_IDE* ide, DeviceStatus status)
{
    switch (status)
    {
        case DeviceStatus_NoError:
            KDEBUG("IDE", "Device ready to accept commands\n");
            return;

        case DeviceStatus_DeviceNotPresent:
            KINFO("IDE", "Device not present\n");
            return;

        case DeviceStatus_DeviceBusy:
            KWARN("IDE", "Device busy\n");
            return;

        case DeviceStatus_DeviceFault:
            KERROR("IDE", "Device faulted\n");
            return;

        case DeviceStatus_DeviceNotReady:
            KWARN("IDE", "Device not ready\n");
            return;

        case DeviceStatus_DeviceError:
            break;
    }

    uint8_t error = ide->error;

    if (!error)
        KERROR("IDE", "Error, no error\n");

    if (error & ATA_ERF_AMNF)
        KERROR("IDE", "No address mark\n");

    if (error & ATA_ERF_TK0NF)
        KERROR("IDE", "Track 0 not found\n");

    if (error & ATA_ERF_ABRT)
        KERROR("IDE", "Command aborted\n");

    if (error & ATA_ERF_MCR)
        KERROR("IDE", "Media change request\n");

    if (error & ATA_ERF_IDNF)
        KERROR("IDE", "ID mark not found\n");

    if (error & ATA_ERF_MC)
        KERROR("IDE", "Media changed\n");

    if (error & ATA_ERF_UNC)
        KERROR("IDE", "Uncorrectable data\n");

    if (error & ATA_ERF_BBK)
        KERROR("IDE", "Bad block\n");
}

static bool DetectDrive(struct ATA_IDE* ide, uint8_t drive)
{
    const uint8_t obsolete = (1 << 7) | (1 << 5);   // obsolete bits = $a0
    const uint8_t DEVSEL = 4;                       // bit 4 in HEAD is DEVSEL
    drive &= 1;                                     // only two drives per channel

    uint16_t timeout = drive ? 1 : 5;               // 5s timeout for master, 1s for slave

    do
    {
        KDEBUG("IDE", "Drive select: %s\n", drive ? "slave" : "master");

        ide->driveHead = obsolete | (drive << DEVSEL);  // select device

        DeviceStatus status = PollStatus(ide);
        PrintStatus(ide, status);

        if (status == DeviceStatus_DeviceNotReady)
        {
            SleepTimer(1, 0);                       // delay 1s to allow drive to spin up
            continue;
        }

        if (status == DeviceStatus_NoError)
            break;

        // all else fails
        return false;
    }
    while (--timeout);

    if (!timeout)
    {
        KWARN("IDE", "No drive found!\n");
        return false;
    }

    ide->cylLow = 0x12;

    if (ide->cylLow != 0x12)
    {
        KWARN("IDE", "Failed - No drive!\n");
        return false;
    }

    ide->cylLow = 0x34;

    if (ide->cylLow != 0x34)
    {
        KWARN("IDE", "Failed - No drive!\n");
        return 0;
    }

    KINFO("IDE", "Drive found\n");
    return true;
}

static void SleepTimer(uint32_t secs, uint32_t micros)
{
    struct ExecBase* SysBase = *(struct ExecBase**)4;

    KDEBUG("IDE", "Sleep %ld s and %ld us...\n", secs, micros);

    struct MsgPort mp;
    mp.mp_Node.ln_Type = NT_MSGPORT;
    mp.mp_Node.ln_Pri = 0;
    mp.mp_Node.ln_Name = NULL;
    mp.mp_Flags = 0;
    mp.mp_SigBit = SIGB_SINGLE;
    mp.mp_SigTask = FindTask(NULL);
    NewList(&mp.mp_MsgList);

    struct timerequest tr;

    uint32_t unit = secs < 4 ? UNIT_MICROHZ : UNIT_VBLANK;

    if (OpenDevice((const uint8_t*)TIMERNAME, unit, &tr.tr_node, 0))
        return;

    tr.tr_node.io_Message.mn_ReplyPort = &mp;
    tr.tr_node.io_Command = TR_ADDREQUEST;
    tr.tr_time.tv_sec = secs;
    tr.tr_time.tv_micro = micros;
    DoIO(&tr.tr_node);

    CloseDevice(&tr.tr_node);

}
